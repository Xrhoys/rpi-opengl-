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
	
	// TODO(Ecy): finish the input layer with all keys on a 100% keyboard, fuck 108
	enum KEY_LABELS
	{
		KEY_W,
		KEY_A,
		KEY_S,
		KEY_D,
		KEY_F1,
		KEY_F2,
		KEY_F3,
		KEY_F4,
		KEY_F5,
		KEY_F6,
		KEY_SPACE,
		KEY_ESC,
		
		KEY_ALT,
		KEY_CTRL,
		KEY_SHIFT,
		
		KEY_COUNT,
	};
	
	char *keyLabels[KEY_COUNT] =
	{
		"w",
		"a",
		"s",
		"d",
		"f1",
		"f2",
		"f3",
		"f4",
		"f5",
		"f6",
		"space",
		"esc",
		"alt",
		"ctrl",
		"shift",
	};
	
	enum MOUSE_BUTTONS
	{
		MOUSE_LEFT,
		MOUSE_RIGHT,
		MOUSE_MIDDLE,
		MOUSE_WHEEL,
		MOUSE_BACKWARD,
		MOUSE_FORWARD,
		// NOTE(Ecy): support horizontal scrolling on mice like G502
		MOUSE_SCROLL_LEFT,
		MOUSE_SCROLL_RIGHT,
		
		MOUSE_BUTTON_COUNT,
	};
	
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
	
	typedef struct app_input_state
	{
		u32 halfTransitionCount;
		b32 endedDown;
		
		// NOTE(Ecy): timestamp from which the button started to be DOWN
		r32 startHoldTime;
	} app_input_state;
	
	typedef struct app_keyboard_input
	{
		b32 isConnected;
		
		app_input_state keys[KEY_COUNT];
	} app_input;
	
	typedef struct app_pointer_input
	{
		b32 isConnected;
		
		r32 mouseX, mouseY, mouseZ, sensX, sensY, sensZ;
		
		b32 isTouchScreen;
		b32 concurrentTouchCount;
		
		app_input_state buttons[MOUSE_BUTTON_COUNT];
	} app_pointer_input;
	
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
		
		// TODO(Ecy): should we support multiple input devices at the same time?
		app_keyboard_input *keyboards[4];
		app_pointer_input  *pointers[4];
	} app_state;
	
#ifdef __cplusplus
}
#endif

#endif //PLATFORM_H
