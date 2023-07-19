#include "platform.h"

#include <stdio.h>
#include <stdlib.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#define STB_TRUETYPE_IMPLEMENTATION 1
#include "stb_truetype.h"

#include "asset_build.h"

static
DEBUG_PLATFORM_FREE_FILE_MEMORY(LinuxFreeFile)
{
	free(memory);
}

static 
DEBUG_PLATFORM_WRITE_ENTIRE_FILE(LinuxWriteEntireFile)
{
	u32 flags = O_WRONLY | O_CREAT | O_TRUNC;
	u32 permissionFlags = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
	i32 fd = open(filename, flags, permissionFlags);

	if(fd == -1) 
	{
		// TODO(Ecy): handle file open error
		return false;
	}

	ssize_t bytesWritten = write(fd, memory, memorySize);
	if(bytesWritten == -1)
	{
		close(fd);
		// TODO(Ecy): handle file write error
		return false;
	}
	
	close(fd);

	return true;
}

static 
DEBUG_PLATFORM_READ_ENTIRE_FILE(LinuxReadEntireFile)
{
	debug_read_file_result file;

	i32 fd = open(filename, O_RDONLY);

	if(fd == -1) 
	{
		// TODO(Ecy): handle file open error
	}

	struct stat st;
	stat(filename, &st);

	// TODO(Ecy): investigate on the off_t size, this could vary per machine
	off_t size = st.st_size;

	u8 *buffer;
	ssize_t bytesRead;
	if(size > 0) 
	{
		buffer = (u8*)malloc(size);
		bytesRead = read(fd, buffer, size);

		if(bytesRead != size)
		{
			// TODO(Ecy): handle file read error
		}
	}

	file.contents    = buffer;
	file.contentSize = bytesRead;

	close(fd);

	return file;
}

int main()
{
	// FONT map
	{
		debug_read_file_result ttfFile = LinuxReadEntireFile(NULL, "data/arial.ttf");
		
		stbtt_fontinfo font;
		stbtt_InitFont(&font, (u8*)ttfFile.contents, stbtt_GetFontOffsetForIndex((u8*)ttfFile.contents, 0));
		

		// NOTE(Ecy): allocate enough memory for output buffer
		char *writeBuffer = (char*)malloc(1024*1024);

		char *cursor = writeBuffer;

		u32 baseOffset = (u32)' ';
		
		u32 maxHeight = 0;
		u32 totalWidth = 0;
		// NOTE(Ecy): from 0x20 to 0x7E
		// The strategy is to line all the glyphs horizontally and supply (u,v) cooridnates in a seperate structure
		// This could potentially have float coordinate precision issues
		asset_font fontData = {};
		for(char index = ' ';
			index < '~';
			++index)
		{
			i32 width, height, xoff, yoff;
			u8 *bitmap = stbtt_GetCodepointBitmap(&font, 0, stbtt_ScaleForPixelHeight(&font, 30.0f), 
												  index, &width, &height, &xoff, &yoff);

			memcpy(bitmap, cursor, width  * height);
			cursor += width * height;

			asset_font_glyph *currentGlyph = &fontData.glyphs[(u32)index - baseOffset];
			currentGlyph->glyph = index;
			currentGlyph->offset = cursor - writeBuffer;
			currentGlyph->width = width;
			currentGlyph->height = height;

			totalWidth += width;
			if(height > maxHeight) maxHeight = height;
		}

		fontData.width = totalWidth;
		fontData.height = maxHeight;

		u32 currentLineWidth = 0;
		r32 bitmapWidth = (r32)totalWidth;
		r32 bitmapHeight = (r32)maxHeight;
		for(u32 index = 0;
			index < 0x5E;
			++index)
		{
			asset_font_glyph *currentGlyph = &fontData.glyphs[index];

			u32 offsetWidth = currentGlyph->width + currentLineWidth;

			currentGlyph->xoffset = (r32)offsetWidth / bitmapWidth;
			currentGlyph->yoffset = (r32)currentGlyph->height / bitmapHeight;

			currentLineWidth += currentGlyph->width;
		}

		char *pixelMapBuffer = (char*)malloc(100*1024*1024);
		memcpy(&fontData, pixelMapBuffer, sizeof(asset_font));

		cursor = pixelMapBuffer + sizeof(asset_font);
		
		for(u32 line = 0;
			line < maxHeight;
			++line)
		{
			for(u32 index = 0;
				index < 0x5E;
				++index)
			{
				asset_font_glyph *currentGlyph = &fontData.glyphs[index];
				if (line > fontData.glyphs[index].height)
				{
					cursor += currentGlyph->width;
				}
				else
				{
					char *writeBufferOffset = writeBuffer;
					writeBufferOffset += currentGlyph->offset + currentGlyph->width * line;

					memcpy(writeBufferOffset, cursor, currentGlyph->width);

					cursor += currentGlyph->width;
				}

			}
		}

		printf("Font data processed, start writing to file");
		
		LinuxWriteEntireFile(NULL, "asset_data", sizeof(asset_font) + fontData.width * fontData.height, pixelMapBuffer);
	}
}
