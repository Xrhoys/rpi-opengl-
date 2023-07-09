#include <windows.h>

#include <stdio.h>
#include <tchar.h>

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
#include <libavdevice/avdevice.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libavutil/pixfmt.h>

#include <EGL/egl.h>
#include <EGL/eglplatform.h>
#include <GL/gl.h>
#include <GLES2/gl2.h>
#include <GLES3/gl3.h>

#define GL_ES_VERSION_3_0 1

// NOTE(Ecy): this should be included from the app.h later
#include "platform.h"

#include "renderer.h"
#include "main.h"

static app_state g_state;

struct SwsContext *swsCtx;
AVFrame *pFrameRGB;
	char buffer[1024];

static void Decode(AVCodecContext *dec_ctx, AVFrame *frame, AVPacket *pkt)
{
    int ret;
	
    ret = avcodec_send_packet(dec_ctx, pkt);
	
	av_strerror(ret, buffer, 1024);
	
    if (ret < 0) {
        fprintf(stderr, "Error sending a packet for decoding\n");
        return;
    }
	
    while (ret >= 0) {
        ret = avcodec_receive_frame(dec_ctx, frame);
		
		av_strerror(ret, buffer, 1024);
		
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            return;
        else if (ret < 0) {
            fprintf(stderr, "Error during decoding\n");
            return;
        }
		
		if(ret >= 0)
		{
			ret = sws_scale(swsCtx, (const uint8_t* const*)frame->data, frame->linesize,
							0, frame->height,
							(uint8_t *const *)pFrameRGB->data, pFrameRGB->linesize);
		}
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
    RegisterClassEx(&wc);
    HWND hwnd = CreateWindow(wc.lpszClassName, _T("RPI Emulation"), WS_OVERLAPPEDWINDOW, 100, 100, WINDOW_WIDTH, WINDOW_HEIGHT, NULL, NULL, wc.hInstance, NULL);
    
    // Show the window
    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);
	
	{
		lastCycleCount = __rdtsc();
		
		LARGE_INTEGER counter;
		LARGE_INTEGER perfCountFrequencyResult;
		QueryPerformanceCounter(&counter);
		QueryPerformanceFrequency(&perfCountFrequencyResult);
		
		g_bootCounter = counter;
		g_lastCounter = counter;
		g_perfCount = perfCountFrequencyResult.QuadPart;
	}
	
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
		
		AVCodecParameters *pCodecParam = NULL;
		const AVCodec *pCodec = NULL;
		
		int videoStream = -1;
		for(int index = 0;
			index < pFormatCtx->nb_streams;
			++index)
		{
			AVCodecParameters *pLocalCodecParameters = pFormatCtx->streams[index]->codecpar;
			const AVCodec *pLocalCodec = avcodec_find_decoder(pLocalCodecParameters->codec_id);
			
			if(pLocalCodecParameters->codec_type == AVMEDIA_TYPE_VIDEO)
			{
				videoStream = index;
				pCodec = pLocalCodec;
				pCodecParam = pLocalCodecParameters;
				break;
			}
		}
		if(videoStream == -1)
		{
			// NO video stream found.
			return -1;
		}
		
		AVCodecContext* pCodecCtx = avcodec_alloc_context3(pCodec);
		
		if(pCodecCtx == NULL)
		{
			// Copy context failed
			return -1;
		}
		
		if(avcodec_parameters_to_context(pCodecCtx, pCodecParam) < 0)
		{
			// Copy context failed
			return -1;
		}
		
		if(avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
		{
			// Could not open codec
			return -1;
		}
		
		AVFrame *pFrame    = av_frame_alloc();
		pFrameRGB = av_frame_alloc();
		
		AVPacket *packet = av_packet_alloc();
		
		swsCtx = sws_getContext(pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P, 
								pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_RGB24,
								SWS_BICUBIC, NULL, NULL, NULL);
		
		int num_bytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height, 1);
		unsigned char* frame_buffer = (uint8_t*)av_malloc(num_bytes);
		av_image_fill_arrays(pFrameRGB->data,       //uint8_t *dst_data[4], 
							 pFrameRGB->linesize,   //int dst_linesize[4],
							 frame_buffer,          //const uint8_t * src,
							 AV_PIX_FMT_RGB24,      //enum AVPixelFormat pix_fmt,
							 pCodecCtx->width,   //int width, 
							 pCodecCtx->height,  //int height,
							 1);                    //int align);
		
		pFrameRGB->width = pCodecCtx->width;
		pFrameRGB->height = pCodecCtx->height;
		
		if(!swsCtx)
		{
			// no scaler context found
			return -1;
		}

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
	
	int eglNumConfigs;
	eglGetConfigs(eglDisplay, NULL, 0, &eglNumConfigs);
	EGLConfig *eglConfigs = (EGLConfig*)malloc(sizeof(EGLConfig) * eglNumConfigs);
	
	eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext);
	
    GLuint shaderProgram;
    
	GLuint VA0, VB0, EB0, FontV0, FontB0;
	GLuint texture;
	
	float vertices[] = 
    {
		-1.0f, -1.0f, 0.0f, 0.0f, 1.0f, // bottom left
		1.0f, -1.0f, 0.0f,  1.0f, 1.0f,// bottom right
        1.0f,  1.0f, 0.0f,  1.0f, 0.0f,// top right
		-1.0f,  1.0f, 0.0f, 0.0f, 0.0f, // top left 
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
            "layout (location = 0) in vec3 aPos;\n"
			"layout (location = 1) in vec2 aTexCoord;\n"
			"out vec2 TexCoord;\n"
            "void main()\n"
            "{\n"
            "   gl_Position = vec4(aPos, 1.0);\n"
			"   TexCoord = aTexCoord;\n"
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
		
        const char *fragmentShaderSource = 
			"#version 310 es\n"
			"precision mediump float;\n"
			"in vec2 TexCoord;\n"
            "out vec4 FragColor;\n"
			"uniform sampler2D tex;\n"
            "void main()\n"
            "{\n"
            "    FragColor = texture(tex, TexCoord);\n"
			//"    FragColor = vec4(1.0f, 0.5f, 1.0f, 1.0f);\n"
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
		glGenBuffers(1, &VB0);
		glGenBuffers(1, &EB0);
		glGenBuffers(1, &FontB0);
		glGenBuffers(1, &FontV0);
		
        glBindBuffer(GL_ARRAY_BUFFER, VB0);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EB0);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
		
		glBindBuffer(GL_ARRAY_BUFFER, FontV0);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertex) * 50000, NULL, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, FontB0);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(u32) * 50000 * 6 / 4, NULL, GL_DYNAMIC_DRAW);

		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		
		//glEnable(GL_DEPTH_TEST);
    }
	
	g_state.running = TRUE;

	g_state.width = WINDOW_WIDTH;
	g_state.height = WINDOW_HEIGHT;
	
	render_group uiRenderGroup;
	uiRenderGroup.vertices = (vertex*)malloc(10 * 1024 * 1024);
	uiRenderGroup.indices  = (u32*)malloc(10 * 1024 * 1024);
	
	while(g_state.running)
	{
		MSG msg;
        while (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			if (msg.message == WM_QUIT)
                g_state.running = 0;
        }
		
		if (!g_state.running)
            break;
		
		// NOTE(Ecy): Replace with suggested frame timing
		//if(Win32GetLastElapsed() > 1.0f / REFRESH_RATE)
		{
			{
				uiRenderGroup.vertexCount = 0;
				uiRenderGroup.indexCount = 0;
				
				char *hello = "hello world";
				DebugRenderText(&uiRenderGroup, &g_state, hello, 11, 10, 10, 10);
				DebugRenderText(&uiRenderGroup, &g_state, hello, 11, 500, 100, 100);
				DebugRenderText(&uiRenderGroup, &g_state, hello, 11, 800, 500, 100);
			}
			
			if(av_read_frame(pFormatCtx, packet) >= 0)
			{
				if(packet->stream_index == videoStream)
				{
					Decode(pCodecCtx, pFrame, packet);
				}
				
				av_packet_unref(packet);
				av_frame_unref(pFrame);
				
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, pCodecCtx->width, pCodecCtx->height, 0, GL_RGB, GL_UNSIGNED_BYTE, pFrameRGB->data[0]);
				glGenerateMipmap(GL_TEXTURE_2D);
			}
			
			//glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
			glClearColor(0.0f, 0.5f, 0.5f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);
			
			glUseProgram(shaderProgram);

#if 0
			{
				glBindVertexArray(VA0);
				
				glEnableVertexAttribArray(0);
				glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
				glEnableVertexAttribArray(1);
				glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),  (void*)(3 * sizeof(float)));
				
				glBindTexture(GL_TEXTURE_2D, texture);
				glBindBuffer(GL_ARRAY_BUFFER, VB0);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EB0);
				glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
			}
#endif

			// NOTE(Ecy): slow, experimental font engine debugging layer
			{
				glBindVertexArray(VA0);
				
				glEnableVertexAttribArray(0);
				glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
				glEnableVertexAttribArray(1);
				glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),  (void*)(3 * sizeof(float)));
				
				glBindTexture(GL_TEXTURE_2D, texture);
				
				glBindBuffer(GL_ARRAY_BUFFER, FontV0);
				glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertex) * uiRenderGroup.vertexCount, uiRenderGroup.vertices);
				
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, FontB0);
				glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, sizeof(u32) * uiRenderGroup.indexCount, uiRenderGroup.indices);
				
				glDrawElements(GL_TRIANGLES, uiRenderGroup.indexCount, GL_UNSIGNED_INT, 0);
			}

			eglSwapBuffers(eglDisplay, eglSurface);
			
			{
				u64 endCycleCounter = __rdtsc();
				u64 cyclesElapsed   = endCycleCounter - lastCycleCount;
				cyclesPerFrame      = cyclesElapsed / (1000.0f * 1000.0f);
				
				lastCycleCount      = endCycleCounter;
				g_lastCounter       = Win32GetWallClock();
			}
		}
		}
	
	av_frame_unref(pFrame);
	av_frame_unref(pFrameRGB);
	av_packet_free(&packet);
	
    DestroyWindow(hwnd);
    UnregisterClass(wc.lpszClassName, wc.hInstance);
    
    return 0;
}
