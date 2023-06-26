/* date = June 3rd 2023 7:11 pm */

#ifndef MAIN_H
#define MAIN_H

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 800
#define REFRESH_RATE 120

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

typedef int8_t   i8;
typedef int16_t  i16;
typedef int32_t  i32;
typedef int64_t  i64;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float    r32;
typedef double   r64;

typedef int32_t  b32;

typedef struct debug_read_file_result
{
		void     *contents;
		   uint32_t contentSize;
} debug_read_file_result;

u64              g_perfCount;
LARGE_INTEGER    g_bootCounter;
LARGE_INTEGER    g_lastCounter;
u64              lastCycleCount;
r64              cyclesPerFrame;

typedef struct win32_timer
{
		r32 frameStart;
		r32 elapsed;
} win32_timer;

inline r32
Win32GetSecondsElapsed(LARGE_INTEGER start, LARGE_INTEGER end)
{
	// NOTE: should g_perfCount be refetched here?
	r64 endTime   = (r64)end.QuadPart;
    r64 startTime = (r64)start.QuadPart;
	r64 perfCount = (r64)g_perfCount;
	r32 result    = (r32)((endTime - startTime) / perfCount);
    
    return result;
}

inline LARGE_INTEGER
Win32GetWallClock()
{
    LARGE_INTEGER result;
    QueryPerformanceCounter(&result);
    return result;
}

inline r32
Win32GetLastElapsed(void)
{
    LARGE_INTEGER currentCounter = Win32GetWallClock();
	r32 elapsed = Win32GetSecondsElapsed(g_lastCounter, currentCounter);
    
    return elapsed;
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

// Win32 message handler
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

inline debug_read_file_result
Win32ReadEntireFile(char *filename)
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

inline b32
Win32WriteEntireFile(char *filename, uint32_t memorySize, void* memory)
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

inline b32
InitFont(GLuint *texture)
{
	debug_read_file_result file = Win32ReadEntireFile("font.jpg");
	
	uint32_t x;
	uint32_t y;
	uint32_t channelsInFile;
	uint32_t desiredChannels = 4;
	
	stbi_uc *image = stbi_load_from_memory((char*)file.contents, file.contentSize, &x, &y, &channelsInFile, desiredChannels);
	
	glGenTextures(1, texture);
	glBindTexture(GL_TEXTURE_2D, *texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, x, y, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
	glGenerateMipmap(GL_TEXTURE_2D);
	
}

#endif //MAIN_H
