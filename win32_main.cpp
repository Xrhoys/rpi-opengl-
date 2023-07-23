#include "app.h"

#include <windows.h>
#include <stdio.h>
#include <tchar.h>

#include "win32_main.h"

// TODO(Ecy): to exclude with proper platform code
#include "app.cpp"

global u64       g_perfCount;
global u64       g_bootCounter;
global u64       g_lastCounter;
global u64       lastCycleCount;
global r64       cyclesPerFrame;

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
    HWND hwnd = CreateWindow(wc.lpszClassName, _T("RPI Emulation"), WS_OVERLAPPEDWINDOW, 
							 100, 100, WINDOW_WIDTH, WINDOW_HEIGHT, NULL, NULL, wc.hInstance, NULL);
    
    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);
	
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
	EGLConfig *eglConfigs = (EGLConfig*)malloc(sizeof(EGLConfig) * eglNumConfigs);
	
	eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext);
	
	app_state g_state;
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
	}
	
	InitApp(&g_state);
	
	while(g_state.running)
	{
		MSG msg;
        while (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			if (msg.message == WM_QUIT)
                g_state.running = 0;
        }
		
		if (!g_state.running)
            break;
		
		UpdateAndRenderApp(&g_state);
		
		eglSwapBuffers(eglDisplay, eglSurface);
		
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
