#include "app.h"
#include "renderer.cpp"
#include "video_decode.cpp"

#define FONT_FILE "data/arial.ttf"
#define SAMPLE_DATA "data/sample.mp4"

global video_decode decoder;
global app_ui mainUi;

internal void
InitApp(app_state *appContext)
{
	InitRenderer(appContext);
	InitFont(appContext, FONT_FILE);
	
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
			
			int b = 0;
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
