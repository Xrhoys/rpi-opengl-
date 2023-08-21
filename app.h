/* date = July 12th 2023 11:15 pm */

#ifndef APP_H
#define APP_H

#include "platform.h"
#include "math.h"
#include "video_decode.h"
#include "asset_build.h"
#include "renderer.h"
#include "renderer_vulkan.h"
#include "utils.h"

#define Kilobytes(n) n * 1024
#define Megabytes(n) n * 1024 * 1024
#define Gigabytes(n) n * 1024 * 1024 * 1024

#define MAX_UI_NODE_COUNT 1000

enum Axis2
{
	Axis2_X,
	Axis2_Y,
	
	Axis2_COUNT
};

enum ui_sizeKind
{
	UI_SizeKind_Null,
	UI_SizeKind_Pixels,
	UI_SizeKind_TextContent,
	UI_SizeKind_PercentOfParent,
	UI_SizeKind_ChildrenSum,
};

enum ui_boxFlags
{
	UI_WidgetFlag_Clickable       = (1<<0),
	UI_WidgetFlag_ViewScroll      = (1<<1),
	UI_WidgetFlag_DrawText        = (1<<2),
	UI_WidgetFlag_DrawBorder      = (1<<3),
	UI_WidgetFlag_DrawBackground  = (1<<4),
	UI_WidgetFlag_DrawDropShadow  = (1<<5),
	UI_WidgetFlag_Clip            = (1<<6),
	UI_WidgetFlag_HotAnimation    = (1<<7),
	UI_WidgetFlag_ActiveAnimation = (1<<8),
};

struct ui_key
{
	char *title;
	char *hash;
};

struct ui_size
{
	ui_sizeKind kind;
	r32         value;
	r32         strictness;
};

/*
TODO(Ecy): the goal would be to implement an API like this
// basic key type helpers
UI_Key UI_KeyNull(void);
UI_Key UI_KeyFromString(String8 string);
B32 UI_KeyMatch(UI_Key a, UI_Key b);

// construct a widget, looking up from the cache if
// possible, and pushing it as a new child of the
// active parent.
UI_Widget *UI_WidgetMake(UI_WidgetFlags flags, String8 string);
UI_Widget *UI_WidgetMakeF(UI_WidgetFlags flags, char *fmt, ...);

// some other possible building parameterizations
void UI_WidgetEquipDisplayString(UI_Widget *widget,
                                 String8 string);
void UI_WidgetEquipChildLayoutAxis(UI_Widget *widget,
                                   Axis2 axis);

// managing the parent stack
UI_Widget *UI_PushParent(UI_Widget *widget);
UI_Widget *UI_PopParent(void);
 */

struct ui_box
{
	// tree links
	ui_box *first;
	ui_box *last;
	ui_box *next;
	ui_box *prev;
	ui_box *parent;
	
	// hash links
	ui_box *hash_next;
	ui_box *hash_prev;
	
	// key+generation info
	ui_key key;
	u64 lastFrameTouchedIndex;
	
	// per-frame info provided by builders
	ui_boxFlags flags;
	char *string;
	u32  stringSize;
	ui_size semanticSize[Axis2_COUNT];
	
	// computed every frame
	r32 computedRelPosition[Axis2_COUNT];
	r32 computedSize[Axis2_COUNT];
	//Rng2F32 rect;
	
	// persistent data
	r32 hotT;
	r32 activeT;
};

struct ui_comm
{
	ui_box *widget;
	v2 mouse;
	v2 drag_delta;
	u8 clicked;
	u8 doubleClicked;
	u8 rightClicked;
	u8 pressed;
	u8 released;
	u8 dragging;
	u8 hovering;
};

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
	u32 width;
	u32 height;
	color *background;
	
	// NOTE(Ecy): relative to parent position
	u32 top;
	u32 left;
	
	u32 clickedAtX, clickedAtY;
	
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

// NOTE(Ecy): problems with overlapped windwow, it has to come from state that lives cross frames
// and not within the life cycle of a single frame
inline b32
IsUiHovered(app_pointer_input *pointer, app_ui *ui)
{
	ui_node *node = ui->currentContextNode;
	
	return (pointer->posX <= node->left + node->width) 
		&& (pointer->posX >= node->left)
		&& (pointer->posY <= node->top + node->height)
		&& (pointer->posY >= node->top);
}

inline b32
IsUiClicked(app_pointer_input *pointer, app_ui *ui)
{
	ui_node *node = ui->currentContextNode;
	
	return (pointer->posX <= node->left + node->width) 
		&& (pointer->posX >= node->left)
		&& (pointer->posY <= node->top + node->height)
		&& (pointer->posY >= node->top)
		&& pointer->buttons[MOUSE_LEFT].endedDown;
}

#endif //APP_H
