#include "video_decode.h"

internal void
ParseH265Stream(u8 *stream)
{
		
}

internal u32
ParseDinf(demux_mp4_box_dinf *dinf, u8 *cursor)
{
	demux_mp4_box_header header = {};
	u32 headerSize = ParseDemuxMP4Header(&header, cursor);
	u8 *end = cursor + header.size;

#if 0	
	cursor += headerSize;
	
	while(cursor < end)
	{
		demux_mp4_box_dref *dref = &box->dref[box->boxCount++];
		headerSize = ParseDemuxMP4HeaderFull(&dref->header, cursor);
		
		cursor += headerSize;
		
		dref->entryCount = _byteSwapU32(cursor);
		cursor += sizeof(u32);
		
		for(u32 index = 0;
			index < dref->entryCount;
			++index)
		{
			// 
		}
	}
#endif
	
	return end - cursor;
}

internal u32
ParseMinf(demux_mp4_box_minf *box, u8 *cursor) 
{
	demux_mp4_box_header header = {};
	u32 headerSize = ParseDemuxMP4Header(&header, cursor);
	u8 *end = cursor + header.size;
	
	cursor += headerSize;
	
	while(cursor < end)
	{
		switch(header.type)
		{
			case DEMUX_MP4_BOX_VMHD:
			{
				memcpy(&box->vmhd.header, &header, sizeof(header));
				
				box->vmhd.graphicsMode = *(u16*)cursor++;
				memcpy(&box->vmhd.opColor, &cursor, sizeof(box->vmhd.opColor));
			}break;
			case DEMUX_MP4_BOX_SMHD:
			{
				memcpy(&box->smhd.header, &header, sizeof(header));
				
				box->smhd.balance = *(u16*)cursor;
				cursor += sizeof(u32);
			}break;
			case DEMUX_MP4_BOX_HMHD:
			{
				memcpy(&box->hmhd.header, &header, sizeof(header));
				
				box->hmhd.maxPDUsize = *(u16*)cursor++;
				box->hmhd.avgPDUsize = *(u16*)cursor++;
				box->hmhd.maxbitrate = *(u32*)cursor++;
				box->hmhd.avgbitrate = *(u32*)cursor++;
			}break;
			case DEMUX_MP4_BOX_STHD:
			{
				memcpy(&box->hmhd.header, &header, sizeof(header));
				
				cursor += box->hmhd.header.size;
			}break;
			case DEMUX_MP4_BOX_NMHD:
			{
				//memcpy(&box->nmhd.header, &header, sizeof(header));
				
				cursor += header.size;
			}break;
			case DEMUX_MP4_BOX_DINF:
			{
				box->dinf.header = header;
				
				cursor += ParseDinf(&box->dinf, cursor);
			}break;
			default: break;
		}
		
	}
	
	return end - cursor;
}

internal u32
ParseMdia(demux_mp4_box_mdia *box, u8 *cursor) 
{
	demux_mp4_box_header header = {};
	u32 headerSize = ParseDemuxMP4Header(&header, cursor);
	u8 *end = cursor + header.size;
	
	cursor += headerSize;
	
	switch(header.type)
	{
		case DEMUX_MP4_BOX_MDHD:
		{
			// TODO(Ecy): Parse mdhd
		}break;
		case DEMUX_MP4_BOX_HDLR:
		{
			
		}break;
		case DEMUX_MP4_BOX_ELNG:
		{

		}break;
		case DEMUX_MP4_BOX_MINF:
		{
			box->minf.header = header;

			cursor += ParseMinf(&box->minf, cursor);
		}break;
		case DEMUX_MP4_BOX_STBL:
		{
			// THE MOST IMPORTANT METADATA!
		}break;
		default: break;
	}

	return end - cursor;
}

internal u32
ParseTrak(demux_mp4_box_trak *box, u8 *cursor)
{
	demux_mp4_box_header header = {};
	u32 headerSize = ParseDemuxMP4Header(&header, cursor);
	u8 *end = cursor + header.size;
	
	cursor += headerSize;
	
	while(cursor > end) 
	{
		switch(header.type)
		{
			case DEMUX_MP4_BOX_TKHD:
			{
				memcpy(&box->tkhd.header, &header, sizeof(header));

				// Parse TKHD
				
			}break;
			case DEMUX_MP4_BOX_TREF:
			{

			}break;
			case DEMUX_MP4_BOX_TRGR:
			{

			}break;
			case DEMUX_MP4_BOX_MDIA:
			{
				box->mdia.header = header;

				cursor += ParseMdia(&box->mdia, cursor);
			}break;
			default: break;
		}

	}

	return end - cursor;
}

internal u32
ParseMOOV(demux_mp4_box_moov *box, u8 *cursor)
{
	demux_mp4_box_header header = {};
	u32 headerSize = ParseDemuxMP4Header(&header, cursor);
	u8 *end = cursor + header.size;
	
	cursor += headerSize;
	
	while(cursor > end)
	{
		switch(header.type)
		{
			case DEMUX_MP4_BOX_MVHD:
			{
				// cursor += ParseDemuxMP4HeaderFull(&box->mvhd.header, cursor);
			}break;
			case DEMUX_MP4_BOX_TRAK:
			{
				demux_mp4_box_trak *trak = &box->trak[box->trakCount++];
				trak->header = header;

				cursor += ParseTrak(trak, cursor);
			}break;
			default:
			{

			}break;
		}
	}

	return end - cursor;
}

internal void
MP4VideoDemuxer(debug_read_file_result *file)
{
	u8 *boxCursor = (u8*)file->contents;
	u8 *end = boxCursor + file->contentSize;
	
	while(boxCursor < end)
	{
		u8 *cursor = boxCursor;
		u32 boxSize = 0;
		
		demux_mp4_box_header header = {};
		cursor += ParseDemuxMP4Header(&header, cursor);
		
		switch(header.size)
		{
			case 0:
			{
				// NOTE(Ecy): last box, size extends until the end of the file
				boxSize = end - cursor;
			}break;
			
			case 1:
			{
				// NOTE(Ecy): Large size is read instead
				header.largesize = _byteSwapU64(*((u64*)cursor));
				cursor += sizeof(u64);
				
				boxSize = header.largesize;
			}break;
			
			default: 
			{
				boxSize = header.size;
			}break;
		}
		
		switch(header.type)
		{
			case DEMUX_MP4_BOX_FTYP:
			{
				demux_mp4_box_ftyp box = {};
				box.majorBrand = *((u32*)cursor);
				cursor += sizeof(u32);
				box.minorVersion = _byteSwapU32(*((u32*)cursor));
				cursor += sizeof(u32);
				
				box.compatibleBrand = (char*)cursor;
				box.brandCharSize   = boxSize - (cursor - boxCursor);
			}break;
			
			case DEMUX_MP4_BOX_MOOV:
			{
				demux_mp4_box_moov box = {};
				memcpy(&box.header, &header, sizeof(header));

				cursor += ParseMOOV(&box, cursor);
			}break;
			
			default:
			{
				// Unhandle type skip
			}break;
		}
		
		boxCursor += boxSize;
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

#ifdef BE_SOFTWARE	
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
#endif

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
#ifdef BE_SOFTWARE
		if(ret >= 0)
		{
			ret = sws_scale(decoder->swsCtx, (const u8* const*)decoder->pFrame->data, decoder->pFrame->linesize,
							0, decoder->pFrame->height,
							(u8 *const *)decoder->pFrameRGB->data, decoder->pFrameRGB->linesize);
		}
#eflif BE_VULKAN
		if(ret >= 0)
		{
			// Vulkan decoding process:
			// 2 - Decoding: get frame data from index, set decodedFrame *lastDecodedFrame = null
			// 3 - if lastDecodedFrame != null, and queryPool of the frame not null => vkGetQueryPoolResults
			// 4 - else test lastDecodedFrame->frameCompleteFence different VkFence, wait for fence then get fence status
			// 5 - after 3 and 4, release currently displayedFrame and reset lastDecodedFrame
			// 6 - compute numVideoFrames from GetNextFrame() and test if needs to stop decoding or not
			// 7 - DrawFrame()
		}
#endif
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
