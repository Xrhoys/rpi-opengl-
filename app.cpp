#include "app.h"
#include "renderer.cpp"
#include "video_decode.cpp"

#define FONT_FILE "data/asset_data"
#define SAMPLE_DATA "data/sample.mp4"

global video_decode decoder;
global app_ui mainUi;
global font_engine g_fontEngine;

internal void
InitFont(app_state *state, font_engine *engine, char* filename)
{
	debug_read_file_result fontFile = state->DEBUGPlatformReadEntireFile(NULL, filename);
	
	memcpy(&engine->asset, fontFile.contents, sizeof(asset_font));
	
	u8 *textureData = (u8*)fontFile.contents;
	textureData += sizeof(asset_font);

	GLuint texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, engine->asset.width, engine->asset.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 
				 textureData);
	glGenerateMipmap(GL_TEXTURE_2D);

	engine->textureId = texture;
}

internal void
DebugRenderText(render_group *group, app_state *appState, char *buffer,
				u32 size, u32 x, u32 y, u32 scale)
{
	for(u32 index = 0;
		index < size;
		++index)
	{
		char currentCharacter = buffer[index];
		asset_font_glyph *glyph = &g_fontEngine.asset.glyphs[currentCharacter - FONT_BASE_OFFSET];
		asset_font_glyph *nextGlyph = &g_fontEngine.asset.glyphs[currentCharacter - FONT_BASE_OFFSET + 1];
		
		// TODO(Ecy): load uv font coords from assets
		r32 u = glyph->xoffset;
		r32 v = glyph->yoffset;
		r32 glyphWidth = nextGlyph->xoffset;
		r32 glyphHeight = nextGlyph->yoffset;
		
		// TODO(Ecy): this ratio is queried from assets
		r32 ratio = 1.5;
		
		// NOTE(Ecy): what would be the scale of the font?
		PushAxisAlignedGlyph(group, appState, x + index * 100, y, scale, scale * ratio, u, v, glyphWidth, glyphHeight);
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
#if 1
	{
		// NOTE(Ecy): that is not good ... should not be reseted at this stage
		debugRenderGroup.vertexCount = 0;
		debugRenderGroup.indexCount = 0;
		
		char *hello = "hello world";
		DebugRenderText(&debugRenderGroup, appContext, hello, 11, 10, 10, 10);
		DebugRenderText(&debugRenderGroup, appContext, hello, 11, 500, 100, 100);
		DebugRenderText(&debugRenderGroup, appContext, hello, 11, 800, 500, 100);
	}
#endif

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

#if 0
	if(decoder.isLoaded)
	{
		UpdateDecode(&decoder);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, decoder.codecContext->width, decoder.codecContext->height, 0, GL_RGB,
					 GL_UNSIGNED_BYTE, decoder.pFrameRGB->data[0]);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
#endif

	Render();
}
