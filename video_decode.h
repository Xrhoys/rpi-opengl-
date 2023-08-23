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
	
	AVFrame         *pFrameRGB;
	AVFrame         *pFrame;
	AVPacket        *packet;
};

struct demux_mp4_box_header
{
	// NOTE(Ecy): big-endian, network format
		// NOTE(Ecy): if size == 1, largesize is read, if size = 0, last box, so goes all the way until EOF
	u32 size;
	u32 type;
	u64 largesize;
	char userType[16];
};

struct demux_mp4_box
{
	demux_mp4_box_header header;
	char *data;
};


#endif //VIDEO_DECODE_H
