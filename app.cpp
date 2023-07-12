#include "app.h"
#include "renderer.cpp"
#include "video_decode.cpp"

static video_decode decoder;

static void
InitApp()
{
	LoadVideoContext(&decoder, "sample.mp4");
}

static void
UpdateAndRenderApp(app_state *appContext)
{
	{
		uiRenderGroup.vertexCount = 0;
		uiRenderGroup.indexCount = 0;
		
		char *hello = "hello world";
		DebugRenderText(&uiRenderGroup, appContext, hello, 11, 10, 10, 10);
		DebugRenderText(&uiRenderGroup, appContext, hello, 11, 500, 100, 100);
		DebugRenderText(&uiRenderGroup, appContext, hello, 11, 800, 500, 100);
	}
	
	UpdateDecode(&decoder);
	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, decoder.codecContext->width, decoder.codecContext->height, 0, GL_RGB, GL_UNSIGNED_BYTE, decoder.pFrameRGB->data[0]);
	glGenerateMipmap(GL_TEXTURE_2D);
	
	Render();
}
