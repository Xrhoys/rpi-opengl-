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

static int decode_packet(AVPacket *pPacket, AVCodecContext *pCodecContext, AVFrame *pFrame, unsigned char *bufs[]) {
  // Supply raw packet data as input to a decoder
  int response = avcodec_send_packet(pCodecContext, pPacket);

  if (response < 0) {
    printf("Error while sending a packet to the decoder: %s\n", av_make_error(response));
    return response;
  }

  int index = 0;

  while (response >= 0) {
    // Return decoded output data (into a frame) from a decoder
    response = avcodec_receive_frame(pCodecContext, pFrame);
    if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
      break;
    } else if (response < 0) {
      printf("Error while receiving a frame from the decoder: %s", av_make_error(response));
      return response;
    }

    if (response >= 0) {
      printf("Frame %c (%d) pts %d dts %d key_frame %d [coded_picture_number %d, display_picture_number %d]\n",
            av_get_picture_type_char(pFrame->pict_type),
             pCodecContext->frame_number,
             pFrame->pts,
             pFrame->pkt_dts,
             pFrame->key_frame,
             pFrame->coded_picture_number,
             pFrame->display_picture_number);

      char frame_filename[1024];
      snprintf(frame_filename, sizeof(frame_filename), "%s-%d.pgm", "frame", pCodecContext->frame_number);
      if (pFrame->format != AV_PIX_FMT_YUV420P) {
        printf("Warning:\n");
      }

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
  AVFormatContext *pFormatContext = avformat_alloc_context();
  avformat_open_input(&pFormatContext, filename, NULL, NULL);
  printf("Format %s, duration %lld us\n", pFormatContext->iformat->long_name, pFormatContext->duration);
  avformat_find_stream_info(pFormatContext, NULL);

  const AVCodec *pCodec = NULL;
  AVCodecParameters *pCodecParameters = NULL;
  int video_stream_index = -1;

  for (int i = 0; i < pFormatContext->nb_streams; i++) {
    AVCodecParameters *pLocalCodecParameters = pFormatContext->streams[i]->codecpar;
    const AVCodec *pLocalCodec = avcodec_find_decoder(pLocalCodecParameters->codec_id);

    // Specific for video and audio
    if (pLocalCodecParameters->codec_type == AVMEDIA_TYPE_VIDEO) {
      if (video_stream_index == -1) {
        video_stream_index = i;
        pCodec = pLocalCodec;
        pCodecParameters = pLocalCodecParameters;
      }
      printf("Video Codec: Resolution %d x %d\n", pLocalCodecParameters->width, pLocalCodecParameters->height);
    } else if (pLocalCodecParameters->codec_type == AVMEDIA_TYPE_AUDIO) {
      printf("Audio Codec: %d channels, sample rate %d\n", pLocalCodecParameters->channels, pLocalCodecParameters->sample_rate);
    }

    // General
    printf("\tCodec %s ID %d bit_rate %lld\n", pLocalCodec->long_name, pLocalCodec->id, pLocalCodecParameters->bit_rate);
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
  int packets = 8; // TODO: Get the actual amount of packets in the video and set this number to amount of packets in the video
  unsigned char *bufs[8];

  while (av_read_frame(pFormatContext, pPacket) >= 0) {
    // if it's the video stream
    if (pPacket->stream_index == video_stream_index) {
      response = decode_packet(pPacket, pCodecContext, pFrame, bufs);
      if (response < 0) break;
      // stop it
      if(--packets <= 0) break;
    }
  }

  // X11 loop
  while (1) {
    int keycode;
    XEvent xev;

    if (XPending(xdisplay)) {
      if (XCheckWindowEvent(xdisplay, window, NULL, &xev)) {
        process_xevent(xev);
      }

      eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext);

      eglSwapBuffers(eglDisplay, eglSurface);
    }
  }

  return 0;
}