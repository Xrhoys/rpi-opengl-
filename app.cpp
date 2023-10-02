#include "app.h"

#include "renderer.cpp"
#include "video_decode.cpp"

#define FONT_FILE "data/asset_data"
#define SAMPLE_DATA "data/sample.mkv"
#define ASSET_DATA "data/asset_data"

global video_decode decoder;
global app_ui mainUi;
GLuint texture;
global memory_arena g_mainArena;
global color DEFAULT_TEXT_COLOR = WHITE;

// NOTE(Xrhoys): UI test globals
global v2U32 mainFramePos  = { 100, 100 };
global v2U32 mainFrameSize = { 200, 200 };

global v2U32 subFramePos  = { 5, 50 };
global v2U32 subFrameSize = { 100, 100 };

global v2U32 lastClickedPos = { 100, 100 };

/*
// The following describe an Immediate mode GUI API 
// Very much work in progress
*/
internal b32
UIFrameBegin(char *title, u32 titleSize, v2U32 *pos, v2U32 *size, color *background)
{
	ui_node *node = NewNode(&mainUi);
	
	// NOTE(Xrhoys): this works like push and pop id
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
	
	// NOTE(Xrhoys): those are RELATIVE positions
	titleNode->type   = UI_NODE_TEXT;
	// NOTE(Xrhoys): leaves 5 pixels from the edges
	// TODO(Xrhoys): calculate the size instead
	titleNode->left   = node->left + 5;
	titleNode->top    = node->top + 5;
	titleNode->background = &DEFAULT_TEXT_COLOR;
	
	// TODO(Xrhoys): use a linear allocator instead of hardcoded array
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
	
	// TODO(Xrhoys): is this a behavior specific to opengl? 
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
			// TODO(Xrhoys): the space character width should be described in the asset file instead
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
	
	InitRenderer();
	InitFont(appContext, &g_fontEngine, ASSET_DATA);
	
	LoadVideoContext(&decoder, SAMPLE_DATA);
	
	{
		// NOTE(Xrhoys): not sure if this is the best solution, but for the time being, 
		// it removes app_state from PushRect render queue calls
		uiRenderGroup.appContext = appContext;
		debugRenderGroup.appContext = appContext;
	}
	
	{
		g_mainArena = CreateArenaMem((u8*)appContext->permanentStorage, appContext->permanentStorageSize);
	}
	
	{
		// TODO(Xrhoys): fixed size until until it becomes a problem ...
		debugRenderGroup = CreateRenderGroup(appContext, &g_mainArena, Megabytes(1), Megabytes(1));
		uiRenderGroup    = CreateRenderGroup(appContext, &g_mainArena, Megabytes(1), Megabytes(1));
	}
	
	pointerInput->sensX = 1.0f;
	pointerInput->sensY = 1.0f;
	pointerInput->sensZ = 1.0f;
	
}

internal void
UpdateAndRenderApp(app_state *appContext)
{
	app_keyboard_input *keyboardInput = appContext->keyboards[0];
	app_pointer_input  *pointerInput  = appContext->pointers[0];
#if DEBUG
	{
		// NOTE(Xrhoys): that is not good ... should not be reset at this stage
		debugRenderGroup.vertexCount = 0;
		debugRenderGroup.indexCount = 0;
		
		char buffer[256];
		// TODO(Xrhoys): remove sprintf , but it's for debugging so it's ok here, and also stdio
		u32 bytesWritten = sprintf(buffer, "Fps: %.2f\n", 1000.0f / appContext->frameTime);
		DebugRenderText(&debugRenderGroup, buffer, bytesWritten, 
						appContext->width - 300.0f, appContext->height - 100.0f, 0.3f, GOLD);
		
		// NOTE(Xrhoys): disply current key
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
#endif

	{
		uiRenderGroup.vertexCount = 0;
		uiRenderGroup.indexCount = 0;
		mainUi.nodeCount = 0;
		
		color mainFrameBackground = RED;
		color background = BLUE;
		{
			// NOTE(Xrhoys): test imgui
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
						
						// NOTE(Xrhoys): experimental drag and drop
						
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
			// TODO(Xrhoys): order node list for Z depth testing, then use mainUi.nodes instead
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
					// NOTE(Xrhoys): impossible code path
					Assert(false);
				}break;
			}
			
		}
		
	}

	if(decoder.isLoaded)
	{
		if(decoder.frameCount < 2)
		{
			// NOTE(Xrhoys): select available decoding frame from pool
			for(u32 index = 0;
				index < 1;
				++index)
			{
				video_decode_unit *unit = &decoder.framePool[index];
				if(unit->frameId == -1)
				{
					fprintf(stderr, "debug1\n");
					if(av_read_frame(decoder.formatContext, unit->packet) >= 0)
					{
						fprintf(stderr, "debug2\n");
						if(unit->packet->stream_index == decoder.streamIndex)
						{
							fprintf(stderr, "debug3\n");
							//Decode(decoder);
							QueueThreadedWork(&threadQueue, (thread_func_t)DecodeThreaded, unit);
							decoder.frameCount++;
						}
					}
					break;
				}
				
			}
			
		}
		
#if 0
		// NOTE(Xrhoys): Look for the next frame to render: displayedFrame
		int findFrameId = decoder.displayedFrameId + 1;
		video_decode_unit *selectedFrame = nullptr;
		{
			//fprintf(stderr, "sframe %d, count: %d, displayedFrameId: %d \n", selectedFrame, decoder.frameCount, decoder.displayedFrameId);

#if 0			
			if(findFrameId < decoder.frameCount + decoder.displayedFrameId)
			{
				break;
			}
#endif

			for(u32 index = 0;
				index < DECODE_QUEUE_SIZE;
				++index)
			{
				video_decode_unit *unit = &decoder.framePool[index];
				if(unit->isReady && unit->frameId == findFrameId)
				{
					//fprintf(stderr, "isReady %d, frameId %d\n", unit->isReady, unit->frameId);
					selectedFrame = unit;
					break;
				}
			}
		}
		
		if(selectedFrame)
		{
			fprintf(stderr, "selecting frame %d\n", selectedFrame->frameId);
			glEnable(GL_TEXTURE_EXTERNAL_OES);
			glBindTexture(GL_TEXTURE_EXTERNAL_OES, g_bgTexture);
			glEGLImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, selectedFrame->eglImage);
			glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			
			eglDestroyImageKHR(eglDisplay, selectedFrame->eglImage);
			
			selectedFrame->isReady = false;
			selectedFrame->frameId = -1;
			
			av_packet_unref(selectedFrame->packet);
			av_frame_unref(selectedFrame->frame);
		}
#endif

		//UpdateDecode(&decoder);
		// TODO(Xrhoys): to move to renderer files
		// glBindTexture(GL_TEXTURE_2D, g_bgTexture);
		// glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, decoder.codecContext->width, decoder.codecContext->height, 0, GL_RGB,
		// 			 GL_UNSIGNED_BYTE, decoder.pFrameRGB->data[0]);
		// glGenerateMipmap(GL_TEXTURE_2D);
	}

	Render();
}
