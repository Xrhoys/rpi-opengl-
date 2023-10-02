#include "video_decode.h"

// NOTE(Xrhoys): return type is temporary
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

	// Find hardware pixel format
	for (i32 i = 0;; i++) {
        const AVCodecHWConfig *config = avcodec_get_hw_config(decoder->codec, i);
        if (!config) {
            fprintf(stderr, "Decoder %s does not support device type %s.\n",
                    decoder->codec->name, av_hwdevice_get_type_name(DECODE_HW_DEVICE_TYPE));
            return;
        }
        if (config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX &&
            config->device_type == DECODE_HW_DEVICE_TYPE) {
            hw_pix_fmt = config->pix_fmt;
            break;
        }
    }

	decoder->codecContext->get_format = get_hw_format;

	// HW Decoder init
	if(hw_decoder_init(decoder, DECODE_HW_DEVICE_TYPE) < 0)
        return;
	
	if(avcodec_open2(decoder->codecContext, decoder->codec, NULL) < 0)
	{
		// Could not open codec
		return;
	}

#if 0
	decoder->pFrame    = av_frame_alloc();
	decoder->packet    = av_packet_alloc();
#else
	// NOTE(Ecy): pre-allocate the packets in work queue
	for(u32 index = 0;
		index < DECODE_QUEUE_SIZE;
		++index)
	{
		decoder->framePool[index].packet = av_packet_alloc();
		decoder->framePool[index].frameId = -1;
		decoder->framePool[index].decoder = decoder;
	}
#endif

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
#if RPI
			AVHWFramesContext *hwContext = (AVHWFramesContext*)decoder->pFrame->hw_frames_ctx->data;
			AVDRMFrameDescriptor *drmDesc = (AVDRMFrameDescriptor*)decoder->pFrame->data[0];
			AVDRMObjectDescriptor *obj = &drmDesc->objects[0];
			auto *layer = &drmDesc->layers[0];

			EGLint atts[] = {
				// W, H used in TexImage2D above!
				EGL_WIDTH, decoder->pFrame->width,
				EGL_HEIGHT, decoder->pFrame->height,
				EGL_LINUX_DRM_FOURCC_EXT, (EGLint)drmDesc->layers[0].format,
				EGL_DMA_BUF_PLANE0_FD_EXT, drmDesc->objects[layer->planes[0].object_index].fd,
				EGL_DMA_BUF_PLANE0_OFFSET_EXT, (EGLint)layer->planes[0].offset,
				EGL_DMA_BUF_PLANE0_PITCH_EXT, (EGLint)layer->planes[0].pitch,
				EGL_DMA_BUF_PLANE0_MODIFIER_LO_EXT, (EGLint)(drmDesc->objects[layer->planes[0].object_index].format_modifier & 0xFFFFFFFF),
  				EGL_DMA_BUF_PLANE0_MODIFIER_HI_EXT, (EGLint)(drmDesc->objects[layer->planes[0].object_index].format_modifier >> 32),
				EGL_DMA_BUF_PLANE1_FD_EXT, drmDesc->objects[layer->planes[1].object_index].fd,
				EGL_DMA_BUF_PLANE1_OFFSET_EXT, (EGLint)layer->planes[1].offset,
				EGL_DMA_BUF_PLANE1_PITCH_EXT, (EGLint)layer->planes[1].pitch,
				EGL_DMA_BUF_PLANE1_MODIFIER_LO_EXT, (EGLint)(drmDesc->objects[layer->planes[1].object_index].format_modifier & 0xFFFFFFFF),
  				EGL_DMA_BUF_PLANE1_MODIFIER_HI_EXT, (EGLint)(drmDesc->objects[layer->planes[1].object_index].format_modifier >> 32),
				// EGL_YUV_COLOR_SPACE_HINT_EXT, EGL_ITU_REC709_EXT,
				// EGL_SAMPLE_RANGE_HINT_EXT, EGL_YUV_FULL_RANGE_EXT,
				EGL_NONE,
			};
			EGLImageKHR imgB = eglCreateImageKHR(eglDisplay, EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT, (EGLClientBuffer)(uint64_t)0, atts);
			Assert(imgB != EGL_NO_IMAGE);

			glEnable(GL_TEXTURE_EXTERNAL_OES);
			glBindTexture(GL_TEXTURE_EXTERNAL_OES, g_bgTexture);
			glEGLImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, imgB);
			glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
           	glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			eglDestroyImageKHR(eglDisplay, imgB);
#else
			// AVHWFramesContext *hwContext = (AVHWFramesContext*)decoder->pFrame->hw_frames_ctx->data;
			VASurfaceID vaSurface = (uintptr_t)decoder->pFrame->data[3];

			VADRMPRIMESurfaceDescriptor prime;
			int res = vaExportSurfaceHandle(decoder->vaDisplay, vaSurface, 
									VA_SURFACE_ATTRIB_MEM_TYPE_DRM_PRIME_2, 
									VA_EXPORT_SURFACE_READ_ONLY | VA_EXPORT_SURFACE_COMPOSED_LAYERS, 
									&prime);
			if(res != VA_STATUS_SUCCESS)
			{
				fprintf(stderr, "failed %d\n", res);
				// TODO(Xrhoys): log failure
				return;
			}

			vaSyncSurface(decoder->vaDisplay, vaSurface);

			// fprintf(stdout, "Fourcc: %c%c%c%c\n", prime.fourcc << 24, prime.fourcc << 16, prime.fourcc << 8, prime.fourcc);
			// for(u32 index = 0; index < prime.num_layers; ++index)
			// {
			// 	fprintf(stdout, "Layer info: %d, %d\n", prime.layers[index].drm_format);
			// 	for(u32 planeIndex = 0; planeIndex < prime.layers[index].num_planes; ++planeIndex)
			// 	{
			// 		fprintf(stdout, "plane info: %d, %d, %d\n", prime.layers[index].object_index[planeIndex], prime.layers[index].offset[planeIndex], prime.layers[index].pitch[planeIndex]);
			// 	}
			// }
			
			EGLint format = prime.layers[0].drm_format;
			auto object1 = &prime.objects[0];
			auto object2 = &prime.objects[1];
			auto layer = &prime.layers[0];

			EGLint atts[] = {
				// W, H used in TexImage2D above!
				EGL_WIDTH, decoder->pFrame->width,
				EGL_HEIGHT, decoder->pFrame->height,
				EGL_LINUX_DRM_FOURCC_EXT, format,
				EGL_DMA_BUF_PLANE0_FD_EXT, object1->fd,
				EGL_DMA_BUF_PLANE0_OFFSET_EXT, (EGLint)layer->offset[0],
				EGL_DMA_BUF_PLANE0_PITCH_EXT, (EGLint)layer->pitch[0],
				EGL_DMA_BUF_PLANE0_MODIFIER_LO_EXT, (EGLint)(object1->drm_format_modifier & 0xFFFFFFFF),
				EGL_DMA_BUF_PLANE0_MODIFIER_HI_EXT, (EGLint)(object1->drm_format_modifier >> 32),
				EGL_DMA_BUF_PLANE1_FD_EXT, object2->fd,
				EGL_DMA_BUF_PLANE1_OFFSET_EXT, (EGLint)layer->offset[1],
				EGL_DMA_BUF_PLANE1_PITCH_EXT, (EGLint)layer->pitch[1],
				EGL_DMA_BUF_PLANE1_MODIFIER_LO_EXT, (EGLint)(object2->drm_format_modifier & 0xFFFFFFFF),
				EGL_DMA_BUF_PLANE1_MODIFIER_HI_EXT, (EGLint)(object2->drm_format_modifier >> 32),
				// EGL_YUV_COLOR_SPACE_HINT_EXT, EGL_ITU_REC709_EXT,
				// EGL_SAMPLE_RANGE_HINT_EXT, EGL_YUV_FULL_RANGE_EXT,
				EGL_NONE,
			};
			EGLImageKHR imgB = eglCreateImageKHR(eglDisplay, EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT, (EGLClientBuffer)(uint64_t)0, atts);
			Assert(imgB != EGL_NO_IMAGE);

			glEnable(GL_TEXTURE_EXTERNAL_OES);
			glBindTexture(GL_TEXTURE_EXTERNAL_OES, g_bgTexture);
			glEGLImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, imgB);
			glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
           	glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			eglDestroyImageKHR(eglDisplay, imgB);
#endif
		}

    }
}

internal void
DecodeThreaded(video_decode_unit *unit)
{
	video_decode *decoder = unit->decoder;
	AVFrame *frame = unit->frame;
	AVPacket *packet = unit->packet;
	fprintf(stderr, "debug4: \n");
	
    int ret;
	
	char buffer[1024];
    ret = avcodec_send_packet(decoder->codecContext, packet);
	
	fprintf(stderr, "debug4.5\n");
	
	av_strerror(ret, buffer, 1024);
	
    if (ret < 0) {
        fprintf(stderr, "Error sending a packet for decoding\n");
        return;
    }
	
    while (ret >= 0) {
        ret = avcodec_receive_frame(decoder->codecContext, frame);
		
		av_strerror(ret, buffer, 1024);
		
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            return;
        else if (ret < 0) {
            fprintf(stderr, "Error during decoding\n");
            return;
        }
		
		if(ret >= 0)
		{
			fprintf(stderr, "debug5");
			
#if RPI
			AVHWFramesContext *hwContext = (AVHWFramesContext*)frame->hw_frames_ctx->data;
			AVDRMFrameDescriptor *drmDesc = (AVDRMFrameDescriptor*)frame->data[0];
			AVDRMObjectDescriptor *obj = &drmDesc->objects[0];
			auto *layer = &drmDesc->layers[0];
			
			EGLint atts[] = {
				// W, H used in TexImage2D above!
				EGL_WIDTH, frame->width,
				EGL_HEIGHT, frame->height,
				EGL_LINUX_DRM_FOURCC_EXT, (EGLint)drmDesc->layers[0].format,
				EGL_DMA_BUF_PLANE0_FD_EXT, drmDesc->objects[layer->planes[0].object_index].fd,
				EGL_DMA_BUF_PLANE0_OFFSET_EXT, (EGLint)layer->planes[0].offset,
				EGL_DMA_BUF_PLANE0_PITCH_EXT, (EGLint)layer->planes[0].pitch,
				EGL_DMA_BUF_PLANE0_MODIFIER_LO_EXT, (EGLint)(drmDesc->objects[layer->planes[0].object_index].format_modifier & 0xFFFFFFFF),
				EGL_DMA_BUF_PLANE0_MODIFIER_HI_EXT, (EGLint)(drmDesc->objects[layer->planes[0].object_index].format_modifier >> 32),
				EGL_DMA_BUF_PLANE1_FD_EXT, drmDesc->objects[layer->planes[1].object_index].fd,
				EGL_DMA_BUF_PLANE1_OFFSET_EXT, (EGLint)layer->planes[1].offset,
				EGL_DMA_BUF_PLANE1_PITCH_EXT, (EGLint)layer->planes[1].pitch,
				EGL_DMA_BUF_PLANE1_MODIFIER_LO_EXT, (EGLint)(drmDesc->objects[layer->planes[1].object_index].format_modifier & 0xFFFFFFFF),
				EGL_DMA_BUF_PLANE1_MODIFIER_HI_EXT, (EGLint)(drmDesc->objects[layer->planes[1].object_index].format_modifier >> 32),
				// EGL_YUV_COLOR_SPACE_HINT_EXT, EGL_ITU_REC709_EXT,
				// EGL_SAMPLE_RANGE_HINT_EXT, EGL_YUV_FULL_RANGE_EXT,
				EGL_NONE,
			};
			EGLImageKHR imgB = eglCreateImageKHR(eglDisplay, EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT, (EGLClientBuffer)(uint64_t)0, atts);
			Assert(imgB != EGL_NO_IMAGE);
			
			glEnable(GL_TEXTURE_EXTERNAL_OES);
			glBindTexture(GL_TEXTURE_EXTERNAL_OES, g_bgTexture);
			glEGLImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, imgB);
			glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			
			eglDestroyImageKHR(eglDisplay, imgB);
#else
			// AVHWFramesContext *hwContext = (AVHWFramesContext*)decoder->pFrame->hw_frames_ctx->data;
			VASurfaceID vaSurface = (uintptr_t)frame->data[3];
			
			VADRMPRIMESurfaceDescriptor prime;
			int res = vaExportSurfaceHandle(decoder->vaDisplay, vaSurface, 
											VA_SURFACE_ATTRIB_MEM_TYPE_DRM_PRIME_2, 
											VA_EXPORT_SURFACE_READ_ONLY | VA_EXPORT_SURFACE_COMPOSED_LAYERS, 
											&prime);
			if(res != VA_STATUS_SUCCESS)
			{
				fprintf(stderr, "failed %d\n", res);
				// TODO(Xrhoys): log failure
				return;
			}
			
			vaSyncSurface(decoder->vaDisplay, vaSurface);
			
			// fprintf(stdout, "Fourcc: %c%c%c%c\n", prime.fourcc << 24, prime.fourcc << 16, prime.fourcc << 8, prime.fourcc);
			// for(u32 index = 0; index < prime.num_layers; ++index)
			// {
			// 	fprintf(stdout, "Layer info: %d, %d\n", prime.layers[index].drm_format);
			// 	for(u32 planeIndex = 0; planeIndex < prime.layers[index].num_planes; ++planeIndex)
			// 	{
			// 		fprintf(stdout, "plane info: %d, %d, %d\n", prime.layers[index].object_index[planeIndex], prime.layers[index].offset[planeIndex], prime.layers[index].pitch[planeIndex]);
			// 	}
			// }
			
			EGLint format = prime.layers[0].drm_format;
			auto object1 = &prime.objects[0];
			auto object2 = &prime.objects[1];
			auto layer = &prime.layers[0];
			
			EGLint atts[] = {
				// W, H used in TexImage2D above!
				EGL_WIDTH, frame->width,
				EGL_HEIGHT, frame->height,
				EGL_LINUX_DRM_FOURCC_EXT, format,
				EGL_DMA_BUF_PLANE0_FD_EXT, object1->fd,
				EGL_DMA_BUF_PLANE0_OFFSET_EXT, (EGLint)layer->offset[0],
				EGL_DMA_BUF_PLANE0_PITCH_EXT, (EGLint)layer->pitch[0],
				EGL_DMA_BUF_PLANE0_MODIFIER_LO_EXT, (EGLint)(object1->drm_format_modifier & 0xFFFFFFFF),
				EGL_DMA_BUF_PLANE0_MODIFIER_HI_EXT, (EGLint)(object1->drm_format_modifier >> 32),
				EGL_DMA_BUF_PLANE1_FD_EXT, object2->fd,
				EGL_DMA_BUF_PLANE1_OFFSET_EXT, (EGLint)layer->offset[1],
				EGL_DMA_BUF_PLANE1_PITCH_EXT, (EGLint)layer->pitch[1],
				EGL_DMA_BUF_PLANE1_MODIFIER_LO_EXT, (EGLint)(object2->drm_format_modifier & 0xFFFFFFFF),
				EGL_DMA_BUF_PLANE1_MODIFIER_HI_EXT, (EGLint)(object2->drm_format_modifier >> 32),
				// EGL_YUV_COLOR_SPACE_HINT_EXT, EGL_ITU_REC709_EXT,
				// EGL_SAMPLE_RANGE_HINT_EXT, EGL_YUV_FULL_RANGE_EXT,
				EGL_NONE,
			};
			EGLImageKHR imgB = eglCreateImageKHR(eglDisplay, EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT, (EGLClientBuffer)(uint64_t)0, atts);
			Assert(imgB != EGL_NO_IMAGE);

#if 0			
			glEnable(GL_TEXTURE_EXTERNAL_OES);
			glBindTexture(GL_TEXTURE_EXTERNAL_OES, g_bgTexture);
			glEGLImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, imgB);
			glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			
			eglDestroyImageKHR(eglDisplay, imgB);
#endif
#endif
			unit->isReady = true;
			unit->eglImage = imgB;
			unit->frameId = decoder->frameIdCounter_++;
			
		}
		
    }
}

internal void
UpdateDecode(video_decode *decoder)
{
	// NOTE(Xrhoys): select available decoding frame from pool
	for(u32 index = 0;
		index < DECODE_QUEUE_SIZE;
		++index)
	{
		video_decode_unit *unit = &decoder->framePool[index];
		if(unit->frameId == -1)
		{
			if(av_read_frame(decoder->formatContext, unit->packet) >= 0)
			{
				if(unit->packet->stream_index == decoder->streamIndex)
				{
					Decode(decoder);
				}
				
				av_packet_unref(unit->packet);
				av_frame_unref(unit->frame);
			}
			break;
		}
		
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
	av_packet_free(&decode->packet);
}