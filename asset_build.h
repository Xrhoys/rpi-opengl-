/* date = July 15th 2023 0:59 am */

#ifndef ASSET_BUILD_H
#define ASSET_BUILD_H

#define MAX_ASCII_SUPPORTED_GLYPH 0x5E
#define FONT_BASE_OFFSET '!'

struct asset_font_glyph
{
	char glyph;
	u32 offset;
	u32 width;
	u32 height;	

	/*
		A--B  A(left, top)                                     
		|\ |  B(right, top)              
		| \|  C(right, bottom)     
		D--C  D(left, bottom)             
	*/
	r32 xoffset;
	r32 yoffset;
};

struct asset_font
{
	asset_font_glyph glyphs[MAX_ASCII_SUPPORTED_GLYPH];

	u32 height;	
	u32 width;
};

#endif //ASSET_BUILD_H
