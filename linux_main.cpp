#define GLFW_EXPOSE_NATIVE_EGL
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <stdint.h>
#include <stdlib.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavfilter/avfilter.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
}
#include "linux_main.h"

struct SwsContext *swsCtx = NULL;

bool g_running = false;

// av_err2str returns a temporary array, This doesn't work in gcc.
// This function can be used as a replacement for av_err2str.
static const char *av_make_error(int errnum) {
  static char str[AV_ERROR_MAX_STRING_SIZE];
  memset(str, 0, sizeof(str));
  return av_make_error_string(str, AV_ERROR_MAX_STRING_SIZE, errnum);
}

// Process X11 events
// TODO: Add proper shutdown handling
void process_xevent(XEvent xev) {
  switch (xev.type) {
  case MotionNotify:
    break;
  case KeyRelease:
    break;
  case KeyPress:
    break;
  case ButtonPress:
    break;
  case ButtonRelease:
    break;
  }
}

// Saves a frame into a PPM format
void save_frame(uint8_t *buf, int wrap, int xsize, int ysize, int idx) {
  FILE *f;
  int i;
  char szFilename[32];
  sprintf(szFilename, "frame%d.raw", idx);
  f = fopen(szFilename, "wb");
  // fprintf(f, "P6\n%d %d\n%d\n", xsize, ysize, 255);

  for (i = 0; i < ysize; i++) {
    fwrite(buf + i * wrap, 1, xsize * 3, f);
  }
  fclose(f);
}

int idxx = 0;

int set_buffer(AVPacket *pPacket, AVFormatContext *pFormatContext, uint8_t *buffer, 
                AVFrame *pFrameRGB, int video_stream_index, AVCodecContext *pCodecContext, AVFrame *pFrame) {
    int ret;
    if (pPacket->stream_index != video_stream_index) {
      return -1;
    }

    ret = avcodec_send_packet(pCodecContext, pPacket);
    if (ret < 0) {
      printf("Send->Failed to decode packet: %s\n", av_make_error(ret));
      return -1;
    }

    ret = avcodec_receive_frame(pCodecContext, pFrame);
    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
      
    } else if (ret < 0) {
      printf("Receive->Failed to decode packet: %s\n", av_make_error(ret));
      return -1;
    }

    sws_scale(swsCtx, (uint8_t const *const *)pFrame->data, pFrame->linesize, 0,
              pCodecContext->height, pFrameRGB->data, pFrameRGB->linesize);

    buffer = pFrameRGB->data[0];
    //printf("%s", pFrameRGB->data[0]);
    

    // save the frame to disk
    if (++idxx <= 5) {
      save_frame(pFrameRGB->data[0], pFrameRGB->linesize[0], pFrameRGB->width,
                 pFrameRGB->height, idxx);
    }

  return 1;
}

static int decode_packet(AVPacket *pPacket, AVCodecContext *pCodecContext,
                         AVFrame *pFrame, unsigned char *bufs[]) {
  // Supply raw packet data as input to a decoder
  int response = avcodec_send_packet(pCodecContext, pPacket);

  if (response < 0) {
    printf("Error while sending a packet to the decoder: %s\n",
           av_make_error(response));
    return response;
  }

  int index = 0;

  while (response >= 0) {
    // Return decoded output data (into a frame) from a decoder
    response = avcodec_receive_frame(pCodecContext, pFrame);
    if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
      break;
    } else if (response < 0) {
      printf("Error while receiving a frame from the decoder: %s",
             av_make_error(response));
      return response;
    }

    // do stuff with frame
    bufs[pFrame->coded_picture_number] = pFrame->data[0];
    index++;
  }

  return 0;
}

int main() {

  // Setup X11 & EGL
  Window root;
  Window window;
  Display *xdisplay;
  XSetWindowAttributes swa;

  // Open standard display (primary screen)
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
  // Create window
  window = XCreateWindow(xdisplay, root, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, 0,
                         0, 0, NULL, 0, &swa);

  XMapWindow(xdisplay, window); // make window visible
  XSync(xdisplay, window);
  XStoreName(xdisplay, window, "RPI Emulation");

  // EGL
  EGLDisplay eglDisplay = eglGetDisplay((EGLNativeDisplayType)xdisplay);
  if (eglDisplay == EGL_NO_DISPLAY) {
    printf("Error getting EGL display\n");
    return 0;
  }
  EGLint eglVersionMajor, eglVersionMinor;
  eglInitialize(eglDisplay, &eglVersionMajor, &eglVersionMinor);
  if (!eglInitialize(eglDisplay, &eglVersionMajor, &eglVersionMinor)) {
    printf("Error initializing EGL\n");
    return 0;
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
  EGLSurface eglSurface = eglCreateWindowSurface(
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
  int eglNumConfigs;
  eglGetConfigs(eglDisplay, NULL, 0, &eglNumConfigs);
  eglConfigs = new EGLConfig[eglNumConfigs];

  eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext);

  // AVcodec stuffs
  // Open file
  char *filename = "./sample.mp4";
  AVFormatContext *pFormatContext = NULL;
  avformat_open_input(&pFormatContext, filename, NULL, NULL);
  avformat_find_stream_info(pFormatContext, NULL);

  // Find the first valid video stream
  const AVCodec *pCodec = NULL;
  AVCodecParameters *pCodecParameters = NULL;
  int video_stream_index = -1;

  for (int i = 0; i < pFormatContext->nb_streams; i++) {
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

  int response = 0;
  //int packets = 8; // TODO: Get the actual amount of packets in the video and
                   // set this number to amount of packets in the video
  unsigned char *bufs[8];

  // while (av_read_frame(pFormatContext, pPacket) >= 0) {
  //   // if it's the video stream
  //   if (pPacket->stream_index == video_stream_index) {
  //     response = decode_packet(pPacket, pCodecContext, pFrame, bufs);
  //     if (response < 0)
  //       break;
  //     // stop it
  //     if (--packets <= 0)
  //       break;
  //   }
  //
  //   av_packet_unref(pPacket);
  //   break;
  // }

  // Unpack the luminense data and set it in RGB format
  unsigned char *data = new unsigned char[pFrame->width * pFrame->height * 3];
  for (int x = 0; x < pFrame->width; ++x) {
    for (int y = 0; y < pFrame->height; ++y) {
      data[y * pFrame->width * 3 + x * 3] =
          pFrame->data[0][y * pFrame->linesize[0] + x];
      data[y * pFrame->width * 3 + x * 3 + 1] =
          pFrame->data[0][y * pFrame->linesize[0] + x];
      data[y * pFrame->width * 3 + x * 3 + 2] =
          pFrame->data[0][y * pFrame->linesize[0] + x];
    }
  }

  int frame_width;
  int frame_height;
  unsigned char *frame_data;

  frame_width = pFrame->width;
  frame_height = pFrame->height;
  frame_data = data;

  uint8_t *frameData;
  if (posix_memalign((void **)&frameData, 128,
                     frame_width * frame_height * 4) != 0) {
    printf("Couldn't allocate frame buffer\n");
    return 1;
  }

  // avformat_close_input(&pFormatContext);
  // avformat_free_context(pFormatContext);
  // av_frame_free(&pFrame);
  // av_packet_free(&pPacket);
  // avcodec_free_context(&pCodecContext);

  GLuint tex_handle;
  glGenTextures(1, &tex_handle);
  glBindTexture(GL_TEXTURE_2D, tex_handle);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

  AVFrame *pFrameRGB = NULL;
  pFrameRGB = av_frame_alloc();
  if (pFrameRGB == NULL) {
    printf("pFrameRGB NULL");
    return -1;
  }

  // Detmine required buffer size and allocate buffer
  int numBytes = av_image_get_buffer_size(
      AV_PIX_FMT_RGB24, pCodecContext->width, pCodecContext->height, 1);
  unsigned char *framebuffer = (uint8_t *)av_malloc(numBytes);

  // Assign appropriate parts of buffer to image planes in pFrameRGB
  // Note that pFrameRGB is an AVFrame, but AVFrame is a superset of
  // AVPicture
  av_image_fill_arrays(pFrameRGB->data, pFrameRGB->linesize, framebuffer,
                       AV_PIX_FMT_RGB24, pCodecContext->width,
                       pCodecContext->height, 1);
  pFrameRGB->width = pCodecContext->width;
  pFrameRGB->height = pCodecContext->height;

  // Read frame and load into texture
  int64_t pts;
  int ret;

  swsCtx = sws_getContext(pCodecContext->width, pCodecContext->height,
                          AV_PIX_FMT_YUV420P, pCodecContext->width,
                          pCodecContext->height, AV_PIX_FMT_RGB24, SWS_BICUBIC,
                          NULL, NULL, NULL);

  // if (!swsCtx)
  // {
  //   printf("Couldn't initialize sw scaler\n");
  //   return false;
  // }

  int idx = 0;
  // while (av_read_frame(pFormatContext, pPacket) >= 0) {
  //   if (pPacket->stream_index != video_stream_index) {
  //     continue;
  //   }
  //
  //   ret = avcodec_send_packet(pCodecContext, pPacket);
  //   if (ret < 0) {
  //     printf("Send->Failed to decode packet: %s\n", av_make_error(ret));
  //     break;
  //   }
  //
  //   ret = avcodec_receive_frame(pCodecContext, pFrame);
  //   if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
  //     continue;
  //   } else if (ret < 0) {
  //     printf("Receive->Failed to decode packet: %s\n", av_make_error(ret));
  //     break;
  //   }
  //
  //   sws_scale(swsCtx, (uint8_t const *const *)pFrame->data, pFrame->linesize, 0,
  //             pCodecContext->height, pFrameRGB->data, pFrameRGB->linesize);
  //
  //   // save the frame to disk
  //   // if (++idx <= 5) {
  //   //   save_frame(pFrameRGB->data[0], pFrameRGB->linesize[0], pFrameRGB->width,
  //   //              pFrameRGB->height, idx);
  //   // }
  // }

  int packets = 1;

  uint8_t *frameBuffer;
  while (av_read_frame(pFormatContext, pPacket) >= 0) {
    set_buffer(pPacket, pFormatContext, frameBuffer, pFrameRGB, video_stream_index, pCodecContext, pFrame);

    if (--packets <= 0)
      break;
  }

  //printf("data->%s", pFrameRGB->data[0] + 1 * pFrameRGB->linesize[0]);
  //set_buffer(pPacket, pFormatContext, frameBuffer, pFrameRGB, video_stream_index, pCodecContext, pFrame);

  // float vertices[] = {
  //   // first triangle
  //   0.5f, 0.5f, 0.0f, // top right
  //   0.5f, -0.5f, 0.0f, // bottom right
  //   -0.5f, 0.5f, 0.0f, // top left
  //   // second triangle
  //   0.5f, -0.5f, 0.0f, // bottom right
  //   -0.5f, -0.5f, 0.0f, // bottom left
  //   -0.5f, 0.5f, 0.0f, // top left
  // };

  // float vertices[] = {
  //   0.5f, 0.5f, 0.0f, // top right
  //   0.5f, -0.5f, 0.0f, // bottom right
  //   -0.5f, -0.5f, 0.0f, // bottom left
  //   -0.5f, 0.5f, 0.0f, //top left
  // };

  float vertices[] = {
    // positions x,y,z          // colors           // texture coords
     1.0f,  1.0f, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 0.0f,   // top right
     1.0f, -1.0f, 0.0f,   0.0f, 1.0f, 0.0f,   1.0f, 1.0f,   // bottom right
    -1.0f, -1.0f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 1.0f,   // bottom left
    -1.0f,  1.0f, 0.0f,   1.0f, 1.0f, 0.0f,   0.0f, 0.0f    // top left 
};

  unsigned int indices[] = {
    0, 1, 3, // first triangle
    1, 2, 3 // second triangle
  };


  unsigned int VBO, VAO, EBO;
  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);
  glGenBuffers(1, &EBO);
  
  glBindVertexArray(VAO);

  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

  // position attrib
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);
  // color attrib
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);
  // texture coord attrib
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
  glEnableVertexAttribArray(2);

  // shader
  const char *vertexShaderSource = "#version 310 es\n"
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

  unsigned int vertexShader;
  vertexShader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
  glCompileShader(vertexShader);

  int success;
  char infoLog[512];
  glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
    printf("ERROR::SAHDER::VERTEX::COMPILATION_FAILED\n%s", infoLog);
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

  unsigned int fragShader;
  fragShader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragShader, 1, &fragShaderSource, NULL);
  glCompileShader(fragShader);
  glGetShaderiv(fragShader, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(fragShader, 512, NULL, infoLog);
    printf("ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n%s", infoLog);
  }

  unsigned int shaderProgram;
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

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  int swidth, sheight, nrChannels;
  //unsigned char *sdata = stbi_load("./frame1.ppm", &swidth, &sheight, &nrChannels, 0);
  FILE *fpp = fopen("./frame1.raw", "rb");
  fseek(fpp, 0, SEEK_END);
  long fsize = ftell(fpp);
  fseek(fpp, 0, SEEK_SET);

  unsigned char *fstring = (unsigned char*)malloc(fsize + 1);
  fread(fstring, fsize, 1, fpp);
  printf("file -> %d", fsize);
  fclose(fpp);
  fstring[fsize] = 0;


  unsigned int texture;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, swidth, sheight, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
  //glGenerateMipmap(GL_TEXTURE_2D);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, pFrameRGB->width, pFrameRGB->height, 0, GL_RGB, GL_UNSIGNED_BYTE, pFrameRGB->data[0] + 0 * pFrameRGB->linesize[0]);
  glGenerateMipmap(GL_TEXTURE_2D);

  // unsigned int EBO;
  // glGenBuffers(1, &EBO);
  // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  // glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);


  // X11 loop
  int frameFinished;
  g_running = true;
  while (g_running) {
    int keycode;
    XEvent xev;

    if (!g_running) {
      break;
    }

    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // bind texture
    glBindTexture(GL_TEXTURE_2D, texture);

    // render
    glUseProgram(shaderProgram);
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    
    eglSwapBuffers(eglDisplay, eglSurface);

    // TODO: Make this work - doesn't work
    if (XPending(xdisplay)) {
      if (XCheckWindowEvent(xdisplay, window, NULL, &xev)) {
        process_xevent(xev);
      }
    }
  }

  XDestroyWindow(xdisplay, window);
  XCloseDisplay(xdisplay);

  return 0;
}
