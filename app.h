/* date = July 12th 2023 11:15 pm */

#ifndef APP_H
#define APP_H

#include "platform.h"
#include "video_decode.h"
#include "asset_build.h"
#include "renderer.h"

#define Kilobytes(n) n * 1024
#define Megabytes(n) n * 1024 * 1024
#define Gigabytes(n) n * 1024 * 1024 * 1024

#define MAX_UI_NODE_COUNT 1000

enum ui_node_type 
{
	UI_NODE_NONE,
	
	UI_NODE_FRAME,
	UI_NODE_BUTTON,
	UI_NODE_TEXT,
		
	UI_NODE_TYPE_COUNT,
};

char *uiLabels[UI_NODE_TYPE_COUNT] =
{
	"none",
	
	"frame",
	"button",
	"text",
};

struct ui_node
{
	ui_node_type type;
	
	ui_node *parent;
	
	// NOTE(Ecy): this is a fixed size cuz i can't be bothered to allocate memory for that yet, 
	// since we don't have custom allocators.
	// TODO(Ecy);
	ui_node *child[5]; 
	u32     childCount;
	
	// UI Node properties
	u32 width;
	u32 height;
	color background;
	
	// NOTE(Ecy): relative to parent position
	i32 top;
	i32 left;	
};

struct app_ui
{
	// NOTE(Ecy): internal un-ordered ui_nodes
	ui_node _nodes[MAX_UI_NODE_COUNT];
	
	// NOTE(Ecy): the list that actually gets loop-ed through, and sorted for Z
	ui_node *nodes[MAX_UI_NODE_COUNT];
	
	u32 nodeCount;
};

inline void
AddNode(ui_node *parent, ui_node *current)
{
	current->parent = parent;
	parent->child[parent->childCount++] = current;
}

inline ui_node*
NewNode(app_ui *currentUI)
{
	ui_node *node = &currentUI->_nodes[currentUI->nodeCount++];
	
	*node = {};
	
	return node;
}

struct font_engine
{
	b32 isLoaded;
	asset_font asset;

	u32 textureId;
};

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

#endif //APP_H
