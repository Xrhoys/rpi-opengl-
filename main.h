/* date = June 3rd 2023 7:11 pm */

#ifndef MAIN_H
#define MAIN_H

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 800
#define REFRESH_RATE 120

#define STB_TRUETYPE_IMPLEMENTATION 1
#include "stb_truetype.h"

struct win32_context
{
	u32 width;
	u32 height;
};

struct win32_timer
{
		r32 frameStart;
		r32 elapsed;
};

#endif //MAIN_H
