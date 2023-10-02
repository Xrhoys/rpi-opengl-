/* date = July 15th 2023 0:59 am */

#ifndef ASSET_BUILD_H
#define ASSET_BUILD_H

#define MAX_ASCII_SUPPORTED_GLYPH 0x5E
#define FONT_BASE_OFFSET ' '

struct asset_font_glyph
{
	char glyph;
	u32 _offset;
	u32 width;
	u32 height;
	i32 xoffset;
	i32 yoffset;

	/*
		A--B  A(left, top)                                     
		|\ |  B(right, top)              
		| \|  C(right, bottom)     
		D--C  D(left, bottom)             
	*/
	r32 u;
	r32 v;
	r32 ratio;
};

struct asset_font
{
	asset_font_glyph glyphs[MAX_ASCII_SUPPORTED_GLYPH];

	u32 height;	
	u32 width;
};

#if 0
struct INPUT_BUFFER
{
    u8  *buffer;
    u64 size;
};

static int read_callback(int64_t offset, void *buffer, u64 size, void *token)
{
    INPUT_BUFFER *buf = (INPUT_BUFFER*)token;
    size_t to_copy = MINIMP4_MIN(size, buf->size - offset - size);
    memcpy(buffer, buf->buffer + offset, to_copy);
    return to_copy != size;
}
#endif

#endif //ASSET_BUILD_H
