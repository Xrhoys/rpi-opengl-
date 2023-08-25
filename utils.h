/* date = July 24th 2023 0:18 am */

#ifndef UTILS_H
#define UTILS_H

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

#if 0
inline u32
_byteSwapU32(u32 b)
{
	return _byteswap_ulong(b);
}

inline u64
_byteSwapU64(u64 b)
{
	return _byteswap_uint64(b);
}
#endif

#endif //UTILS_H
