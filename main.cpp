#include <windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>

#include <EGL/egl.h>
#include <EGL/eglplatform.h>
#include <GL/gl.h>
#include <GLES3/gl32.h>

#define GL_ES_VERSION_3_0 1

#include "main.h"

bool g_running = false;

int CALLBACK
WinMain(HINSTANCE Instance,
        HINSTANCE PrevInstance,
        LPSTR CommandLine,
        int ShowCode)
{
    // Create application window
    //ImGui_ImplWin32_EnableDpiAwareness();
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, _T("RPI Emulation"), NULL };
    ::RegisterClassEx(&wc);
    HWND hwnd = ::CreateWindow(wc.lpszClassName, _T("RPI Emulation"), WS_OVERLAPPEDWINDOW, 100, 100, WINDOW_WIDTH, WINDOW_HEIGH, NULL, NULL, wc.hInstance, NULL);
    
    // Show the window
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);
	
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
	
	EGLConfig *eglConfigs;
	int eglNumConfigs;
	eglGetConfigs(eglDisplay, NULL, 0, &eglNumConfigs);
	eglConfigs = new EGLConfig[eglNumConfigs];
	
	eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext);
	
	{
		
	}
	
	g_running = true;
	while(g_running)
	{
		MSG msg;
        while (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                g_running = false;
        }
		
		if (!g_running)
            break;
		
		{
			glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGH);
			glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);
		}
		
		eglSwapBuffers(eglDisplay, eglSurface);
	}
	
    ::DestroyWindow(hwnd);
    ::UnregisterClass(wc.lpszClassName, wc.hInstance);
    
    return 0;
}
