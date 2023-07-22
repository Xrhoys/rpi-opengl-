#include "platform.h"

#include <stdio.h>
#include <stdlib.h>

#define STB_TRUETYPE_IMPLEMENTATION 1
#include "stb_truetype.h"

#include "asset_build.h"

#if LINUX
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

static
DEBUG_PLATFORM_FREE_FILE_MEMORY(FreeFile)
{
	free(memory);
}

static 
DEBUG_PLATFORM_WRITE_ENTIRE_FILE(WriteEntireFile)
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
DEBUG_PLATFORM_READ_ENTIRE_FILE(ReadEntireFile)
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

#elif WIN32

#include <windows.h>

static
DEBUG_PLATFORM_FREE_FILE_MEMORY(FreeFile)
{
	VirtualFree(memory, 0, MEM_RELEASE);
}

static
DEBUG_PLATFORM_READ_ENTIRE_FILE(ReadEntireFile)
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

static
DEBUG_PLATFORM_WRITE_ENTIRE_FILE(WriteEntireFile)
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

#endif

int main()
{
	// FONT map
	{
		debug_read_file_result ttfFile = ReadEntireFile(NULL, "data/arial.ttf");
		
		stbtt_fontinfo font;
		stbtt_InitFont(&font, (u8*)ttfFile.contents, stbtt_GetFontOffsetForIndex((u8*)ttfFile.contents, 0));		

		// NOTE(Ecy): allocate enough memory for output buffer
		u8 *writeBuffer = (u8*)malloc(1024*1024);

		u8 *cursor = writeBuffer;

		u32 maxHeight = 0;
		u32 totalWidth = 0;
		// NOTE(Ecy): from 0x20 to 0x7E
		// The strategy is to line all the glyphs horizontally and supply (u,v) cooridnates in a seperate structure
		// This could potentially have float coordinate precision issues
		asset_font fontData = {};
		for(char index = FONT_BASE_OFFSET;
			index < '~';
			++index)
		{
			i32 width, height, xoff, yoff;
			u8 *bitmap = stbtt_GetCodepointBitmap(&font, 0, stbtt_ScaleForPixelHeight(&font, 120.0f), 
												  index, &width, &height, &xoff, &yoff);

			memcpy(cursor, bitmap, width * height);
			
			asset_font_glyph *currentGlyph = &fontData.glyphs[index - FONT_BASE_OFFSET];
			currentGlyph->glyph   = index;
			currentGlyph->_offset  = cursor - writeBuffer;
			currentGlyph->width   = width;
			currentGlyph->height  = height;
			currentGlyph->ratio   = (r32)width / (r32)height;
			currentGlyph->xoffset = xoff;
			currentGlyph->yoffset = yoff;

			totalWidth += width;
			if(height > maxHeight) maxHeight = height;
			
			cursor += width * height;
			
			stbtt_FreeBitmap(bitmap, 0);
		}
		
		fontData.width = totalWidth;
		fontData.height = maxHeight;

		u32 currentLineWidth = 0;
		r32 bitmapWidth = (r32)totalWidth;
		r32 bitmapHeight = (r32)maxHeight;
		for(u32 index = 0;
			index < MAX_ASCII_SUPPORTED_GLYPH;
			++index)
		{
			asset_font_glyph *currentGlyph = &fontData.glyphs[index];

			r32 offsetWidth = (r32)currentLineWidth;

			currentGlyph->u = offsetWidth / bitmapWidth;
			currentGlyph->v = (r32)currentGlyph->height / bitmapHeight;

			currentLineWidth += currentGlyph->width;
		}

		u8 *pixelMapBuffer = (u8*)malloc(10*1024*1024);
		memcpy(pixelMapBuffer, &fontData, sizeof(asset_font));

		cursor = pixelMapBuffer + sizeof(asset_font);
		
		for(u32 line = 0;
			line < maxHeight;
			++line)
		{
			for(u32 index = 0;
				index < MAX_ASCII_SUPPORTED_GLYPH;
				++index)
			{
				asset_font_glyph *currentGlyph = &fontData.glyphs[index];
				if (line >= fontData.glyphs[index].height)
				{
					cursor += currentGlyph->width * 4;
				}
				else
				{
					u8 *writeBufferOffset = writeBuffer + currentGlyph->_offset + currentGlyph->width * line;
					
					u32 *dest = (u32*)cursor;
					for(u32 pixelIndex = 0;
						pixelIndex < currentGlyph->width;
						++pixelIndex)
					{
						u8 pixel = writeBufferOffset[pixelIndex];
						*dest++ = ((pixel << 24) |
								   (pixel << 16) |
								   (pixel <<  8) |
								   (pixel <<  0) );
						cursor += 4;
					}

				}

			}
		}

		WriteEntireFile(NULL, "asset_data", fontData.width * fontData.height * 4 + sizeof(asset_font), pixelMapBuffer);
	}
}
