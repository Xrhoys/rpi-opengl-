/* date = June 3rd 2023 7:11 pm */

#ifndef MAIN_H
#define MAIN_H

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 800
#define REFRESH_RATE 120

#define internal static
#define global   static

internal DEBUG_CLOCK_GET_TIME(Win32GetLastElapsed);
internal DEBUG_PLATFORM_FREE_FILE_MEMORY(Win32FreeFile);
internal DEBUG_PLATFORM_READ_ENTIRE_FILE(Win32ReadEntireFile);
internal DEBUG_PLATFORM_WRITE_ENTIRE_FILE(Win32WriteEntireFile);


#endif //MAIN_H
