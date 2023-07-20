/* date = July 12th 2023 11:15 pm */

#ifndef APP_H
#define APP_H

#include "platform.h"
#include "video_decode.h"
#include "asset_build.h"

#define MAX_UI_NODE_COUNT 1000

// Tree structures
/*
Node:
- 1 parent
 - Children
*/
struct ui_node
{
	ui_node *parent;
	
	// NOTE(Ecy): this is a fixed size cuz i can't be bothered to allocate memory for that yet, 
	// since we don't have custom allocators.
	// TODO(Ecy);
	ui_node *child[5]; 
	u32     childCount;
	
	// UI Node properties
	u32 width;
	u32 height;
	
	// NOTE(Ecy): relative to parent position
	u32 top;
	u32 left;
	
	r32 color[4];
};

struct app_ui
{
	ui_node nodes[MAX_UI_NODE_COUNT];
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
	return &currentUI->nodes[currentUI->nodeCount++];
}

struct font_engine
{
	b32 isLoaded;
	asset_font asset;

	u32 textureId;
};

#endif //APP_H
