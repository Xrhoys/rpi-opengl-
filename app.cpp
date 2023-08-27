#include "app.h"

#include "renderer.cpp"
#include "video_decode.cpp"

#include "video_decode_vulkan.cpp"

#define FONT_FILE "data/asset_data"
#define SAMPLE_DATA "data/sample.mp4"

// NOTE(Ecy): nothing of those should remain global => integrate into app state mega struct
global video_decode decoder;
global app_ui mainUi;
global memory_arena g_mainArena;
global color DEFAULT_TEXT_COLOR = WHITE;

// NOTE(Ecy): UI test globals
global v2U32 mainFramePos  = { 100, 100 };
global v2U32 mainFrameSize = { 200, 200 };

global v2U32 subFramePos  = { 5, 50 };
global v2U32 subFrameSize = { 100, 100 };

global v2U32 lastClickedPos = { 100, 100 };

/*
// The following describe an Immediate mode GUI API 
*/

// UI Frame element
internal b32
UIFrameBegin(char *title, u32 titleSize, v2U32 *pos, v2U32 *size, color *background)
{
	ui_node *node = NewNode(&mainUi);
	
	// NOTE(Ecy): this works like push and pop id
	if(mainUi.currentContextNode)
	{
		node->parent = mainUi.currentContextNode;
		//AddNode(node->parent, node);
		node->left = node->parent->left + pos->x;
		node->top = node->parent->top + pos->y;
	}
	else
	{
		node->left = pos->x;
		node->top  = pos->y;
	}
	
	mainUi.currentContextNode = node;
	
	node->type   = UI_NODE_FRAME;
	node->width  = size->width;
	node->height = size->height;
	node->background = background;
	
	ui_node *titleNode = NewNode(&mainUi);
	
	// NOTE(Ecy): those are RELATIVE positions
	titleNode->type   = UI_NODE_TEXT;
	// NOTE(Ecy): leaves 5 pixels from the edges
	// TODO(Ecy): calculate the size instead
	titleNode->left   = node->left + 5;
	titleNode->top    = node->top + 5;
	titleNode->background = &DEFAULT_TEXT_COLOR;
	
	// TODO(Ecy): use a linear allocator instead of hardcoded array
	memcpy(titleNode->title, title, titleSize);
	titleNode->titleSize = titleSize;
	
	titleNode->parent = node;
	
	return true;
}

internal b32
UIEnd()
{
	Assert(mainUi.currentContextNode);
	
	mainUi.currentContextNode = mainUi.currentContextNode->parent;
	return true;
}


internal void
InitFont(app_state *state, font_engine *engine, char* filename)
{
	debug_read_file_result fontFile = state->DEBUGPlatformReadEntireFile(NULL, filename);
	
	memcpy(&engine->asset, fontFile.contents, sizeof(asset_font));
	
	u8 *textureData = (u8*)fontFile.contents;
	textureData += sizeof(asset_font);
	
	MakeTexture(1, &texture);
	
	PushDataToTexture(texture, engine->asset.width, engine->asset.height, textureData);
	engine->textureId = texture;
	
	state->DEBUGPlatformFreeFileMemory(NULL, fontFile.contents);
}

internal void
DebugRenderText(render_group *group, char *buffer, u32 bufferSize, 
				u32 x, u32 y, r32 scale, color fontColor)
{
	r32 currentXCursor = 0.0f;
	
	v4 color = RGBToFloat(fontColor);
	
	r32 downOffset = (r32)g_fontEngine.asset.height * scale;
	for(u32 index = 0;
		index < bufferSize;
		++index)
	{
		char currentCharacter = buffer[index];
		if(currentCharacter == ' ')
		{
			// TODO(Ecy): the space character width should be described in the asset file instead
			currentXCursor += g_fontEngine.asset.height * 0.2f * scale;
			continue;
		}
		
		asset_font_glyph *glyph = &g_fontEngine.asset.glyphs[currentCharacter - FONT_BASE_OFFSET];
		asset_font_glyph *nextGlyph = &g_fontEngine.asset.glyphs[currentCharacter - FONT_BASE_OFFSET + 1];
		
		r32 u = glyph->u;
		r32 v = 0.0f;
		r32 glyphWidth  = nextGlyph->u;
		r32 glyphHeight = glyph->v;
		
		r32 posX   = (r32)x + currentXCursor;
		r32 posY   = (r32)y + (r32)glyph->yoffset * scale + downOffset;
		r32 height = scale * glyph->height;
		r32 width  = height * glyph->ratio;
		
		PushAxisAlignedGlyph(group, posX, posY, width, height, u, v, glyphWidth, glyphHeight, (r32*)&color);
		
		currentXCursor += width;
	}
}

internal void
InitApp(app_state *appContext)
{
	app_keyboard_input *keyboardInput = appContext->keyboards[0];
	app_pointer_input  *pointerInput  = appContext->pointers[0];
	
	LoadVideoContext(&decoder, "data/sample.mp4");
	InitFont(appContext, &g_fontEngine, "data/asset_data");
	
	{
		// NOTE(Ecy): not sure if this is the best solution, but for the time being, 
		// it removes app_state from PushRect render queue calls
		uiRenderGroup.appContext = appContext;
		debugRenderGroup.appContext = appContext;
	}
	
	{
		g_mainArena = CreateArenaMem((u8*)appContext->permanentStorage, appContext->permanentStorageSize);
	}
	
	{
		// TODO(Ecy): until it becomes a problem ...
		debugRenderGroup = CreateRenderGroup(appContext, &g_mainArena, Megabytes(1), Megabytes(1));
		uiRenderGroup    = CreateRenderGroup(appContext, &g_mainArena, Megabytes(1), Megabytes(1));
	}
	
	pointerInput->sensX = 1.0f;
	pointerInput->sensY = 1.0f;
	pointerInput->sensZ = 1.0f;
	
}

internal void
UpdateApp(app_state *appContext)
{
	app_keyboard_input *keyboardInput = appContext->keyboards[0];
	app_pointer_input  *pointerInput  = appContext->pointers[0];
	
	{
		// NOTE(Ecy): that is not good ... should not be reseted at this stage
		debugRenderGroup.vertexCount = 0;
		debugRenderGroup.indexCount = 0;
		
		char buffer[256];
		// TODO(Ecy): remove this horrible thing here, and also stdio
		u32 bytesWritten = sprintf(buffer, "Frametime: %.2fms\n", appContext->frameTime * 1000.0f);
		DebugRenderText(&debugRenderGroup, buffer, bytesWritten, 
						appContext->width - 300.0f, appContext->height - 100.0f, 0.3f, GOLD);
		
		// NOTE(Ecy): disply current key
		char *cursor = buffer;
		for(u32 index = 0;
			index < KEY_COUNT;
			++index)
		{
			if(keyboardInput->keys[index].endedDown)
			{
				cursor += (u8)sprintf(cursor, "%s", keyLabels[index]);
			}
		} 
		DebugRenderText(&debugRenderGroup, buffer, cursor - buffer, 
						appContext->width - 300.0f, appContext->height - 200.0f, 0.3f, WHITE);
		
		bytesWritten = sprintf(buffer, "Mouse: x%d - y:%d", 
							   pointerInput->posX, pointerInput->posY);
		DebugRenderText(&debugRenderGroup, buffer, bytesWritten, 
						100.0f, appContext->height - 100.0f, 0.3f, GOLD);
	}

	{
		uiRenderGroup.vertexCount = 0;
		uiRenderGroup.indexCount = 0;
		mainUi.nodeCount = 0;
		
		color mainFrameBackground = RED;
		color background = BLUE;
		{
			// NOTE(Ecy): test im mode gui
			if(UIFrameBegin("parent", 6, &mainFramePos, &mainFrameSize, &mainFrameBackground))
			{
				if(IsUiClicked(pointerInput, &mainUi))
				{
					mainFrameBackground = GRAY;
				} 
				
				if(UIFrameBegin("child", 5, &subFramePos, &subFrameSize, &background))
				{
					if(IsUiHovered(pointerInput, &mainUi))
					{
						background = LIME;
					}
					
					if(IsUiClicked(pointerInput, &mainUi))
					{
						background = GOLD;
						
						// NOTE(Ecy): experimental drag and drop
						
					}
					
					UIEnd();
				}
				UIEnd();
			}
		}

		Assert(!mainUi.currentContextNode)
		for(u32 index = 0;
			index < mainUi.nodeCount;
			++index)
		{
			// TODO(Ecy): order node list for Z depth testing, then use mainUi.nodes instead
			ui_node *node = &mainUi._nodes[index];

			v4 color = RGBToFloat(*node->background);
			
			switch(node->type)
			{
				case UI_NODE_FRAME: 
				{
					PushAxisAlignedRect(&uiRenderGroup, (r32)node->left, (r32)node->top, (r32)node->width, (r32)node->height, 
										(r32*)&color);
				}break;
				
				case UI_NODE_TEXT: 
				{
					DebugRenderText(&debugRenderGroup, node->title, node->titleSize, 
									node->left, node->top, 0.25f, WHITE);
				}break;
				
				default:
				{
					// NOTE(Ecy): impossible code path
					Assert(false);
				}break;
			}
			
		}
		
	}

	if(decoder.isLoaded)
	{
		UpdateDecode(&decoder);
		PushDataToTextureRGB(g_bgTexture, decoder.codecContext->width, decoder.codecContext->height, 
						  decoder.pFrameRGB->data[0]);
	}
}
