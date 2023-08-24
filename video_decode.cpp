#include "video_decode.h"

internal void
MP4VideoDemuxer(debug_read_file_result *file)
{
	u8 *cursor = (u8*)file->contents;
	u64 size = file->contentSize;
	u32 end = cursor + size;
	
	while(cursor < end)
	{
		u32 boxSize = 0;
		
		demux_mp4_box box = {};
		demux_mp4_box.header = *((demux_mp4_box_header*)cursor);
		cursor += sizeof(demux_mp4_box_header);
		
		switch(box.header.size)
		{
			case 0:
			{
				// NOTE(Ecy): last box, size extends until the end of the file
				boxSize = end - cursor;
			}break;
			
			case 1:
			{
				// NOTE(Ecy): Large size is read instead
				box.largesize = *((u64*)cursor);
				cursor += sizeof(u64);
				
				boxSize = box.largesize;
			}break;
			
			default:
			{
				boxSize = box.header.size;
			}break;
		}
		
		switch(box.header.type)
		{
			case demux_mp4_box_codes[DEMUX_MP4_BOX_FTYP]:
			{
				
			}break;
			
			default:
			{
				// Unhandle type skip
			}break;
		}
		
		cursor += boxSize;
	}
}

// NOTE(Ecy): return type is temporary
internal void
LoadVideoContext(video_decode *decoder, char *filename)
{
	if(avformat_open_input(&decoder->formatContext, filename, NULL, 0) != 0)
	{
		// Couldn't open file
		return;
	}
	
	if(avformat_find_stream_info(decoder->formatContext, NULL) < 0)
	{
		// Couldn't find stream information
		return;
	}
	
	AVCodecParameters *pCodecParam = NULL;
	
	decoder->streamIndex = -1;
	for(u32 index = 0;
		index < decoder->formatContext->nb_streams;
		++index)
	{
		AVCodecParameters *pLocalCodecParameters = decoder->formatContext->streams[index]->codecpar;
		const AVCodec *pLocalCodec = avcodec_find_decoder(pLocalCodecParameters->codec_id);
		
		if(pLocalCodecParameters->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			decoder->streamIndex = index;
			decoder->codec = (AVCodec*)pLocalCodec;
			pCodecParam = pLocalCodecParameters;
			break;
		}
	}
	if(decoder->streamIndex == -1)
	{
		// NO video stream found.
		return;
	}
	
	decoder->codecContext = avcodec_alloc_context3(decoder->codec);
	
	if(decoder->codecContext == NULL)
	{
		// Copy context failed
		return;
	}
	
	if(avcodec_parameters_to_context(decoder->codecContext, pCodecParam) < 0)
	{
		// Copy context failed
		return;
	}
	
	if(avcodec_open2(decoder->codecContext, decoder->codec, NULL) < 0)
	{
		// Could not open codec
		return;
	}
	
	decoder->pFrame    = av_frame_alloc();
	decoder->pFrameRGB = av_frame_alloc();
	decoder->packet    = av_packet_alloc();
	
	decoder->swsCtx = sws_getContext(decoder->codecContext->width, decoder->codecContext->height, AV_PIX_FMT_YUV420P, 
							decoder->codecContext->width, decoder->codecContext->height, AV_PIX_FMT_RGB24,
							SWS_BICUBIC, NULL, NULL, NULL);
	
	u32 num_bytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, decoder->codecContext->width, decoder->codecContext->height, 1);
	unsigned char* frame_buffer = (u8*)av_malloc(num_bytes);
	av_image_fill_arrays(decoder->pFrameRGB->data,       //uint8_t *dst_data[4], 
						 decoder->pFrameRGB->linesize,   //int dst_linesize[4],
						 frame_buffer,          //const uint8_t * src,
						 AV_PIX_FMT_RGB24,      //enum AVPixelFormat pix_fmt,
						 decoder->codecContext->width,   //int width, 
						 decoder->codecContext->height,  //int height,
						 1);                    //int align);
	
	decoder->pFrameRGB->width = decoder->codecContext->width;
	decoder->pFrameRGB->height = decoder->codecContext->height;
	
	if(!decoder->swsCtx)
	{
		// no scaler context found
		return;
	}

	//auto descriptor = av_pix_fmt_desc_get(AV_PIX_FMT_NV12);
	
	decoder->isLoaded = true;
}

internal void
Decode(video_decode *decoder)
{
    int ret;
	
	char buffer[1024];
    ret = avcodec_send_packet(decoder->codecContext, decoder->packet);
	
	av_strerror(ret, buffer, 1024);
	
    if (ret < 0) {
        fprintf(stderr, "Error sending a packet for decoding\n");
        return;
    }
	
	decoder->pFrame = av_frame_alloc();
    while (ret >= 0) {
		ret = avcodec_receive_frame(decoder->codecContext, decoder->pFrame);
		
		av_strerror(ret, buffer, 1024);
		
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            return;
        else if (ret < 0) {
            fprintf(stderr, "Error during decoding\n");
            return;
        }
		
		if(ret >= 0)
		{
			ret = sws_scale(decoder->swsCtx, (const u8* const*)decoder->pFrame->data, decoder->pFrame->linesize,
							0, decoder->pFrame->height,
							(u8 *const *)decoder->pFrameRGB->data, decoder->pFrameRGB->linesize);
		}
    }
}

internal void
UpdateDecode(video_decode *decoder)
{
	if(av_read_frame(decoder->formatContext, decoder->packet) >= 0)
	{
		if(decoder->packet->stream_index == decoder->streamIndex)
		{
			Decode(decoder);
		}
		
		av_packet_unref(decoder->packet);
		av_frame_unref(decoder->pFrame);
	}
}

internal void
FreeDecode(video_decode *decode)
{
	avformat_close_input(&decode->formatContext);
	avformat_free_context(decode->formatContext);
	av_frame_free(&decode->pFrame);
	av_packet_free(&decode->packet);
	avcodec_free_context(&decode->codecContext);
	
	av_frame_unref(decode->pFrame);
	av_frame_unref(decode->pFrameRGB);
	av_packet_free(&decode->packet);
}