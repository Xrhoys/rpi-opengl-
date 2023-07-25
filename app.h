/* date = July 12th 2023 11:15 pm */

#ifndef APP_H
#define APP_H

#include "platform.h"
#include "math.h"
#include "video_decode.h"
#include "asset_build.h"
#include "renderer.h"
#include "utils.h"

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
	
	// TODO(Ecy): having referenecs in pointer is very dangerous in case 
	// - it gets unpaged by the OS 
	// - cleared by other parts of the program
	// The suggestion is to have an HASH/ID based system to find in a bucket, the linked nodes
	ui_node *parent;
	
	// NOTE(Ecy): this is a fixed size cuz i can't be bothered to allocate memory for that yet, 
	// since we don't have custom allocators.
	// TODO(Ecy);
	ui_node *child[5]; 
	u32     childCount;
	
	// UI Node properties
	r32 width;
	r32 height;
	color background;
	
	// NOTE(Ecy): relative to parent position
	r32 top;
	r32 left;
	
	char title[50];
	u32 titleSize;
};

struct app_ui
{
	// NOTE(Ecy): internal un-ordered ui_nodes
	ui_node _nodes[MAX_UI_NODE_COUNT];
	
	// NOTE(Ecy): the list that actually gets loop-ed through, and sorted for Z
	ui_node *nodes[MAX_UI_NODE_COUNT];
	
	u32 nodeCount;
	
	// NOTE(Ecy): UI building context
	ui_node *currentContextNode;
};

inline void
AddNode(ui_node *parent, ui_node *current)
{
	//Assert(parent->childCount < 5);
	
	current->parent = parent;
	parent->child[parent->childCount++] = current;
}

inline ui_node*
NewNode(app_ui *currentUI)
{
	ui_node *node = &currentUI->_nodes[currentUI->nodeCount++];
	
	// NOTE(Ecy): intent is to ZeroMemory(), platform independant. Since the UI tree is rebuilt every frame. 
	// But not sure what's the generated ASM in CL/GCC 
	*node = {};
	
	return node;
}

struct font_engine
{
	b32 isLoaded;
	asset_font asset;
	
	u32 textureId;
};

#endif //APP_H
