#ifndef LINUX_MAIN_H
#define LINUX_MAIN_H

#include <stdint.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint32_t b32;

typedef float r32;
typedef double r64;

#define internal static
#define global   static

#include <EGL/egl.h>
#include <EGL/eglplatform.h>
#include <GL/gl.h>
#include <GLES2/gl2.h>
#include <GLES3/gl3.h>

#include "stb_image.h"
#define STB_IMAGE_IMPLEMENTATION

#define FONT_FILE "font.jpg"

#define GL_ES_VERSION_3_0 1

struct timer
{
	uint32_t lastCounter;
	uint32_t currentCounter;	
};

inline u32 
GetTicks()
{
  u64 ticks = 0;
  u32 a, d;
  asm volatile("rdtsc" : "=a" (a), "=d" (d));

  return a;
}
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

inline void 
InitFont(GLuint *texture)
{
	int x;
	int y;
	int channels_in_file;

	stbi_uc *fontFile = stbi_load(FONT_FILE, &x, &y, &channels_in_file, 4);

	glGenTextures(1, texture);
	glBindTexture(GL_TEXTURE_2D, *texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, x, y, 0, GL_RGB, GL_UNSIGNED_BYTE, fontFile);
	glGenerateMipmap(GL_TEXTURE_2D);
}

#endif