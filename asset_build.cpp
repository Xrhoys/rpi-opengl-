#include "platform.h"

#include <stdio.h>
#include <stdlib.h>

#define STB_TRUETYPE_IMPLEMENTATION 1
#include "stb_truetype.h"

#include "utils.h"
#include "video_decode.h"

#define MINIMP4_IMPLEMENTATION
#define MP4D_HEVC_SUPPORTED 1
#include "minimp4.h"

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

static void
ParseDemuxMP4(u8 **start, u8 *end)
{
	u8 *cursor = *start;
	
	demux_mp4_box_header header = {};
	ParseDemuxMP4Header(&header, cursor);
	
	switch(header.type)
	{
		case DEMUX_MP4_BOX_MOOV:
		{
			u8 *subCursor = cursor + sizeof(u32) * 2;
			u8 *subEnd = cursor + header.size;
			while(subCursor < subEnd)
			{
				ParseDemuxMP4(&subCursor, subEnd);
			}
		}break;
		
		case DEMUX_MP4_BOX_MVHD:
		{
			int b = 0;
		}
		
		case DEMUX_MP4_BOX_TRAK:
		{
			u8 *subCursor = cursor + sizeof(u32) * 2;
			u8 *subEnd = cursor + header.size;
			while(subCursor < subEnd)
			{
				ParseDemuxMP4(&subCursor, subEnd);
			}
		}break;
		
		case DEMUX_MP4_BOX_TKHD:
		{
			int b = 0;
		}break;
		
		case DEMUX_MP4_BOX_EDTS:
		{
			int b = 0;
		}break;
		
		case DEMUX_MP4_BOX_MDIA:
		{
			u8 *subCursor = cursor + sizeof(u32) * 2;
			u8 *subEnd = cursor + header.size;
			while(subCursor < subEnd)
			{
				ParseDemuxMP4(&subCursor, subEnd);
			}
		}break;
		
		case DEMUX_MP4_BOX_MDHD:
		{
			int b = 0;
		}break;
		
		case DEMUX_MP4_BOX_HDLR:
		{
			int b = 0;
		}break;
		
		case DEMUX_MP4_BOX_MINF:
		{
			u8 *subCursor = cursor + sizeof(u32) * 2;
			u8 *subEnd = cursor + header.size;
			while(subCursor < subEnd)
			{
				ParseDemuxMP4(&subCursor, subEnd);
			}
		}break;
		
		case DEMUX_MP4_BOX_VMHD:
		{
			int b = 0;
		}break;
		
		case DEMUX_MP4_BOX_STBL:
		{
			u8 *subCursor = cursor + sizeof(u32) * 2;
			u8 *subEnd = cursor + header.size;
			while(subCursor < subEnd)
			{
				ParseDemuxMP4(&subCursor, subEnd);
			}
		}break;
		
		// The rest inside stbl
		case DEMUX_MP4_BOX_STSD:
		{
			u8 *subCursor = cursor;
			
			demux_mp4_box_full_header fullHeader;
			u32 offset = ParseDemuxMP4HeaderFull(&fullHeader, cursor);
			subCursor += offset;			
			
			u8 *subEnd = cursor + header.size;
			
			u32 entry_size = _byteSwapU32(*(u32*)cursor);
			subCursor += sizeof(u32);
			
			for(u32 index = 0;
				index < entry_size;
				++index)
			{
				ParseDemuxMP4(&subCursor, subEnd);
			}
		}break;
		
		case DEMUX_MP4_BOX_HEV1:
		{
			demux_mp4_box_hev1 videoBox = {};
			
			// NOTE(Ecy): VisualSampleEntry size + header
			u8 *subCursor = cursor + 2 * sizeof(u32) + 8 + 38 + sizeof(char) * 32;
			u8 *subEnd = cursor + header.size;
#if 0			
			subCursor += 16;
			videoBox.width = *(i16*)subCursor;
			videoBox.height = 0;
			videoBox.horizresolution = 0;
			videoBox.vertresolution = 0;
			videoBox.frameCount = 0;
			videoBox.depth = 0;
			
			memcpy(&videoBox.compressorname, 0, sizeof(videoBox.compressorname));
#endif
			
			while(subCursor < subEnd)
			{
				ParseDemuxMP4(&subCursor, subEnd);
			}
		}break;
		
		case DEMUX_MP4_BOX_HVCC:
		{
			u8 *subCursor = cursor + 2 * sizeof(u32);
			u8 *subEnd = cursor + header.size;
			
			demux_mp4_box_hvcc box = {};
			box.header = header;
			
			box.configurationVersion = *subCursor++;
			
			u8 general = *subCursor++;
			
			box.general_profile_space = general >> 6;
			box.general_tier_flag = (general & 0x4) >> 4;
			box.general_profile_idc = general & 0x1f;
			
			box.general_profile_compatibility_flags = (i32)_byteSwapU32(*(u32*)subCursor);
			subCursor += sizeof(box.general_profile_compatibility_flags);
			
			memcpy(&box.general_constraint_indicator_flags, subCursor, sizeof(box.general_constraint_indicator_flags));
			subCursor += sizeof(box.general_constraint_indicator_flags);
			
			box.general_level_idc = *subCursor++;
			
			box.min_spatial_segmentation_idc = (i16)_byteSwapU16(*(u16*)subCursor) & 0xfff; // 12 bits
			subCursor += sizeof(box.min_spatial_segmentation_idc);
			
			box.parallelismType = (*subCursor++) & 0x3;
			box.chromaFormat = (*subCursor++) & 0x3;
			box.bitDepthLumaMinus8 = (*subCursor++) & 0x7;
			box.bitDepthChromaMinus8 = (*subCursor++) & 0x7;
			
			box.avgFrameRate = _byteSwapU16(*(u16*)subCursor);
			subCursor += sizeof(box.avgFrameRate);
			
			u8 data = *subCursor++;
			
			box.constantFrameRate  = data >> 6;
			box.numTemporalLayers  = (data & 0x38) >> 3 ;
			box.temporalIdNested   = (data & 0x4) >> 2;
			box.lengthSizeMinusOne = data & 0x3;
			
			box.numOfArrays = *subCursor++;
			
			for(u32 index = 0;
				index < box.box.numOfArrays;
				++index)
			{
				bit(1) array_completeness;
				unsigned int(1) reserved = 0;
				unsigned int(6) NAL_unit_type;
				unsigned int(16) numNalus;
				
				for(u32 nalIndex = 0;
					nalIndex < numNalus;
					++nalIndex)
				{
					unsigned int(16) nalUnitLength;
					bit(8*nalUnitLength) nalUnit;
				}
			}
		}break;
		
		case DEMUX_MP4_BOX_UDTA:
		{
			int b = 0;
		}break;
		
		case DEMUX_MP4_BOX_FTYP:
		{
			int b = 0;
		}break;
		
		case DEMUX_MP4_BOX_MDAT:
		{
			int b = 0;
		}break;
		
		default:
		{
			int b = 0;
		}break;
	}
	
	switch(header.size)
	{
		case 0:
		{
			// NOTE(Ecy): last box, size extends until the end of the file
			*start = end;
		}break;
		
		case 1:
		{
			*start += _byteSwapU64(*((u64*)start));
		}break;
		
		default: 
		{
			*start += header.size;
		}break;
	}

}

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
	
	{
		debug_read_file_result file = ReadEntireFile(NULL, "data/sample.mp4");
		
		u8 *cursor = (u8*)file.contents;
		u8 *end = (u8*)file.contents + file.contentSize;
		
		while(cursor < end)
		{
			ParseDemuxMP4(&cursor, end);
		}
		
	}
}

