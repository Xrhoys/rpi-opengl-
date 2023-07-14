/* date = July 9th 2023 11:53 pm */

#ifndef PLATFORM_H
#define PLATFORM_H

#ifdef __cplusplus
extern "C" {
#endif
#define Assert(expression) if(!(expression)) { *(int *)0 = 0; }
#include <stdint.h>

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
	
	typedef struct thread_context
	{
		int placeHolder;
	} thread_context;
	
	typedef struct debug_read_file_result
	{
		void     *contents;
		u32      contentSize;
	} debug_read_file_result;
	
#define DEBUG_PLATFORM_FREE_FILE_MEMORY(name) void name(thread_context *thread, void *memory)
	typedef DEBUG_PLATFORM_FREE_FILE_MEMORY(debug_platform_free_file_memory);
#define DEBUG_PLATFORM_READ_ENTIRE_FILE(name) debug_read_file_result name(thread_context *thread, char *filename)
	typedef DEBUG_PLATFORM_READ_ENTIRE_FILE(debug_platform_read_entire_file);
#define DEBUG_PLATFORM_WRITE_ENTIRE_FILE(name) b32 name(thread_context *thread, char *filename, u32 memorySize, void* memory)
	typedef DEBUG_PLATFORM_WRITE_ENTIRE_FILE(debug_platform_write_entire_file);
	
#define DEBUG_CLOCK_GET_TIME(name) r32 name(void)
	typedef DEBUG_CLOCK_GET_TIME(debug_clock_get_time);
	
	typedef struct app_state
	{
		b32       running;
		b32       isInitialized;
		
		u32       width, height;
		
		u64       permanentStorageSize;
		void*     permanentStorage;
		
		u64       transientStorageSize;
		void*     transientStorage;
		
		debug_platform_read_entire_file  *DEBUGPlatformReadEntireFile;
		debug_platform_write_entire_file *DEBUGPlatformWriteEntireFile;
		debug_platform_free_file_memory  *DEBUGPlatformFreeFileMemory;
		
		r64       clock;
		r64       frameTime;
		
		debug_clock_get_time             *getTime; // Get current time
	} app_state;

#ifdef __cplusplus
}
#endif

#endif //PLATFORM_H
