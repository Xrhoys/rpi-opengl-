#include "app.h"

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

// LINUX POSIX API
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#include <stdio.h>

#include "linux_main.h"

// TODO(Ecy): to exclude with proper platform code
#include "app.cpp"

// NOTE(Ecy): X11 globals, thsoe need to be global variable
global Window                 root;
global Window                 window;
global Display                *xdisplay;
global XSetWindowAttributes   swa;
global u32                    root_w, root_h, root_border_width, root_depth;

char buffertest[256];

inline u32 
GetTicks()
{
  u64 ticks = 0;
  u32 a, d;
  asm volatile("rdtsc" : "=a" (a), "=d" (d));

  return a;
}

internal
DEBUG_CLOCK_GET_TIME(LinuxGetLastElapsed)
{
	return 0.0f;
}

internal
DEBUG_PLATFORM_FREE_FILE_MEMORY(LinuxFreeFile)
{
	free(memory);
}

internal
DEBUG_PLATFORM_WRITE_ENTIRE_FILE(LinuxWriteEntireFile)
{
	u32 flags = O_WRONLY | O_CREAT | O_TRUNC;
	u32 permissionFlags = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
	i32 fd = open(filename, flags, permissionFlags);

	if(fd == -1) 
	{
		// TODO(Ecy): handle file open error
		return false;
	}

	ssize_t bytesWritten = write(fd, memory, memorySize);
	if(bytesWritten == -1)
	{
		close(fd);
		// TODO(Ecy): handle file write error
		return false;
	}
	
	close(fd);

	return true;
}

internal
DEBUG_PLATFORM_READ_ENTIRE_FILE(LinuxReadEntireFile)
{
	debug_read_file_result file;

	i32 fd = open(filename, O_RDONLY);

	if(fd == -1) 
	{
		// TODO(Ecy): handle file open error
	}

	struct stat st;
	stat(filename, &st);

	// TODO(Ecy): investigate on the off_t size, this could vary per machine
	off_t size = st.st_size;

	u8 *buffer;
	ssize_t bytesRead;
	if(size > 0) 
	{
		buffer = (u8*)malloc(size);
		bytesRead = read(fd, buffer, size);

		if(bytesRead != size)
		{
			// TODO(Ecy): handle file read error
		}
	}

	file.contents    = buffer;
	file.contentSize = bytesRead;

	close(fd);

	return file;
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
			state->startHoldTime = LinuxGetLastElapsed();
			// state->startHoldTime = Win32GetSecondsElapsed(g_bootCounter, g_lastCounter);
		}
		else
		{
			state->startHoldTime = INFINITY;
		}
    }
}

internal void 
ProcessEvent(app_state *appContext, XEvent *xev, Atom *wmDeleteMessage, app_keyboard_input *keyInput, app_pointer_input *pointerInput) {
	Atom wmDeleteMessage1 = XInternAtom(xdisplay, "WM_DELETE_WINDOW", False);
	XSetWMProtocols(xdisplay, window, &wmDeleteMessage1, 1);
	KeySym keysym;
	char keytext[255];
	bool isDown = true;

	XKeyEvent *keyEvent = (XKeyEvent*)xev;
	// NOTE(Nico): this is weird but necessary for key lookup...
	XLookupString(keyEvent,keytext,255, &keysym,0);

	// NOTE(Ecy): do not call printf here, it will cause segfault or being ignored
	switch (xev->type) {
		case KeyPress:
		{
			isDown = true;
			if(keysym == XK_Escape) {
				appContext->running = false;
			}
			else if (keysym == XK_w)
			{
				ProcessKeyboardMessage(&keyInput->keys[KEY_W], isDown);
			}
			else if (keysym == XK_a)
			{
				ProcessKeyboardMessage(&keyInput->keys[KEY_A], isDown);
			}
			else if (keysym == XK_s)
			{
				ProcessKeyboardMessage(&keyInput->keys[KEY_S], isDown);
			}
			else if (keysym == XK_d)
			{
				ProcessKeyboardMessage(&keyInput->keys[KEY_D], isDown);
			}
			else if (keysym == XK_Alt_L || keysym == XK_Alt_R)
			{
				ProcessKeyboardMessage(&keyInput->keys[KEY_ALT], isDown);
			}
			else if (keysym == XK_Control_L || keysym == XK_Control_R)
			{
				ProcessKeyboardMessage(&keyInput->keys[KEY_CTRL], isDown);
			}
			else if (keysym == XK_space)
			{
				ProcessKeyboardMessage(&keyInput->keys[KEY_SPACE], isDown);
			}
			else if (keysym == XK_F1)
			{
				ProcessKeyboardMessage(&keyInput->keys[KEY_F1], isDown);
			}
			else if (keysym == XK_F2)
			{
				ProcessKeyboardMessage(&keyInput->keys[KEY_F2], isDown);
			}
			else if (keysym == XK_F3)
			{
				ProcessKeyboardMessage(&keyInput->keys[KEY_F3], isDown);
			}
			else if (keysym == XK_F4)
			{
				ProcessKeyboardMessage(&keyInput->keys[KEY_F4], isDown);
			}
			else if (keysym == XK_F5)
			{
				ProcessKeyboardMessage(&keyInput->keys[KEY_F5], isDown);
			}
			else if (keysym == XK_F6)
			{
				ProcessKeyboardMessage(&keyInput->keys[KEY_F6], isDown);
			}
			else if (keysym == XK_Shift_L || keysym == XK_Shift_R)
			{
				ProcessKeyboardMessage(&keyInput->keys[KEY_SHIFT], isDown);
			}
		} break;
		
		case KeyRelease:
		{
			isDown = false;
			if (keysym == XK_w)
			{
				ProcessKeyboardMessage(&keyInput->keys[KEY_W], isDown);
			}
			else if (keysym == XK_a)
			{
				ProcessKeyboardMessage(&keyInput->keys[KEY_A], isDown);
			}
			else if (keysym == XK_s)
			{
				ProcessKeyboardMessage(&keyInput->keys[KEY_S], isDown);
			}
			else if (keysym == XK_d)
			{
				ProcessKeyboardMessage(&keyInput->keys[KEY_D], isDown);
			}
			else if (keysym == XK_Alt_L || keysym == XK_Alt_R)
			{
				ProcessKeyboardMessage(&keyInput->keys[KEY_ALT], isDown);
			}
			else if (keysym == XK_Control_L || keysym == XK_Control_R)
			{
				ProcessKeyboardMessage(&keyInput->keys[KEY_CTRL], isDown);
			}
			else if (keysym == XK_space)
			{
				ProcessKeyboardMessage(&keyInput->keys[KEY_SPACE], isDown);
			}
			else if (keysym == XK_F1)
			{
				ProcessKeyboardMessage(&keyInput->keys[KEY_F1], isDown);
			}
			else if (keysym == XK_F2)
			{
				ProcessKeyboardMessage(&keyInput->keys[KEY_F2], isDown);
			}
			else if (keysym == XK_F3)
			{
				ProcessKeyboardMessage(&keyInput->keys[KEY_F3], isDown);
			}
			else if (keysym == XK_F4)
			{
				ProcessKeyboardMessage(&keyInput->keys[KEY_F4], isDown);
			}
			else if (keysym == XK_F5)
			{
				ProcessKeyboardMessage(&keyInput->keys[KEY_F5], isDown);
			}
			else if (keysym == XK_F6)
			{
				ProcessKeyboardMessage(&keyInput->keys[KEY_F6], isDown);
			}
			else if (keysym == XK_Shift_L || keysym == XK_Shift_R)
			{
				ProcessKeyboardMessage(&keyInput->keys[KEY_SHIFT], isDown);
			}
			break;
		}
		case MotionNotify:
		break;
		case ButtonPress:
		{
			isDown = true;
			pointerInput->mouseX += pointerInput->sensX * xev->xbutton.x;
			pointerInput->mouseY += pointerInput->sensY * xev->xbutton.y;	
			if (xev->xbutton.button == 1)
			{
				ProcessKeyboardMessage(&pointerInput->buttons[MOUSE_LEFT], isDown);
			}
			else if (xev->xbutton.button == 2)
			{
				ProcessKeyboardMessage(&pointerInput->buttons[MOUSE_MIDDLE], isDown);
			}
			else if (xev->xbutton.button == 3)
			{
				ProcessKeyboardMessage(&pointerInput->buttons[MOUSE_RIGHT], isDown);
			}

			// Scroll - button4
			
			break;
		}
		case ButtonRelease:
		{
			isDown = false;
			if (xev->xbutton.button == 1) {
				ProcessKeyboardMessage(&pointerInput->buttons[MOUSE_LEFT], isDown);
			}
			else if (xev->xbutton.button == 2) {
				ProcessKeyboardMessage(&pointerInput->buttons[MOUSE_MIDDLE], isDown);
			}
			else if (xev->xbutton.button == 3) {
				ProcessKeyboardMessage(&pointerInput->buttons[MOUSE_RIGHT], isDown);
			}

			// Scroll - button4

			break;
		}
		case ClientMessage:
            if (xev->xclient.data.l[0] == wmDeleteMessage1)
                appContext->running = false;
            break;
		default:
		{
		} break;
	}
}

int main(int argc, char *argv[]) {
	xdisplay = XOpenDisplay(NULL);
	if (xdisplay == NULL) {
		printf("Error opening X display\n");
		return 0;
	}
	
	// All events the window accepts
	swa.event_mask = StructureNotifyMask | ExposureMask | PointerMotionMask |
		KeyPressMask | KeyReleaseMask | ButtonPressMask |
		ButtonReleaseMask;
	
	// Get root window
	root = DefaultRootWindow(xdisplay);
	i32 root_x, root_y;
	Window root_again;
	
	XGetGeometry(xdisplay, root, &root_again, &root_x, &root_y, &root_w, &root_h, &root_border_width, &root_depth);
	
	// Create window
	window = XCreateWindow(xdisplay, root,
							0, 0, root_w, root_h, 0,
							CopyFromParent, InputOutput,
							CopyFromParent, CWEventMask,
							&swa);
	
	XSetWindowAttributes xattr;
	
	xattr.override_redirect = False;
	XChangeWindowAttributes(xdisplay, window, CWOverrideRedirect, &xattr);
	
	Atom atomWmDeleteWindow = XInternAtom(xdisplay, "WM_DELETE_WINDOW", false);
	XSetWMProtocols(xdisplay, root, &atomWmDeleteWindow, 1);
	
	i32 one = 1;
	XChangeProperty(
					xdisplay, window,
					XInternAtom (xdisplay, "_HILDON_NON_COMPOSITED_WINDOW", False),
					XA_INTEGER,  32,  PropModeReplace,
					(unsigned char*) &one, 1);
	
	XWMHints hints;
	hints.input = True;
	hints.flags = InputHint;
	XSetWMHints(xdisplay, window, &hints);
	
	XMapWindow(xdisplay, window); // make window visible
	XSync(xdisplay, window);
	XStoreName(xdisplay, window, "RPI Emulation");
	
	app_keyboard_input keyInputs[2] = {};
	app_keyboard_input *oldKeyInput = &keyInputs[0];
	app_keyboard_input *newKeyInput = &keyInputs[1];
	
	app_pointer_input pointerInputs[2] = {};
	app_pointer_input *oldPointerInput = &pointerInputs[0];
	app_pointer_input *newPointerInput = &pointerInputs[1];
	
	app_state g_state;
	{
		g_state.running = true;
		
		g_state.clock = 0;
		g_state.frameTime = 0;
		g_state.getTime = LinuxGetLastElapsed;
		
		// NOTE(Ecy): needs to set at runtime + dynamic with resize event
		XWindowAttributes winAttr;
		XGetWindowAttributes(xdisplay, window, &winAttr);
		g_state.width = winAttr.width;
		g_state.height = winAttr.height;
		
		g_state.DEBUGPlatformReadEntireFile  = LinuxReadEntireFile;
		g_state.DEBUGPlatformWriteEntireFile = LinuxWriteEntireFile;
		g_state.DEBUGPlatformFreeFileMemory  = LinuxFreeFile;
		
		g_state.permanentStorageSize = Megabytes(256);
		g_state.permanentStorage = malloc(g_state.permanentStorageSize);
		if(!g_state.permanentStorage)
		{
			// TODO(Ecy): log errors
			return -1;
		}
		g_state.transientStorageSize = Gigabytes(1);
		g_state.transientStorage     = malloc(g_state.transientStorageSize);
		if(!g_state.transientStorage)
		{
			// TODO(Ecy): log errors
			return -1;
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
	
#ifdef BE_OPENGL
	
	// EGL
	EGLDisplay eglDisplay = {};
	EGLSurface eglSurface = {};
	{
		eglDisplay = eglGetDisplay((EGLNativeDisplayType)xdisplay);
		if (eglDisplay == EGL_NO_DISPLAY) {
			printf("Error getting EGL display\n");
			return 1;
		}
		EGLint eglVersionMajor, eglVersionMinor;
		eglInitialize(eglDisplay, &eglVersionMajor, &eglVersionMinor);
		if (!eglInitialize(eglDisplay, &eglVersionMajor, &eglVersionMinor)) {
			printf("Error initializing EGL\n");
			return 1;
		}
		
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
		
		EGLint surfaceAttributes[] = {EGL_NONE};
		EGLint contextAttributes[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
		
		EGLint nrOfConfigs;
		EGLConfig windowConfig;
		eglChooseConfig(eglDisplay, configAttributes, &windowConfig, 1, &nrOfConfigs);
		eglSurface = eglCreateWindowSurface(eglDisplay, windowConfig, (EGLNativeWindowType)window, surfaceAttributes);
		
		if (eglSurface == EGL_NO_SURFACE) {
			EGLint error = eglGetError();
			
			char *errorStr = getErrorStr(error);
			fprintf(stdout, "Could not create EGL surface : %s\n", errorStr);
			return 1;
		}
		
		EGLContext eglContext =
			eglCreateContext(eglDisplay, windowConfig, NULL, contextAttributes);
		
		EGLConfig *eglConfigs;
		EGLint eglNumConfigs;
		eglGetConfigs(eglDisplay, NULL, 0, &eglNumConfigs);
		eglConfigs = (EGLConfig*)malloc(sizeof(EGLConfig) * eglNumConfigs);
		
		eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext);
	}
	
	InitRenderer();
#elif BE_VULKAN
	vk_render_context renderContext = {};
	
	char *extensions[32];
	u32 extensionCount = 0;
	extensions[extensionCount++] = "VK_KHR_xlib_surface";
	
	CreateDeviceContext(&renderContext, extensions, &extensionCount);
	
	VkResult result;
	VkXlibSurfaceCreateInfoKHR createSurfaceInfo =
	{
		VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR,
		0,
		0,
		xdisplay,
		window,
	};
	
	result = vkCreateXlibSurfaceKHR(renderContext.instance, &createSurfaceInfo, nullptr, &renderContext.surface);
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
	
	{
		video_decode_vulkan vulkanDecoder = {};
		//LoadVideoContext(&decoder, SAMPLE_DATA);
		LoadVulkanVideoContext(&vulkanDecoder, "data/sample.mp4");
		
		InitVideoDecoder(&renderContext, &vulkanDecoder);
		//debug_read_file_result mp4File = appContext->DEBUGPlatformReadEntireFile(NULL, "data/sample.mp4");
		
		//MP4VideoDemuxer(&mp4File);
	}
	
#endif
	
	InitApp(&g_state);
	
	while(g_state.running)
	{
		// NOTE(Nico): Reset input
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
			
			// TODO: (Nico) switch to linux
			i32 root_x, root_y, win_x, win_y;
			u32 mask;
			Window dummy;
			int mouseRet = XQueryPointer(xdisplay, window, &dummy, &dummy, &root_x, &root_y, &win_x, &win_y, &mask);
			newPointerInput->posX = win_x;
			newPointerInput->posY = win_y;
		}

		while(XPending(xdisplay))
		{
			XEvent xev;
			
			XNextEvent(xdisplay, &xev);
			ProcessEvent(&g_state, &xev, &atomWmDeleteWindow, newKeyInput, newPointerInput);
		}
			
		if (!g_state.running) 
			break;

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
		
		// Timer update
		{
			// NOTE(Ecy): placeholder
			g_state.clock = GetTicks();
			g_state.frameTime = GetTicks();
		}
		
	}
	
	XDestroyWindow(xdisplay, window);
	XCloseDisplay(xdisplay);
	
	return 0;
}
