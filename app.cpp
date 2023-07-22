#include "app.h"
#include "renderer.cpp"
#include "video_decode.cpp"

#define FONT_FILE "data/asset_data"
#define SAMPLE_DATA "data/sample.mp4"

global video_decode decoder;
global app_ui mainUi;
GLuint texture;

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
	
	state->DEBUGPlatformFreeFileMemory(NULL, &fontFile);
}

internal void
DebugRenderText(render_group *group, app_state *appState, 
				char *buffer, u32 bufferSize, 
				u32 x, u32 y, r32 scale)
{
	r32 currentXCursor = 0.0f;
	
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
		
		PushAxisAlignedGlyph(group, appState, 
							 posX, posY, width, height, 
							 u, v, glyphWidth, glyphHeight);
		
		currentXCursor += width;
	}
}

internal void
InitApp(app_state *appContext)
{
	InitRenderer(appContext);
	InitFont(appContext, &g_fontEngine, "data/asset_data");
	
	LoadVideoContext(&decoder, SAMPLE_DATA);
	
	{
		ui_node *root       = NewNode(&mainUi);
		ui_node *childNode1 = NewNode(&mainUi);
		ui_node *childNode2 = NewNode(&mainUi);
		ui_node *childNode3 = NewNode(&mainUi);
		ui_node *childNode4 = NewNode(&mainUi);
		ui_node *childNode5 = NewNode(&mainUi);
		
		AddNode(root,       childNode1);
		AddNode(childNode1, childNode2);
		AddNode(childNode2, childNode3);
		AddNode(root,       childNode4);
		AddNode(root,       childNode5);
	}
}

internal void
UpdateAndRenderApp(app_state *appContext)
{
	{
		// NOTE(Ecy): that is not good ... should not be reseted at this stage
		debugRenderGroup.vertexCount = 0;
		debugRenderGroup.indexCount = 0;
		
		char buffer[32];
		u32 bytesWritten = sprintf(buffer, "Frametime: %.2fms\n", appContext->frameTime * 1000.0f);
		DebugRenderText(&debugRenderGroup, appContext, buffer, bytesWritten, 10, 10, 0.3f);
	}

#if 0	
	{
		uiRenderGroup.vertexCount = 0;
		uiRenderGroup.indexCount = 0;
		
		// Generate rects for main UI
		for(u32 index = 0;
			index < mainUi.nodeCount;
			++index)
		{
			ui_node *node = &mainUi.nodes[index];
			
			
		}
	}
#endif

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
