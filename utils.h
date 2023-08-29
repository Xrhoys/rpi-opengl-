/* date = July 24th 2023 0:18 am */

#ifndef UTILS_H
#define UTILS_H

#include <byteswap.h>

// NOTE(Ecy): linear/bump allocator
struct memory_arena
{
	u8  *_start;
	u8  *cursor;
	
	u32  size;
};

struct memory_block
{
	u8 *_start;
	u8  _id;
	
	b32 isUsed;
	
	u32 size;
};

inline memory_arena 
CreateArenaMem(u8* memory, u32 size)
{
	memory_arena arena;
	
	arena._start = memory;
	arena.cursor = arena._start;
	arena.size = size;
	
	return arena;
}

inline u8*
LinearAlloc(memory_arena *arena, u32 size)
{
	Assert(arena->cursor + size < arena->_start + arena->size);
	
	u8 *returnCursor = arena->cursor;
	arena->cursor    += size;
	
	return returnCursor;
}

inline u32
_byteSwapU32(u32 b)
{
	//return _byteswap_ulong(b);
	return bswap_32(b);
}

inline u64
_byteSwapU64(u64 b)
{
	//return _byteswap_uint64(b);
	return bswap_64(b);
}

inline u32
ParseDemuxMP4Header(demux_mp4_box_header *header, u8 *data)
{
	u8 *cursor = data;
	header->size = _byteSwapU32(*cursor);
	cursor += sizeof(header->size);
	header->type = *((u32*)cursor);
	cursor += sizeof(header->type);

	if(header->size == 1)  
	{
		header->largesize = _byteSwapU64(*((u64*)cursor));
		cursor += sizeof(header->largesize);
	}
	else if(header->size == 0)
	{
		header->isLast = true;
	}

	if(header->type == *((u32*)"uuid"))
	{
		memcpy(header->userType, cursor, sizeof(header->userType));
		cursor += sizeof(header->userType);
	}

	return cursor - data;
}

inline u32
ParseDemuxMP4HeaderFull(demux_mp4_box_full_header *header, u8 *data)
{
	u8 *cursor = data;
	demux_mp4_box_header subHeader = {};
	cursor += ParseDemuxMP4Header(&subHeader, cursor);
	
	header->size = subHeader.size;
	header->type = subHeader.type;
	header->largesize = subHeader.largesize;
	
	memcpy(header->userType, subHeader.userType, sizeof(subHeader.userType));
	cursor += sizeof(subHeader.userType);

	header->isLast = subHeader.isLast;

	header->version = _byteSwapU32(*cursor);
	cursor += sizeof(header->version);
	
	memcpy(cursor, header->flags, sizeof(header->flags));
	cursor += sizeof(header->flags);

	return cursor - data;
}
#endif //UTILS_H
