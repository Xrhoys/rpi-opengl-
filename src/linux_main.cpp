
#include <stdlib.h>


#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

extern "C" {
  #define _XOPEN_SOURCE 600 /* for usleep */
  #include <libavcodec/avcodec.h>
  #include <libavdevice/avdevice.h>
  #include <libavfilter/avfilter.h>
  #include <libavformat/avformat.h>
  #include <libavutil/avutil.h>
  #include <libavutil/imgutils.h>
  #include <libavutil/timestamp.h>
  #include <libswresample/swresample.h>
  #include <libswscale/swscale.h>
  #include <unistd.h>
}

#include "linux_main.h"

global struct SwsContext *swsCtx = NULL;

global b32                    g_running = false;
global i64                    last_pts = AV_NOPTS_VALUE;
global i64                    delay;
global char                   buffer[1024];
global AVFrame                *pFrameRGB;

// NOTE(Ecy): X11 globals, thsoe need to be global variable
global Window                 root;
global Window                 window;
global Display                *xdisplay;
global XSetWindowAttributes   swa;
global u32                    root_w, 
                              root_h, 
                              root_border_width, 
                              root_depth;


internal void 
Decode(AVCodecContext *dec_ctx, AVFrame *frame, AVPacket *pkt) 
{
  u32 ret = avcodec_send_packet(dec_ctx, pkt);
	
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
			ret = sws_scale(swsCtx, (const u8* const*)frame->data, frame->linesize,
							0, frame->height,
							(u8 *const *)pFrameRGB->data, pFrameRGB->linesize);
		}
  }
}

// av_err2str returns a temporary array, This doesn't work in gcc.
// This function can be used as a replacement for av_err2str.
internal const char *
av_make_error(u32 errnum) {
  static char str[AV_ERROR_MAX_STRING_SIZE];
  memset(str, 0, sizeof(str));
  return av_make_error_string(str, AV_ERROR_MAX_STRING_SIZE, errnum);
}

internal void 
ProcessEvent(XEvent *xev) {
  // NOTE(Ecy): do not call printf here, it will cause segfault or being ignored
  switch (xev->type) {
    case KeyPress:
    {
      XKeyEvent *keyEvent = (XKeyEvent*)xev;

      // NOTE(Ecy): escape key
      if(keyEvent->keycode == 0x09) {
        g_running = false;
      }
    } break;

    case KeyRelease:
    case MotionNotify:
    case ButtonPress:
    case ButtonRelease:
    default:
    {
    } break;
  }
}

// Saves a frame into a PPM format
internal void 
save_frame(u8 *buf, u32 wrap, u32 xsize, u32 ysize, u32 idx) {
  FILE *f;
  u32 i;
  char szFilename[32];
  sprintf(szFilename, "frame%d.raw", idx);
  f = fopen(szFilename, "wb");
  // fprintf(f, "P6\n%d %d\n%d\n", xsize, ysize, 255);

  for (i = 0; i < ysize; i++) {
    fwrite(buf + i * wrap, 1, xsize * 3, f);
  }
  fclose(f);
}

int main(int argc, char *argv[]) {
  char *filename;

  if (argc == 1) {
    printf("Please include a video path.\nEx: ./linux-main './sample.mp4'\n");
    return -1;
  }

  if (argc >= 2) {
    filename = argv[1];
  }

  u32 ret; // avcodec
  u32 response;
  u32 packets = 1; // TODO: Get the actual amount of packets in the video and
                   // set this number to amount of packets in the video
  pFrameRGB = av_frame_alloc();
  u32 video_stream_index = -1;
  int64_t pts;
  
  {
    xdisplay = XOpenDisplay(NULL);
    if (xdisplay == NULL) {
      printf("Error opening X display\n");
      return 0;
    }

    // All events the window accepts
    swa.event_mask = StructureNotifyMask | ExposureMask | PointerMotionMask |
                    KeyPressMask | KeyReleaseMask | ButtonPressMask |
                    ButtonReleaseMask;

    // Get root window
    root = DefaultRootWindow(xdisplay);
    i32 root_x, root_y;
    Window root_again;

    XGetGeometry(xdisplay, root, &root_again, &root_x, &root_y, &root_w, &root_h, &root_border_width, &root_depth);

    // Create window
    window = XCreateWindow(xdisplay, root,
                    0, 0, root_w, root_h, 0,
                    CopyFromParent, InputOutput,
                    CopyFromParent, CWEventMask,
                    &swa);

    XSetWindowAttributes xattr;

    xattr.override_redirect = False;
    XChangeWindowAttributes(xdisplay, window, CWOverrideRedirect, &xattr);

    Atom atomWmDeleteWindow = XInternAtom(xdisplay, "WM_DELETE_WINDOW", false);
    XSetWMProtocols(xdisplay, root, &atomWmDeleteWindow, 1);

    i32 one = 1;
    XChangeProperty(
        xdisplay, window,
        XInternAtom (xdisplay, "_HILDON_NON_COMPOSITED_WINDOW", False),
        XA_INTEGER,  32,  PropModeReplace,
        (unsigned char*) &one, 1);
    
    XWMHints hints;
    hints.input = True;
    hints.flags = InputHint;
    XSetWMHints(xdisplay, window, &hints);

    XMapWindow(xdisplay, window); // make window visible
    XSync(xdisplay, window);
    XStoreName(xdisplay, window, "RPI Emulation");
  }

  // EGL
  EGLDisplay eglDisplay = {};
  EGLSurface eglSurface = {};
  {
    eglDisplay = eglGetDisplay((EGLNativeDisplayType)xdisplay);
    if (eglDisplay == EGL_NO_DISPLAY) {
      printf("Error getting EGL display\n");
      return 1;
    }
    EGLint eglVersionMajor, eglVersionMinor;
    eglInitialize(eglDisplay, &eglVersionMajor, &eglVersionMinor);
    if (!eglInitialize(eglDisplay, &eglVersionMajor, &eglVersionMinor)) {
      printf("Error initializing EGL\n");
      return 1;
    }

    eglBindAPI(EGL_OPENGL_ES_API);

    EGLint configAttributes[] = {EGL_RENDERABLE_TYPE,
                                EGL_OPENGL_ES3_BIT,
                                EGL_SURFACE_TYPE,
                                EGL_WINDOW_BIT,
                                EGL_DEPTH_SIZE,
                                16,
                                EGL_BLUE_SIZE,
                                8,
                                EGL_GREEN_SIZE,
                                8,
                                EGL_RED_SIZE,
                                8,
                                EGL_ALPHA_SIZE,
                                8,
                                EGL_NONE};

    EGLint surfaceAttributes[] = {EGL_NONE};
    EGLint contextAttributes[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};

    EGLint nrOfConfigs;
    EGLConfig windowConfig;
    eglChooseConfig(eglDisplay, configAttributes, &windowConfig, 1, &nrOfConfigs);
    eglSurface = eglCreateWindowSurface(
        eglDisplay, windowConfig, (EGLNativeWindowType)window, surfaceAttributes);

    if (eglSurface == EGL_NO_SURFACE) {
      EGLint error = eglGetError();

      char *errorStr = getErrorStr(error);
      fprintf(stdout, "Could not create EGL surface : %s\n", errorStr);
      return 1;
    }

    EGLContext eglContext =
        eglCreateContext(eglDisplay, windowConfig, NULL, contextAttributes);

    EGLConfig *eglConfigs;
    EGLint eglNumConfigs;
    eglGetConfigs(eglDisplay, NULL, 0, &eglNumConfigs);
    eglConfigs = new EGLConfig[eglNumConfigs];

    eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext);
  }

  // AVcodec stuffs
  // Open file
  AVFormatContext *pFormatContext = NULL;
  avformat_open_input(&pFormatContext, filename, NULL, NULL);
  avformat_find_stream_info(pFormatContext, NULL);

  // Find the first valid video stream
  const AVCodec *pCodec = NULL;
  AVCodecParameters *pCodecParameters = NULL;

  for (u32 i = 0; i < pFormatContext->nb_streams; i++) {
    AVCodecParameters *pLocalCodecParameters =
        pFormatContext->streams[i]->codecpar;
    const AVCodec *pLocalCodec =
        avcodec_find_decoder(pLocalCodecParameters->codec_id);

    // Specific for video and audio
    if (pLocalCodecParameters->codec_type == AVMEDIA_TYPE_VIDEO) {
      if (video_stream_index == -1) {
        video_stream_index = i;
        pCodec = pLocalCodec;
        pCodecParameters = pLocalCodecParameters;
      }
    }
  }

  if (video_stream_index == -1) {
    printf("File %s does not contain a video stream!\n", filename);
    return -1;
  }

  // Setup a codec context for the decoder
  AVCodecContext *pCodecContext = avcodec_alloc_context3(pCodec);
  if (!pCodecContext) {
    printf("Failed to allocate memory for AVCodecContext\n");
    return -1;
  }

  // enum AVHWDeviceType type;
  // fprintf(stderr, "Available device types:");
  // while((type = av_hwdevice_iterate_types(type)) != AV_HWDEVICE_TYPE_NONE)
  //     fprintf(stderr, " %s", av_hwdevice_get_type_name(type));
  // fprintf(stderr, "\n");

  if (avcodec_parameters_to_context(pCodecContext, pCodecParameters) < 0) {
    printf("Failed to copy codec params to codec context\n");
    return -1;
  }

  if (avcodec_open2(pCodecContext, pCodec, NULL) < 0) {
    printf("Failed to open codec through avcodec_open2\n");
    return -1;
  }

  AVFrame *pFrame = av_frame_alloc();
  if (!pFrame) {
    printf("Failed to allocate memory for AVFrame\n");
    return -1;
  }

  AVPacket *pPacket = av_packet_alloc();
  if (!pPacket) {
    printf("Failed to allocate memory for AVPacket\n");
    return -1;
  }

  if (pFrameRGB == NULL) {
    printf("pFrameRGB NULL");
    return -1;
  }

  // Determine required buffer size and allocate buffer
  u32 numBytes = av_image_get_buffer_size(
      AV_PIX_FMT_RGB24, pCodecContext->width, pCodecContext->height, 1);
  u8 *framebuffer = (u8 *)av_malloc(numBytes);

  // Assign appropriate parts of buffer to image planes in pFrameRGB
  // Note that pFrameRGB is an AVFrame, but AVFrame is a superset of
  // AVPicture
  av_image_fill_arrays(pFrameRGB->data, pFrameRGB->linesize, framebuffer,
                       AV_PIX_FMT_RGB24, pCodecContext->width,
                       pCodecContext->height, 1);
  pFrameRGB->width = pCodecContext->width;
  pFrameRGB->height = pCodecContext->height;

  // Read frame and load into texture
  swsCtx = sws_getContext(pCodecContext->width, pCodecContext->height,
                          AV_PIX_FMT_YUV420P, pCodecContext->width,
                          pCodecContext->height, AV_PIX_FMT_RGB24, SWS_BICUBIC,
                          NULL, NULL, NULL);

  r32 vertices[] = {
      // positions x,y,z          // colors           // texture coords
      1.0f,  1.0f,  0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, // top right
      1.0f,  -1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, // bottom right
      -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, // bottom left
      -1.0f, 1.0f,  0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f  // top left
  };

  u32 indices[] = {
      0, 1, 3, // first triangle
      1, 2, 3  // second triangle
  };

  GLuint texture;
  GLint success;
  char infoLog[512];
  GLuint vertexShader;
  GLuint fragShader;
  GLuint shaderProgram;
  GLuint VBO, VAO, EBO;

  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);
  glGenBuffers(1, &EBO);

  glBindVertexArray(VAO);

  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices,
               GL_STATIC_DRAW);

  // position attrib
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(r32), (void *)0);
  glEnableVertexAttribArray(0);
  // color attrib
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(r32),
                        (void *)(3 * sizeof(r32)));
  glEnableVertexAttribArray(1);
  // texture coord attrib
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(r32),
                        (void *)(6 * sizeof(r32)));
  glEnableVertexAttribArray(2);

  const char *vertexShaderSource =
      "#version 310 es\n"
      "precision mediump float;\n"
      "in vec3 aPos;\n"
      "in vec3 color;\n"
      "in vec2 tcoord;\n"
      "out vec3 ocolor;\n"
      "out vec2 otcoord;\n"
      "void main()\n"
      "{\n"
      "   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
      "   ocolor = color;\n"
      "   otcoord = tcoord;\n"
      "}\0";

  vertexShader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
  glCompileShader(vertexShader);

  glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
    printf("ERROR::SHADER::VERTEX::COMPILATION_FAILED\n%s", infoLog);
  }

  const char *fragShaderSource = "#version 310 es\n"
                                 "precision mediump float;\n"
                                 "out vec4 FragColor;\n"
                                 "in vec3 ocolor;\n"
                                 "in vec2 otcoord;\n"
                                 "uniform sampler2D tex;\n"
                                 "void main()\n"
                                 "{\n"
                                 "    FragColor = texture(tex, otcoord);\n"
                                 "}\0";

  fragShader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragShader, 1, &fragShaderSource, NULL);
  glCompileShader(fragShader);
  glGetShaderiv(fragShader, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(fragShader, 512, NULL, infoLog);
    printf("ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n%s", infoLog);
  }

  shaderProgram = glCreateProgram();

  glAttachShader(shaderProgram, vertexShader);
  glAttachShader(shaderProgram, fragShader);
  glLinkProgram(shaderProgram);

  glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
  if (!success) {
    glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
    printf("ERROR::SHADERPROGRAM::COMPILATION_FAILED\n%s", infoLog);
  }

  glUseProgram(shaderProgram);


  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);


  timer appTimer = {};
  appTimer.lastCounter = GetTicks();

  g_running = true;
  while (g_running) {

    // Read input/OS events
    {
      // TODO(Nico): Make this work - doesn't work
      // while (XNextEvent(xdisplay, xev)) {
      //   if (XCheckWindowEvent(xdisplay, window, NULL, xev)) {
      //     process_xevent(xev);
      //   }
      // }

      while(XPending(xdisplay))
      {
        XEvent xev;

        XNextEvent(xdisplay, &xev);
        ProcessEvent(&xev);
      }

      if (!g_running) {
        break;
      }
    }

    if(av_read_frame(pFormatContext, pPacket) >= 0)
    {
      // TODO(Ecy): update next texture if timestamp > currentime

      if(pPacket->stream_index == video_stream_index)
      {
        Decode(pCodecContext, pFrame, pPacket);
      }

      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, pFrameRGB->width, pFrameRGB->height, 0, GL_RGB, GL_UNSIGNED_BYTE, pFrameRGB->data[0]);
      glGenerateMipmap(GL_TEXTURE_2D);
    }


    // Render pipeline
    {
      glViewport(0, 0, root_w, root_h);
      glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
      glClear(GL_COLOR_BUFFER_BIT);

      glUseProgram(shaderProgram);
      glBindVertexArray(VAO);
      glBindTexture(GL_TEXTURE_2D, texture);
      glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    }

    eglSwapBuffers(eglDisplay, eglSurface);

    // Timer update
    {
      appTimer.lastCounter = appTimer.currentCounter;
      appTimer.currentCounter = GetTicks();
    }

  }

  // NOTE(Ecy): Clean Up. This step is usually not necesary
  avformat_close_input(&pFormatContext);
  avformat_free_context(pFormatContext);
  av_frame_free(&pFrame);
  av_packet_free(&pPacket);
  avcodec_free_context(&pCodecContext);

  XDestroyWindow(xdisplay, window);
  XCloseDisplay(xdisplay);

  return 0;
}
