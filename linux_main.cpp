#include "app.h"

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

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
ProcessEvent(app_state *appContext, XEvent *xev) {
	// NOTE(Ecy): do not call printf here, it will cause segfault or being ignored
	switch (xev->type) {
		case KeyPress:
		{
			XKeyEvent *keyEvent = (XKeyEvent*)xev;
			
			// NOTE(Ecy): escape key
			if(keyEvent->keycode == 0x09) {
				appContext->running = false;
			}
		} break;
		
		case KeyRelease:
		case MotionNotify:
		case ButtonPress:
		case ButtonRelease:
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
	
	app_state g_state;
	{
		g_state.running = true;
		
		g_state.clock = 0;
		g_state.frameTime = 0;
		g_state.getTime = LinuxGetLastElapsed;
		
		// NOTE(Ecy): needs to set at runtime + dynamic with resize event
		g_state.width = WINDOW_WIDTH;
		g_state.height = WINDOW_HEIGHT;
		
		g_state.DEBUGPlatformReadEntireFile  = LinuxReadEntireFile;
		g_state.DEBUGPlatformWriteEntireFile = LinuxWriteEntireFile;
		g_state.DEBUGPlatformFreeFileMemory  = LinuxFreeFile;
	}
	
	InitApp(&g_state);
	
	while(g_state.running)
	{
		while(XPending(xdisplay))
		{
			XEvent xev;
			
			XNextEvent(xdisplay, &xev);
			ProcessEvent(&g_state, &xev);
		}
			
		if (!g_state.running) 
			break;
		
		UpdateAndRenderApp(&g_state);
		
		eglSwapBuffers(eglDisplay, eglSurface);
		
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
