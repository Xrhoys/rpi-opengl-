#include "app.h"

#include <windows.h>
#include <hidusage.h>
#include <tchar.h>

#include "win32_main.h"
// TODO(Ecy): to exclude with proper platform code
#include "app.cpp"

global u64       g_perfCount;
global u64       g_bootCounter;
global u64       g_lastCounter;
global u64       lastCycleCount;
global r64       cyclesPerFrame;
global HWND      hwnd;

global app_state g_state;

internal
DEBUG_PLATFORM_PRINT_OUT(Win32OutputDebugString)
{
	OutputDebugStringA(buffer);
}

internal r32
Win32GetSecondsElapsed(u64 start, u64 end)
{
	r64 endTime   = (r64)end;
    r64 startTime = (r64)start;
	r64 perfCount = (r64)g_perfCount;
	r32 result    = (r32)((endTime - startTime) / perfCount);
    
    return result;
}

internal u64
Win32GetWallClock()
{
    LARGE_INTEGER result;
    QueryPerformanceCounter(&result);
    return result.QuadPart;
}

internal void
ProcessKeyboardMessage(app_input_state *state, b32 isDown)
{
    if(state->endedDown != isDown)
    {
        state->endedDown = isDown;
        ++state->halfTransitionCount;
		
		if(isDown)
		{
			state->startHoldTime = Win32GetSecondsElapsed(g_bootCounter, g_lastCounter);
		}
		else
		{
			state->startHoldTime = INFINITY;
		}
    }
}

internal void
Win32ProcessInputEvent(app_keyboard_input *keyInput, app_pointer_input *pointerInput, LPARAM *lParam)
{
	u32 dwSize;
	
	HRAWINPUT input = (HRAWINPUT)*lParam;
	
	GetRawInputData(input, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));
	
	if(dwSize != 0)
	{
		// TODO: Experiment, moving input on a secondary thread
		// Could have latency concerns
		char buffer[256]; // 1KB, may be enough?
		LPBYTE *lpb = (LPBYTE *)buffer;
		
		GetRawInputData(input, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER));
		
		RAWINPUT* raw = (RAWINPUT *)lpb;
		
		if(raw->header.dwType == RIM_TYPEKEYBOARD)
		{
			RAWKEYBOARD keyboardData = raw->data.keyboard;
#if 0			
			// NOTE: doesn't seem like this is necessary
			b32 wasDown = ((keyboardData.Message & (1 << 30)) != 0);
			b32 isDown  = ((keyboardData.Message & (1 << 31)) == 0);
#endif
			b32 isDown = keyboardData.Flags == RI_KEY_MAKE;
			u8  key     = keyboardData.VKey;
#if 0
			char buffer[256];
			_snprintf_s(buffer, sizeof(buffer), "State: %c, Input: %d\n", key, isDown);
			OutputDebugStringA(buffer);
#endif
			if (key == 'W')
			{
				ProcessKeyboardMessage(&keyInput->keys[KEY_W], isDown);
			}
			else if (key == 'A')
			{
				ProcessKeyboardMessage(&keyInput->keys[KEY_A], isDown);
			}
			else if (key == 'S')
			{
				ProcessKeyboardMessage(&keyInput->keys[KEY_S], isDown);
			}
			else if (key == 'D')
			{
				ProcessKeyboardMessage(&keyInput->keys[KEY_D], isDown);
			}
			else if (key == VK_ESCAPE)
			{
				g_state.running = false;
			}
			else if (key == VK_MENU)
			{
				ProcessKeyboardMessage(&keyInput->keys[KEY_ALT], isDown);
			}
			else if (key == VK_CONTROL)
			{
				ProcessKeyboardMessage(&keyInput->keys[KEY_CTRL], isDown);
			}
			else if (key == VK_SPACE)
			{
				ProcessKeyboardMessage(&keyInput->keys[KEY_SPACE], isDown);
			}
			else if (key == VK_F1)
			{
				ProcessKeyboardMessage(&keyInput->keys[KEY_F1], isDown);
			}
			else if (key == VK_F2)
			{
				ProcessKeyboardMessage(&keyInput->keys[KEY_F2], isDown);
			}
			else if (key == VK_F3)
			{
				ProcessKeyboardMessage(&keyInput->keys[KEY_F3], isDown);
			}
			else if (key == VK_F4)
			{
				ProcessKeyboardMessage(&keyInput->keys[KEY_F4], isDown);
			}
			else if (key == VK_F5)
			{
				ProcessKeyboardMessage(&keyInput->keys[KEY_F5], isDown);
			}
			else if (key == VK_F6)
			{
				ProcessKeyboardMessage(&keyInput->keys[KEY_F6], isDown);
			}
		}
		else if (raw->header.dwType == RIM_TYPEMOUSE)
		{
			RAWMOUSE mouseData = raw->data.mouse;
			
			pointerInput->mouseX += pointerInput->sensX * mouseData.lLastX;
			pointerInput->mouseY += pointerInput->sensY * mouseData.lLastY;	

			u16 leftDown   = mouseData.usButtonFlags & RI_MOUSE_LEFT_BUTTON_DOWN;
			u16 rightDown  = mouseData.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_DOWN;
			u16 middleDown = mouseData.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_DOWN;
			
			// NOTE(Ecy): mouse button flag mapping
			// https://learn.microsoft.com/en-us/windows/win32/api/winuser/ns-winuser-rawmouse
			ProcessKeyboardMessage(&pointerInput->buttons[MOUSE_LEFT], leftDown == RI_MOUSE_LEFT_BUTTON_DOWN);
			ProcessKeyboardMessage(&pointerInput->buttons[MOUSE_RIGHT],  rightDown == RI_MOUSE_RIGHT_BUTTON_DOWN);
			ProcessKeyboardMessage(&pointerInput->buttons[MOUSE_MIDDLE],  middleDown == RI_MOUSE_MIDDLE_BUTTON_DOWN);
			
			// NOTE(Ecy): handle vertical wheel scrolling
			if((mouseData.usButtonFlags & RI_MOUSE_WHEEL) == RI_MOUSE_WHEEL)
			{
				r32 wheelDelta = (r32)((i16)mouseData.usButtonData);
				r32 numTicks   = wheelDelta / WHEEL_DELTA;
				
				pointerInput->mouseZ += numTicks;
			}
		}
	}
}

internal
DEBUG_CLOCK_GET_TIME(Win32GetLastElapsed)
{
    u64 currentCounter = Win32GetWallClock();
	r32 elapsed = Win32GetSecondsElapsed(g_lastCounter, currentCounter);
    
    return elapsed;
}

internal
DEBUG_PLATFORM_FREE_FILE_MEMORY(Win32FreeFile)
{
	VirtualFree(memory, 0, MEM_RELEASE);
}

internal
DEBUG_PLATFORM_READ_ENTIRE_FILE(Win32ReadEntireFile)
{
	debug_read_file_result File;
    
    void *Result = 0;
    DWORD BytesRead;
	uint32_t FileSize32;
    HANDLE FileHandle = CreateFileA(filename,
                                    GENERIC_READ,
                                    FILE_SHARE_READ,
                                    0,
                                    OPEN_EXISTING,
                                    0,
                                    0);
    
    if(FileHandle != INVALID_HANDLE_VALUE)
    {
        LARGE_INTEGER FileSize;
        if(GetFileSizeEx(FileHandle, &FileSize))
        {
            // NOTE: Will cause problem for 64bits
            FileSize32 = (uint32_t)FileSize.QuadPart;
            Result = VirtualAlloc(0, FileSize.QuadPart, 
                                  MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
            
            if(Result)
            {
                if(ReadFile(FileHandle, Result, FileSize32, &BytesRead, 0))
                {
                    File.contents = Result;
                    File.contentSize = FileSize32;
                }
                else
                {
                    VirtualFree(Result, 0, MEM_RELEASE);
                }
            }
            else
            {
                // TODO: Logging
            }
        }
        else
        {
            // TODO: Logging
        }
    }
    else
    {
        // TODO: Logging
    }
    
    return File;
}

internal
DEBUG_PLATFORM_WRITE_ENTIRE_FILE(Win32WriteEntireFile)
{
	b32 result = 0;
    
    HANDLE fileHandle = CreateFileA(filename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
    if(fileHandle != INVALID_HANDLE_VALUE)
    {
        DWORD bytesWritten;
        if(WriteFile(fileHandle, memory, memorySize, &bytesWritten, 0))
        {
            result = (bytesWritten == memorySize);
        }
        else
        {
            // TODO: Logging
			char err[256];
			FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(),
						  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), err, 255, NULL);
			
			int b = 0;
        }
		
        CloseHandle(fileHandle);
    }
    else
    {
        // TODO: Logging
    }
	
    return result;
}

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_SIZE:
		/* 
				if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
				{
					CleanupRenderTarget();
					g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
					CreateRenderTarget();
				}
		 */
        return 0;
        case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
        case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

int CALLBACK
WinMain(HINSTANCE Instance,
        HINSTANCE PrevInstance,
        LPSTR CommandLine,
        int ShowCode)
{
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), 
		NULL, NULL, NULL, NULL, _T("RPI Emulation"), NULL };
    RegisterClassEx(&wc);
    hwnd = CreateWindow(wc.lpszClassName, _T("RPI Emulation"), WS_OVERLAPPEDWINDOW, 
							 100, 100, WINDOW_WIDTH, WINDOW_HEIGHT, NULL, NULL, wc.hInstance, NULL);
    
    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);
	
	app_keyboard_input keyInputs[2] = {};
	app_keyboard_input *oldKeyInput = &keyInputs[0];
	app_keyboard_input *newKeyInput = &keyInputs[1];
	
	app_pointer_input pointerInputs[2] = {};
	app_pointer_input *oldPointerInput = &pointerInputs[0];
	app_pointer_input *newPointerInput = &pointerInputs[1];
	
	{
		g_state.running = true;
		
		lastCycleCount = __rdtsc();
		
		LARGE_INTEGER counter;
		LARGE_INTEGER perfCountFrequencyResult;
		QueryPerformanceCounter(&counter);
		QueryPerformanceFrequency(&perfCountFrequencyResult);
		
		g_bootCounter = counter.QuadPart;
		g_lastCounter = counter.QuadPart;
		g_perfCount   = perfCountFrequencyResult.QuadPart;
		
		g_state.clock     = 0.0f;
		g_state.frameTime = 0.0f;
		g_state.getTime   = Win32GetLastElapsed;
		
		// NOTE(Ecy): needs to set at runtime + dynamic with resize event
		g_state.width = WINDOW_WIDTH;
		g_state.height = WINDOW_HEIGHT;
		
		g_state.DEBUGPlatformReadEntireFile  = Win32ReadEntireFile;
		g_state.DEBUGPlatformWriteEntireFile = Win32WriteEntireFile;
		g_state.DEBUGPlatformFreeFileMemory  = Win32FreeFile;
		g_state.DEBUGPlatformPrint           = Win32OutputDebugString;
			
		g_state.permanentStorageSize = Megabytes(256);
		g_state.permanentStorage = VirtualAlloc(0, g_state.permanentStorageSize, 
												MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
		if(!g_state.permanentStorage)
		{
			// TODO(Ecy): log errors
			return -1;
		}
		g_state.transientStorageSize = Gigabytes(1);
		g_state.transientStorage     = VirtualAlloc(0, g_state.transientStorageSize, 
													MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
		if(!g_state.transientStorage)
		{
			// TODO(Ecy): log errors
			return -1;
		}
		g_state.isInitialized = true;
		
		// NOTE(Ecy): windows raw input api
		RAWINPUTDEVICE devices[2];
		
		devices[0].usUsagePage = HID_USAGE_PAGE_GENERIC;
		devices[0].usUsage     = HID_USAGE_GENERIC_KEYBOARD;
		devices[0].dwFlags     = RIDEV_NOLEGACY;
		devices[0].hwndTarget  = 0;
		
		devices[1].usUsagePage = HID_USAGE_PAGE_GENERIC;
		devices[1].usUsage     = HID_USAGE_GENERIC_MOUSE;
		devices[1].dwFlags     = RIDEV_NOLEGACY;
		devices[1].hwndTarget  = 0;
		
		if(RegisterRawInputDevices(devices, 2, sizeof(devices[0])) == false)
		{
			// TODO(Ecy): log errors
		}
		
		g_state.keyboards[0] = oldKeyInput;
		g_state.pointers[0]  = oldPointerInput;
		
		g_state.keyboards[0]->isConnected = true;
		g_state.pointers[0]->isConnected = true;
		
		for(u32 index = 0;
			index < KEY_COUNT;
			index++)
		{
			g_state.keyboards[0]->keys[index].startHoldTime = INFINITY;
		}
		
		for(u32 index = 0;
			index < MOUSE_BUTTON_COUNT;
			index++)
		{
			g_state.pointers[0]->buttons[index].startHoldTime = INFINITY;
		}
		
	}
	
	// TODO(Ecy): make it so we can change renderer at runtime instead of build dependent 
#ifdef BE_OPENGL	
	HDC hdc = GetDC(hwnd);
    EGLDisplay eglDisplay = eglGetDisplay(hdc);
	EGLint eglVersionMajor, eglVersionMinor;
    eglInitialize(eglDisplay, &eglVersionMajor, &eglVersionMinor);
	
	eglBindAPI(EGL_OPENGL_ES_API);
	
	EGLint configAttributes[] =
	{
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_DEPTH_SIZE, 16,
		EGL_BLUE_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_RED_SIZE, 8,
		EGL_ALPHA_SIZE, 8,
		EGL_NONE
	};
	
	EGLint surfaceAttributes[] = { EGL_NONE };
	EGLint contextAttributes[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };
	
	EGLint nrOfConfigs;
	EGLConfig windowConfig;
	eglChooseConfig(eglDisplay, configAttributes, &windowConfig, 1, &nrOfConfigs);
	EGLSurface eglSurface = eglCreateWindowSurface(eglDisplay, windowConfig, hwnd, surfaceAttributes);
	if (eglSurface == EGL_NO_SURFACE) {
		EGLint error = eglGetError();
		
		char *errorStr = getErrorStr(error);
		fprintf(stdout, "Could not create EGL surface : %s\n", errorStr);
		return 1;
	}
	
	EGLContext eglContext = eglCreateContext(eglDisplay, windowConfig, NULL, contextAttributes);

	i32 eglNumConfigs;
	eglGetConfigs(eglDisplay, NULL, 0, &eglNumConfigs);
	EGLConfig *eglConfigs = (EGLConfig*)VirtualAlloc(0, sizeof(EGLConfig) * eglNumConfigs, 
													 MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
	
	eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext);
	
	InitRenderer();
	
#elif BE_VULKAN
	vk_render_context renderContext = {};
	
	char *extensions[32];
	u32 extensionCount = 0;
	extensions[extensionCount++] = VK_KHR_WIN32_SURFACE_EXTENSION_NAME;
	
	CreateDeviceContext(&renderContext, extensions, &extensionCount);
	
	VkResult result;
	VkWin32SurfaceCreateInfoKHR createSurfaceInfo =
	{
		VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
		0,
		0,
		Instance,
		hwnd,
	};
	
	result = vkCreateWin32SurfaceKHR(renderContext.instance, &createSurfaceInfo, nullptr, &renderContext.surface);
	CheckRes(result);
	
	// Check for WSI support
	VkBool32 res;
	vkGetPhysicalDeviceSurfaceSupportKHR(renderContext.physicalDevice, renderContext.queueFamily, 
										 renderContext.surface, &res);
	if(res != VK_TRUE)
	{
		// WSI not supported
	}
	
	VkSurfaceFormatKHR formats[16];
	u32 formatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(renderContext.physicalDevice, renderContext.surface, &formatCount, nullptr);
	if(formatCount > 0)
	{
		vkGetPhysicalDeviceSurfaceFormatsKHR(renderContext.physicalDevice, renderContext.surface, &formatCount, formats);
	}
	
	renderContext.presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
	VkPresentModeKHR presentModes[16];
	u32 presentModeCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(renderContext.physicalDevice, renderContext.surface, 
											  &presentModeCount, nullptr);
	if(presentModeCount > 0)
	{
		vkGetPhysicalDeviceSurfacePresentModesKHR(renderContext.physicalDevice, renderContext.surface, 
												  &presentModeCount, presentModes);
	}
	
	renderContext.surfaceFormat = formats[0];
	for(u32 formatIndex = 0;
		formatIndex < formatCount;
		++formatIndex)
	{
		if(formats[formatIndex].format == VK_FORMAT_B8G8R8A8_UNORM && 
		   formats[formatIndex].colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR)
		{
			renderContext.surfaceFormat = formats[formatIndex];
			break;
		}
	}
	
	CreateSwapchain(&renderContext);
	CreatePipeline(&renderContext, &g_state);

#if 0	
	{
		{
			video_decode_vulkan vulkanDecoder = {};
			//LoadVideoContext(&decoder, SAMPLE_DATA);
			LoadVulkanVideoContext(&vulkanDecoder, "data/sample.mp4");
			
			InitVideoDecoder(&renderContext, &vulkanDecoder);
			//debug_read_file_result mp4File = appContext->DEBUGPlatformReadEntireFile(NULL, "data/sample.mp4");
			
			//MP4VideoDemuxer(&mp4File);
		}
	}
	#endif

#endif
	
	InitApp(&g_state);
	
	while(g_state.running)
	{
		// NOTE(Ecy): reset input
		{
			*newKeyInput     = {};
			*newPointerInput = {};
			
			for(u32 index = 0;
				index < KEY_COUNT;
				++index)
			{
				newKeyInput->keys[index].endedDown = oldKeyInput->keys[index].endedDown;
				newKeyInput->keys[index].halfTransitionCount = oldKeyInput->keys[index].halfTransitionCount;
				
				if(newKeyInput->keys[index].endedDown)
				{
					newKeyInput->keys[index].startHoldTime = oldKeyInput->keys[index].startHoldTime;
				}
			}
			
			for(u32 index = 0;
				index < MOUSE_BUTTON_COUNT;
				++index)
			{
				newPointerInput->buttons[index].endedDown = oldPointerInput->buttons[index].endedDown;
				newPointerInput->buttons[index].halfTransitionCount = oldPointerInput->buttons[index].halfTransitionCount;
				if(newPointerInput->buttons[index].endedDown)
				{
					newPointerInput->buttons[index].startHoldTime = oldPointerInput->buttons[index].startHoldTime;
				}
			}

			newPointerInput->mouseX = oldPointerInput->mouseX;
			newPointerInput->mouseY = oldPointerInput->mouseY;
			newPointerInput->mouseZ = oldPointerInput->mouseZ;
			newPointerInput->sensX  = oldPointerInput->sensX;
			newPointerInput->sensY  = oldPointerInput->sensY;
			newPointerInput->sensZ  = oldPointerInput->sensZ;
			
			POINT lpPoint;
			GetCursorPos(&lpPoint);
			ScreenToClient(hwnd, &lpPoint);
			newPointerInput->posX = lpPoint.x;
			newPointerInput->posY = lpPoint.y;
		}
		
		MSG msg;
        while(PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
			switch(msg.message)
			{
				case WM_QUIT:
				{
					g_state.running = false;
				}break;
				case WM_INPUT:
				{
					Win32ProcessInputEvent(newKeyInput, newPointerInput, &msg.lParam);
				}break;
				default:
				{
					// NOTE(Ecy): non handled
					int b = 0;
				}break;
			}
			
			TranslateMessage(&msg);
			DispatchMessage(&msg);
        }
		if(!g_state.running) break;
		
		// NOTE(Ecy): process inputs. For the time being only the first one of each type. 
		// Not sure if the dual input buffer will be useful ..
		{
			g_state.keyboards[0] = newKeyInput;
			g_state.pointers[0]  = newPointerInput;
			
			app_keyboard_input *tempKeyInput = newKeyInput;
			newKeyInput = oldKeyInput;
			oldKeyInput = tempKeyInput;
			
			app_pointer_input *tempPointerInput = newPointerInput;
			newPointerInput = oldPointerInput;
			oldPointerInput = tempPointerInput;
		}
		
		UpdateApp(&g_state);
		
#ifdef BE_OPENGL
		Render();
		// TODO(Ecy): opengl es render context
		eglSwapBuffers(eglDisplay, eglSurface);
#elif BE_VULKAN
		Render(&renderContext, &g_state);
#endif

		{
			u64 endCycleCounter = __rdtsc();
			u64 cyclesElapsed   = endCycleCounter - lastCycleCount;
			cyclesPerFrame      = cyclesElapsed / (1000.0f * 1000.0f);
			
			g_state.clock       = Win32GetWallClock();
			g_state.frameTime   = Win32GetLastElapsed();
			
			lastCycleCount      = endCycleCounter;
			g_lastCounter       = Win32GetWallClock();
		}
	}
	
    DestroyWindow(hwnd);
    UnregisterClass(wc.lpszClassName, wc.hInstance);
    
    return 0;
}
