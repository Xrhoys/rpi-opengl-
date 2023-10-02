/* date = July 12th 2023 11:45 pm */

#ifndef VIDEO_DECODE_H
#define VIDEO_DECODE_H

#ifdef __cplusplus
extern "C" {
#endif
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
	
#include <libavutil/hwcontext_drm.h>
#include <libavutil/hwcontext_vaapi.h>
	
#ifdef __cplusplus
}
#endif

#include <libdrm/drm_fourcc.h>
#include <va/va_drmcommon.h>

#define DECODE_QUEUE_SIZE 32

#if RPI
#define DECODE_HW_DEVICE_TYPE AV_HWDEVICE_TYPE_DRM
#else
#define DECODE_HW_DEVICE_TYPE AV_HWDEVICE_TYPE_VAAPI
#endif


static AVPixelFormat hw_pix_fmt;

struct video_decode;

struct video_decode_unit
{
	i32 frameId;
	
	b32      isReady;
	
    AVFrame  *frame;
	AVPacket *packet;
	
	EGLImageKHR eglImage;
	
	video_decode *decoder;
};

struct video_decode
{
	b32             isLoaded;
	
	i32             streamIndex;
	AVFormatContext *formatContext;
	AVCodec         *codec;
	AVCodecContext  *codecContext;
	
	AVFrame         *pFrame;
	AVPacket        *packet;
	
	AVPixelFormat   hw_pix_fmt;
	AVBufferRef     *hw_device_ctx;
	
    AVHWDeviceContext    *hwContext;
	
    AVVAAPIDeviceContext *vaDeviceContext;
    VADisplay             *vaDisplay;
    
    u32 frameCount;
	u32 displayedFrameId;
	u32 frameIdCounter_;
	
	video_decode_unit framePool[DECODE_QUEUE_SIZE];
};

inline void
QueueNextFrame()
{
	
}

inline video_decode_unit*
GetFrameHandle(video_decode *ctx, i32 id)
{
	for(u32 index = 0;
		index < DECODE_QUEUE_SIZE;
		++index)	
	{
		if(ctx->framePool[index].frameId == id)
		{
			return &ctx->framePool[index];
		}
	}
	
	// NOTE(Ecy): this should not normally happen
	return nullptr;
}

inline AVPixelFormat 
get_hw_format(AVCodecContext *ctx, const enum AVPixelFormat *pix_fmts)
{
    const enum AVPixelFormat *p;
	
    for (p = pix_fmts; *p != -1; p++) {
        if (*p == hw_pix_fmt)
        {
            // fprintf(stderr, "surface format: %s\n", av_get_pix_fmt_name(*p));
            return *p;
        }
    }
	
    fprintf(stderr, "Failed to get HW surface format.\n");
    return AV_PIX_FMT_NONE;
}

inline int 
hw_decoder_init(video_decode *ctx, AVHWDeviceType type)
{
    int err = 0;
	
    if ((err = av_hwdevice_ctx_create(&ctx->hw_device_ctx, type,
                                      NULL, NULL, 0)) < 0) {
		fprintf(stderr, "Failed to create specified HW device.\n");
        return err;
    }
    ctx->codecContext->hw_device_ctx = av_buffer_ref(ctx->hw_device_ctx);
#if RPI
	
#else
	// NOTE(Xrhoys): default VA device
	const char *drm_node = "/dev/dri/renderD128";
	
	AVHWDeviceContext *hwctx = (AVHWDeviceContext*)ctx->codecContext->hw_device_ctx->data;
	AVVAAPIDeviceContext *vactx = (AVVAAPIDeviceContext*)hwctx->hwctx;
	
	ctx->vaDisplay = (VADisplay*)vactx->display;
#endif
	
    return err;
}

#endif //VIDEO_DECODE_H
