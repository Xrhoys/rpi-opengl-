#include <stdio.h>

#define STB_TRUETYPE_IMPLEMENTATION 1
#include "stb_truetype.h"

struct asset_font
{
	char symbol;
	
};

int main()
{
	// FONT building
	{
		stbtt_fontinfo font;
		stbtt_InitFont(&font, (u8*)ttfFile.contents, stbtt_GetFontOffsetForIndex((u8*)ttfFile.contents, 0));
		
		// NOTE(Ecy): from 0x20 to 0x7E
		for(char index = ' ';
			index < '~';
			++index)
		{
			i32 width, height, xoff, yoff;
			u8 *bitmap = stbtt_GetCodepointBitmap(&font, 0, stbtt_ScaleForPixelHeight(&font, 10.0f), 
												  index, &width, &height, &xoff, &yoff);
			
		}
		
		
	}
}