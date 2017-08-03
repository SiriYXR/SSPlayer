#pragma once
#include "function.h"

struct Decoder
{
	Decoder();
	~Decoder()
	{
		swr_free(&au_convert_ctx);
		sws_freeContext(img_convert_ctx);

		av_free(out_buffer);
		av_frame_free(&pFrameYUV);
		av_frame_free(&pFrame);
		av_frame_free(&audioFrame);
		avcodec_close(pCodecCtx);//关闭解码器
		avcodec_close(aCodecCtx);
		avformat_close_input(&pFormatCtx);//关闭输入视频文件
	}
	void init_getFilePath(char path[], char name[]);
	bool decode();

	//ffmpeg变量------------------------------------------
	AVFormatContext *pFormatCtx;
	int i, videoindex, audioindex;
	AVCodecContext *pCodecCtx, *aCodecCtx;
	AVCodec *pCodec, *aCodec;
	AVFrame *pFrame, *pFrameYUV, *audioFrame;
	uint8_t *out_buffer;

	AVPacket *packet;
	int ret, got_picture;
	uint32_t len = 0;
	int64_t in_channel_layout;

	uint64_t out_channel_layout;
	int out_nb_samples;
	AVSampleFormat out_sample_fmt;
	int out_sample_rate;
	int out_channels;
	int out_buffer_size;

	//其他变量
	struct SwsContext *img_convert_ctx;
	struct SwrContext *au_convert_ctx;
	char filebuffer[255] = { 0 };
	char filepath[255] = { 0 };
	char filename[255] = { 0 };
};