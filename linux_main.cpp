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

    if (response >= 0) {
      // do stuff with frame
      bufs[pFrame->coded_picture_number] = pFrame->data[0];
      index++;
    }
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
  char *filename = "./sample.mp4";
  AVFormatContext *pFormatContext = NULL;
  avformat_open_input(&pFormatContext, filename, NULL, NULL);
  avformat_find_stream_info(pFormatContext, NULL);

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
    } else if (pLocalCodecParameters->codec_type == AVMEDIA_TYPE_AUDIO) {
    }

    // General
  }

  if (video_stream_index == -1) {
    printf("File %s does not contain a video stream!\n", filename);
    return -1;
  }

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
  int packets = 8; // TODO: Get the actual amount of packets in the video and
                   // set this number to amount of packets in the video
  unsigned char *bufs[8];

  while (av_read_frame(pFormatContext, pPacket) >= 0) {
    // if it's the video stream
    if (pPacket->stream_index == video_stream_index) {
      response = decode_packet(pPacket, pCodecContext, pFrame, bufs);
      if (response < 0)
        break;
      // stop it
      if (--packets <= 0)
        break;
    }
  }

  GLuint VA0;
  GLuint shaderProgram;
  GLuint VB0;
  GLuint EB0;
  float vertices[] = {
      1.0f,  1.0f,  0.0f, // top right
      1.0f,  -1.0f, 0.0f, // bottom right
      -1.0f, -1.0f, 0.0f, // bottom left
      -1.0f, 1.0f,  0.0f, // top left
  };

  unsigned int indices[] = {
      0, 1, 3, // first triangle
      1, 2, 3  // second triangle
  };
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
  int success;
  char infoLog[512];
  glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
    fprintf(stdout, "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n%s", infoLog);
  }

  const char *fragmentShaderSource =
      "#version 310 es\n"
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
  if (!success) {
    glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
    fprintf(stdout, "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n%s", infoLog);
  }

  shaderProgram = glCreateProgram();

  glAttachShader(shaderProgram, vertexShader);
  glAttachShader(shaderProgram, fragmentShader);
  glLinkProgram(shaderProgram);

  // Check ShaderCompile success
  glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
  if (!success) {
    glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
    fprintf(stdout, "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n%s", infoLog);
  }

  // glDeleteShader(vertexShader);
  // glDeleteShader(fragmentShader);

  glGenVertexArrays(1, &VA0);
  glBindVertexArray(VA0);
  glGenBuffers(1, &VB0);

  glBindBuffer(GL_ARRAY_BUFFER, VB0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EB0);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices,
               GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EB0);

  // glEnable(GL_DEPTH_TEST);

  // X11 loop
  g_running = true;
  while (g_running) {
    int keycode;
    XEvent xev;

    if (!g_running) {
      break;
    }

    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(shaderProgram);
    glBindVertexArray(VA0);

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
