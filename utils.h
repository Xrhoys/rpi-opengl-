/* date = July 24th 2023 0:18 am */

#ifndef UTILS_H
#define UTILS_H

#if 0
#include <byteswap.h>
#endif

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
_byteSwapU16(u16 b)
{
	return _byteswap_ushort(b);
	//return bswap_16(b);
}

inline u32
_byteSwapU32(u32 b)
{
	return _byteswap_ulong(b);
	//return bswap_32(b);
}

inline u64
_byteSwapU64(u64 b)
{
	return _byteswap_uint64(b);
	//return bswap_64(b);
}

#endif //UTILS_H
