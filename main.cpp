#include "app.h"

#include <windows.h>
#include <stdio.h>
#include <tchar.h>

#include "main.h"

// TODO(Ecy): to exclude with proper platform code
#include "app.cpp"

static app_state g_state;

int CALLBACK
WinMain(HINSTANCE Instance,
        HINSTANCE PrevInstance,
        LPSTR CommandLine,
        int ShowCode)
{
    // Create application window
    //ImGui_ImplWin32_EnableDpiAwareness();
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, _T("RPI Emulation"), NULL };
    RegisterClassEx(&wc);
    HWND hwnd = CreateWindow(wc.lpszClassName, _T("RPI Emulation"), WS_OVERLAPPEDWINDOW, 100, 100, WINDOW_WIDTH, WINDOW_HEIGHT, NULL, NULL, wc.hInstance, NULL);
    
    // Show the window
    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);
	
	{
		lastCycleCount = __rdtsc();
		
		LARGE_INTEGER counter;
		LARGE_INTEGER perfCountFrequencyResult;
		QueryPerformanceCounter(&counter);
		QueryPerformanceFrequency(&perfCountFrequencyResult);
		
		g_bootCounter = counter;
		g_lastCounter = counter;
		g_perfCount = perfCountFrequencyResult.QuadPart;
	}
	
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
	
	InitRenderer();
	//InitFont();
	InitApp();
	
	g_state.running = true;
	
	// NOTE(Ecy): needs to set at runtime + dynamic with resize event
	g_state.width = WINDOW_WIDTH;
	g_state.height = WINDOW_HEIGHT;
	
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
				
				lastCycleCount      = endCycleCounter;
				g_lastCounter       = Win32GetWallClock();
			}
		}
	
    DestroyWindow(hwnd);
    UnregisterClass(wc.lpszClassName, wc.hInstance);
    
    return 0;
}
