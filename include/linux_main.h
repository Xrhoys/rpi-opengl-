#ifndef LINUX_MAIN_H
#define LINUX_MAIN_H

#include <EGL/egl.h>
#include <EGL/eglplatform.h>
#include <GL/gl.h>
#include <GLES2/gl2.h>
#include <GLES3/gl3.h>

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 800

#define GL_ES_VERSION_3_0 1

inline char* 
getErrorStr(EGLint code)
{
	switch(code)
	{
		case EGL_SUCCESS: return "No error";
		case EGL_NOT_INITIALIZED: return "EGL not initialized or failed to initialize";
		case EGL_BAD_ACCESS: return "Resource inaccessible";
		case EGL_BAD_ALLOC: return "Cannot allocate resources";
		case EGL_BAD_ATTRIBUTE: return "Unrecognized attribute or attribute value";
		case EGL_BAD_CONTEXT: return "Invalid EGL context";
		case EGL_BAD_CONFIG: return "Invalid EGL frame buffer configuration";
		case EGL_BAD_CURRENT_SURFACE: return "Current surface is no longer valid";
		case EGL_BAD_DISPLAY: return "Invalid EGL display";
		case EGL_BAD_SURFACE: return "Invalid surface";
		case EGL_BAD_MATCH: return "Inconsistent arguments";
		case EGL_BAD_PARAMETER: return "Invalid argument";
		case EGL_BAD_NATIVE_PIXMAP: return "Invalid native pixmap";
		case EGL_BAD_NATIVE_WINDOW: return "Invalid native window";
		case EGL_CONTEXT_LOST: return "Context lost";
		default: return "";
	}
}

#endif