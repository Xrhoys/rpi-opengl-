#include <windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>

extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
#include <libavdevice/avdevice.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
}

#include <EGL/egl.h>
#include <EGL/eglplatform.h>
#include <GL/gl.h>
#include <GLES2/gl2.h>
#include <GLES3/gl3.h>

#define GL_ES_VERSION_3_0 1

#include "main.h"

bool g_running = false;

static void Decode(AVCodecContext *dec_ctx, AVFrame *frame, AVPacket *pkt)
{
	char buffer[1024];
    int ret;
	
    ret = avcodec_send_packet(dec_ctx, pkt);
    if (ret < 0) {
        fprintf(stderr, "Error sending a packet for decoding\n");
        return;
    }
	
    while (ret >= 0) {
        ret = avcodec_receive_frame(dec_ctx, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            return;
        else if (ret < 0) {
            fprintf(stderr, "Error during decoding\n");
            return;
        }
		
        /* the picture is allocated by the decoder. no need to
           free it */
		
		int b = 0;
    }
}

int CALLBACK
WinMain(HINSTANCE Instance,
        HINSTANCE PrevInstance,
        LPSTR CommandLine,
        int ShowCode)
{
    // Create application window
    //ImGui_ImplWin32_EnableDpiAwareness();
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, _T("RPI Emulation"), NULL };
    ::RegisterClassEx(&wc);
    HWND hwnd = ::CreateWindow(wc.lpszClassName, _T("RPI Emulation"), WS_OVERLAPPEDWINDOW, 100, 100, WINDOW_WIDTH, WINDOW_HEIGHT, NULL, NULL, wc.hInstance, NULL);
    
    // Show the window
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);
	
	HDC hdc = GetDC(hwnd);
    EGLDisplay eglDisplay = eglGetDisplay(hdc);
    EGLint eglVersionMajor, eglVersionMinor;
    eglInitialize(eglDisplay, &eglVersionMajor, &eglVersionMinor);
	
	eglBindAPI(EGL_OPENGL_ES_API);
	
	EGLint configAttributes[] =
	{
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_DEPTH_SIZE, 16,
		EGL_BLUE_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_RED_SIZE, 8,
		EGL_ALPHA_SIZE, 8,
		EGL_NONE
	};
	
	EGLint surfaceAttributes[] = { EGL_NONE };
	EGLint contextAttributes[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };
	
	EGLint nrOfConfigs;
	EGLConfig windowConfig;
	eglChooseConfig(eglDisplay, configAttributes, &windowConfig, 1, &nrOfConfigs);
	EGLSurface eglSurface = eglCreateWindowSurface(eglDisplay, windowConfig, hwnd, surfaceAttributes);
	if (eglSurface == EGL_NO_SURFACE) {
		EGLint error = eglGetError();
		
		char *errorStr = getErrorStr(error);
		fprintf(stdout, "Could not create EGL surface : %s\n", errorStr);
		return 1;
	}
	
	EGLContext eglContext = eglCreateContext(eglDisplay, windowConfig, NULL, contextAttributes);
	
	EGLConfig *eglConfigs;
	int eglNumConfigs;
	eglGetConfigs(eglDisplay, NULL, 0, &eglNumConfigs);
	eglConfigs = new EGLConfig[eglNumConfigs];
	
	eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext);

#if 0	
	{
		AVFormatContext *pFormatCtx = NULL;
		if(avformat_open_input(&pFormatCtx, "sample.mp4", NULL, 0) != 0)
		{
			// Couldn't open file
			return -1;
		}
		
		if(avformat_find_stream_info(pFormatCtx, NULL) < 0)
		{
			// Couldn't find stream information
			return -1;
		}
		
		AVCodecContext *pCodecCtx = NULL;

		int videoStream = -1;
		for(int index = 0;
			index < pFormatCtx->nb_streams;
			++index)
		{
			if(pFormatCtx->streams[index]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
			{
				videoStream = index;
				break;
			}
		}
		
		if(videoStream == -1)
		{
			// NO video stream found.
			return -1;
		}
		
		pCodecCtx = pFormatCtx->streams[videoStream]->;
		
		const AVCodec *pCodec = pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
		
		if(pCodec == NULL)
		{
			// Unsupported codec
			return -1;
		}
		
		AVCodecContext* pCodecCpy = avcodec_alloc_context3(pCodec);
		
		if(pCodecCpy == NULL)
		{
			// Copy context failed
			return -1;
		}
		
		if(avcodec_open2(pCodecCpy, pCodec, NULL) < 0)
		{
			// Could not open codec
			return -1;
		}
		
		AVFrame *pFrame = NULL;
		AVFrame *pFrameRGB = NULL;
		
		pFrame = av_frame_alloc();
		pFrameRGB = av_frame_alloc();
		
		uint8_t *buffer = NULL;
		int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, pCodecCtx->width,
									pCodecCtx->height, 16);
		buffer = (uint8_t *)av_malloc(numBytes*sizeof(uint8_t));
		
		av_image_fill_arrays(pFrameRGB->data, pFrameRGB->linesize, buffer, AV_PIX_FMT_RGB24,
					   pCodecCtx->width, pCodecCtx->height, 1);
		
		struct SwsContext *sws_ctx = NULL;
		int frameFinished;
		
		AVPacket *packet = av_packet_alloc();
		
#if 0		
		while(av_read_frame(pFormatCtx, &packet) > 0)
		{
			if(packet.stream_index == videoStream)
			{
				avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);
				
				if(frameFinished)
				{
					sws_scale(sws_ctx, (uint8_t*)pFrame->data, pFrame->linesize, 0, pCodexCtx->height, pFrameRGB->data, pFrameRGB->linesize);
					
		int b = 0;
				}
			}
		}
		
		av_free_packet(&packet);
		#endif
		
		int ret;
		ret = avcodec_send_packet(pCodecCpy, packet);
		if (ret < 0)
		{
		}
		
		ret = avcodec_receive_frame(pCodecCpy, pFrameRGB);
		if(ret >= 0)
		{
			
		}
		
		av_frame_unref(pFrame);
		av_packet_free(&packet);
	}
	#endif

	
	{
		const AVCodec *codec;
		AVCodecParserContext *parser;
		AVCodecContext *c = NULL;
		AVFrame *frame;
		
		int ret;
		int eof;
		
		AVPacket *packet = av_packet_alloc();
		if(!packet) return -1;
		
		codec = avcodec_find_decoder(AV_CODEC_ID_MPEG4);
		if(!codec) return -1;
		
		parser = av_parser_init(codec->id);
		if(!parser) return -1;
		
		c = avcodec_alloc_context3(codec);
		if(!c) return -1;
		
		c->height = 1344;
		c->width = 768;
		
		if(avcodec_open2(c, codec, NULL) < 0)
		{
			return -1;
		}
		
		frame = av_frame_alloc();
		if(!frame) return -1;
		
		debug_read_file_result sample = Win32ReadEntireFile("sample.mp4");
		if(sample.contentSize == 0)
		{
			return -1;
		}
		
		uint8_t *cursor = (uint8_t*)sample.contents;
		uint8_t *endOfFile = (uint8_t*)sample.contents + sample.contentSize;
		
		while(cursor < endOfFile)
		{
			uint32_t data_size = 4096;
			while(data_size > 0 || cursor < endOfFile)
			{
				ret = av_parser_parse2(parser, c, &packet->data, &packet->size,
									   cursor, data_size, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
				
				if(ret < 0)
				{
					// Error while parsing
					return -1;
				}
				
				cursor += ret;
				data_size -= ret;
				
				if(packet->size)
				{
					Decode(c, frame, packet);
				}
			}
			
		}
		
	}
	
	GLuint VA0;
    GLuint shaderProgram;
    GLuint VB0;
	GLuint EB0;
    float vertices[] = 
    {
        1.0f,  1.0f, 0.0f,  // top right
		1.0f, -1.0f, 0.0f,  // bottom right
		-1.0f, -1.0f, 0.0f,  // bottom left
		-1.0f,  1.0f, 0.0f   // top left 
    };
	unsigned int indices[] = 
	{
		0, 1, 3,   // first triangle
		1, 2, 3    // second triangle
	};
	
    {
        const char *vertexShaderSource = 
			"#version 310 es\n"
			"precision mediump float;\n"
            "in vec3 aPos;\n"
            "void main()\n"
            "{\n"
            "   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
            "}\0";
		
        GLuint vertexShader;
        vertexShader = glCreateShader(GL_VERTEX_SHADER);
		
        glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
        glCompileShader(vertexShader);
		
        // Check ShaderCompile success
		int  success;
		char infoLog[512];
		glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
		if(!success)
		{
			glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
			fprintf(stdout, "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n%s", infoLog);
		}
		
        const char *fragmentShaderSource = "#version 310 es\n"
			"precision mediump float;\n"
            "out vec4 FragColor;\n"
            "void main()\n"
            "{\n"
            "    FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);\n"
            "}\0";
        
        GLuint fragmentShader;
        fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
		
        glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
        glCompileShader(fragmentShader);
		
        // Check ShaderCompile success
		glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
		if(!success)
		{
			glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
			fprintf(stdout, "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n%s", infoLog);
		}
		
        shaderProgram = glCreateProgram();
		
        glAttachShader(shaderProgram, vertexShader);
        glAttachShader(shaderProgram, fragmentShader);
        glLinkProgram(shaderProgram);
		
        // Check ShaderCompile success
		glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
		if(!success) {
			glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
			fprintf(stdout, "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n%s", infoLog);
		}
		
        //glDeleteShader(vertexShader);
        //glDeleteShader(fragmentShader);
		
        glGenVertexArrays(1, &VA0);
        glBindVertexArray(VA0);
		glGenBuffers(1, &VB0);
		
        glBindBuffer(GL_ARRAY_BUFFER, VB0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EB0);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
		
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EB0);
		
		//glEnable(GL_DEPTH_TEST);
    }
	
	g_running = true;
	while(g_running)
	{
		MSG msg;
        while (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
			if (msg.message == WM_QUIT)
                g_running = false;
        }
		
		if (!g_running)
            break;
		
		{ 
			glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
			glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);
			
			glUseProgram(shaderProgram);
			glBindVertexArray(VA0);
			
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		}
		
		eglSwapBuffers(eglDisplay, eglSurface);
	}
	
    ::DestroyWindow(hwnd);
    ::UnregisterClass(wc.lpszClassName, wc.hInstance);
    
    return 0;
}
