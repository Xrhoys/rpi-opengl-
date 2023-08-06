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
#include <libavutil/hwcontext.h>
#ifdef __cplusplus
}
#endif


struct video_decode
{
	b32               isLoaded;
	struct SwsContext *swsCtx;
	
	i32             streamIndex;
	AVFormatContext *formatContext;
	AVCodec         *codec;
	AVCodecContext  *codecContext;
	
	AVFrame         *pFrameDest;
	AVFrame         *pFrame;
	AVPacket        *packet;
	
	u8              *frameData;
};


#endif //VIDEO_DECODE_H
