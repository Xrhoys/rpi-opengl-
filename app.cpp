#include "app.h"

#include "renderer.cpp"
#include "video_decode.cpp"

#define FONT_FILE "data/asset_data"
#define SAMPLE_DATA "data/sample.mp4"

global video_decode decoder;
global app_ui mainUi;
GLuint texture;
global memory_arena g_mainArena;

/*
// The following describe an Immediate mode GUI API 
*/

// UI Frame element
internal b32
UIFrameBegin(char *title, u32 titleSize, v2 pos, v2 size, color background)
{
	ui_node *node = NewNode(&mainUi);
	
	// NOTE(Ecy): this works like push and pop id
	if(mainUi.currentContextNode)
	{
		node->parent = mainUi.currentContextNode;
		//AddNode(node->parent, node);
	}
	
	mainUi.currentContextNode = node;
	
	node->type   = UI_NODE_FRAME;
	node->left   = pos.x;
	node->top    = pos.y;
	node->width  = size.x;
	node->height = size.y;
	node->background = background;
	
	ui_node *titleNode = NewNode(&mainUi);
	
	// NOTE(Ecy): those are RELATIVE positions
	titleNode->type   = UI_NODE_TEXT;
	// NOTE(Ecy): leaves 5 pixels from the edges
	// TODO(Ecy): calculate the size instead
	titleNode->left   = 5;
	titleNode->top    = 5;
	
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

	glGenTextures(1, &texture);
	
	// TODO(Ecy): is this a behavior specific to opengl? 
	// You probably need to generate before use if you want to use multiple textures: texImage2D etc.
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, engine->asset.width, engine->asset.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 
				 textureData);
	glGenerateMipmap(GL_TEXTURE_2D);
	
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
	InitRenderer();
	InitFont(appContext, &g_fontEngine, "data/asset_data");
	
	LoadVideoContext(&decoder, SAMPLE_DATA);
	
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
	
	
}

internal void
UpdateAndRenderApp(app_state *appContext)
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
		
	}

	{
		uiRenderGroup.vertexCount = 0;
		uiRenderGroup.indexCount = 0;
		mainUi.nodeCount = 0;
		
		{
			// NOTE(Ecy): test im mode gui
			if(UIFrameBegin("parent", 6, { 100.0f, 100.0f }, { 200.0f, 200.0f }, RED))
			{
				if(UIFrameBegin("child", 5, { 50.0f, 5.0f }, { 100.0f, 100.0f }, BLUE))
				{
					UIEnd();
				}
				UIEnd();
			}
			
			if(UIFrameBegin("parent1", 7, { 500.0f, 500.0f }, { 200.0f, 500.0f }, LIME))
			{
				if(UIFrameBegin("child1", 6, { 50.0f, 5.0f }, { 100.0f, 500.0f }, BLUE))
				{
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
			
			if(node->parent)
			{
				node->top += node->parent->top;
				node->left += node->parent->left;
			}
			
			v4 color = RGBToFloat(node->background);
			
			switch(node->type)
			{
				case UI_NODE_FRAME: 
				{
					PushAxisAlignedRect(&uiRenderGroup, node->top, node->left, node->width, node->height, 
										(r32*)&color);
				}break;
				
				case UI_NODE_TEXT: 
				{
					DebugRenderText(&debugRenderGroup, node->title, node->titleSize, 
									node->top, node->left, 0.25f, WHITE);
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
		glBindTexture(GL_TEXTURE_2D, g_bgTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, decoder.codecContext->width, decoder.codecContext->height, 0, GL_RGB,
					 GL_UNSIGNED_BYTE, decoder.pFrameRGB->data[0]);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	
	Render();
}
