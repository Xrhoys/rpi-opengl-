#include "app.h"
#include "renderer.cpp"
#include "video_decode.cpp"

#define FONT_FILE "data/asset_data"
#define SAMPLE_DATA "data/sample.mp4"

global video_decode decoder;
global app_ui mainUi;
global font_engine g_fontEngine;

inline void
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
	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, engine->asset.width, engine->asset.height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, textureData);
	glGenerateMipmap(GL_TEXTURE_2D);

	engine->textureId = texture;
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
#if 0
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
	
	if(decoder.isLoaded)
	{
		UpdateDecode(&decoder);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, decoder.codecContext->width, decoder.codecContext->height, 0, GL_RGB, GL_UNSIGNED_BYTE, decoder.pFrameRGB->data[0]);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	
	Render();
}
