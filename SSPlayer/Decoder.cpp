#include "Decoder.h"
#include "Thread.h"
#include "acyclebuffer.h"

Decoder::Decoder()
{
	init_getFilePath(filepath, filename);

	//---------------------------------------FFmpeg初始化----------------------------------------------------
	//注册所有组件
	av_register_all();
	avformat_network_init();

	pFormatCtx = avformat_alloc_context();
	pFrame = av_frame_alloc();
	pFrameYUV = av_frame_alloc();
	audioFrame = av_frame_alloc();
	packet = (AVPacket *)av_malloc(sizeof(AVPacket));
	av_init_packet(packet);

	//打开输入视频文件
	if (avformat_open_input(&pFormatCtx, filepath, nullptr, nullptr) != 0) {
		printf("Couldn't open input stream.\n");
		exit(-1);
	}
	//获取视频文件信息
	if (avformat_find_stream_info(pFormatCtx, nullptr) < 0) {
		printf("Couldn't find stream information.\n");
		exit(-1);
	}
	//查找视音频流
	videoindex = -1;
	audioindex = -1;
	for (i = 0; i < pFormatCtx->nb_streams; i++) {
		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO&&videoindex < 0) {
			videoindex = i;
		}
		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO&&audioindex < 0) {
			audioindex = i;
		}
	}
	if (videoindex == -1) {
		printf("Didn't find a video stream.\n");
		exit(-1);
	}
	if (audioindex == -1) {
		printf("Didn't find a audio stream.\n");
		exit(-1);
	}
	//解码器信息
	pCodecCtx = pFormatCtx->streams[videoindex]->codec;
	aCodecCtx = pFormatCtx->streams[audioindex]->codec;
	//查找解码器
	pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
	aCodec = avcodec_find_decoder(aCodecCtx->codec_id);
	if (pCodec == nullptr) {
		printf("VideoCodec not found.\n");
		exit(-1);
	}
	if (aCodec == nullptr) {
		printf("AudioCodec not found.\n");
		exit(-1);
	}
	//打开解码器
	if (avcodec_open2(pCodecCtx, pCodec, nullptr) < 0) {
		printf("VideoCould not open codec.\n");
		exit(-1);
	}
	if (avcodec_open2(aCodecCtx, aCodec, nullptr) < 0) {
		printf("AudioCould not open codec.\n");
		exit(-1);
	}

	out_buffer = (unsigned char *)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height, 1));
	av_image_fill_arrays(pFrameYUV->data, pFrameYUV->linesize, out_buffer, AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height, 1);

	//初始化音频信息
	out_channel_layout = AV_CH_LAYOUT_STEREO;
	out_nb_samples = aCodecCtx->frame_size;
	out_sample_fmt = AV_SAMPLE_FMT_S16;
	out_sample_rate = 44100;
	out_channels = av_get_channel_layout_nb_channels(out_channel_layout);
	out_buffer_size = av_samples_get_buffer_size(nullptr, out_channels, out_nb_samples, out_sample_fmt, 1);
	copy_buf = (uint8_t *)av_malloc(MAX_AUDIO_FRAME_SIZE * 2);
	if (copy_buf == 0) {
		exit(-1);
	}

	//裁剪无效数据
	img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, nullptr, nullptr, nullptr);

	//计算帧率
	fps = (double)pFormatCtx->streams[videoindex]->r_frame_rate.num / (double)pFormatCtx->streams[videoindex]->r_frame_rate.den;

}

bool Decoder::decode()
{

	while (true) {
		//从输入文件读取一帧压缩数据
		if (av_read_frame(pFormatCtx, packet) < 0) {
			thread_exit = true;
		}

		//Play
		SDL_PauseAudio(0);

		if (packet->stream_index == videoindex) {

			//解码一帧压缩数据
			ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
			if (ret < 0) {
				printf("Decode Error.\n");
				return -1;
			}
			if (got_picture) {
				sws_scale(img_convert_ctx, (const unsigned char* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height, pFrameYUV->data, pFrameYUV->linesize);
				return true;
			}
		}
		else if (packet->stream_index = audioindex) {

			//解码一帧压缩数据
			ret = avcodec_decode_audio4(aCodecCtx, audioFrame, &got_picture, packet);
			if (ret < 0) {
				printf("Error in decoding audio frame.\n");
				return -1;
			}

			if (got_picture > 0) {
				int rr = swr_convert(au_convert_ctx, &out_buffer, MAX_AUDIO_FRAME_SIZE, (const uint8_t **)audioFrame->data, audioFrame->nb_samples);
				while (true) {
					if (rr * 4 > Thread::out_buffer_audio.restSpace()) {
						SDL_Delay(1);
					}
					else {
						Thread::out_buffer_audio.write((char*)out_buffer, rr * 4);
						break;
					}
				}
			}
		}
		av_free_packet(packet);
	}
	return false;
}

void Decoder::init_getFilePath(char path[], char name[])
{
	FILE *fp;

	fp = fopen("data/path.txt", "rb+");
	fscanf(fp, "%s", name);

	sprintf(path, "data/resources/%s", name);

	fclose(fp);
}