#define __STDC_CONSTANT_MACROS

extern "C"
{

#pragma comment(lib,"avcodec.lib")
	//#pragma comment(lib,"avdevice.lib")
	//#pragma comment(lib,"avfilter.lib")
#pragma comment(lib,"avformat.lib")
#pragma comment(lib,"avutil.lib")
//#pragma comment(lib,"postproc.lib")
#pragma comment(lib,"swresample.lib")
#pragma comment(lib,"swscale.lib")

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libswresample/swresample.h>

#pragma comment(lib,"SDL2.lib")
#pragma comment(lib,"SDL2main.lib")

#include <SDL.h>

}

#include "acyclebuffer.h"

//Refresh event
#define SFM_REFRESH_EVENT (SDL_USEREVENT+1)
#define SFM_BREAK_EVENT (SDL_USEREVENT+2)

#define MAX_AUDIO_FRAME_SIZE 192000//1 second of 48khz 32bit audio 

//Buffer
static Uint8 *audio_chunk;
static Uint32 audio_len;
static Uint8 *audio_pos;
static Uint8 *copy_buf;
static ACycleBuffer out_buffer_audio(MAX_AUDIO_FRAME_SIZE * 10);

bool thread_exit = false;
bool thread_pause = false;

float fps = 25;

void getFilePath(char path[], char name[]);
int sfp_refresh_thread(void *opaque);
void fill_audio(void *udata, Uint8 *stream, int len);
void update_sdlRect(SDL_Rect &Rect, const AVCodecContext  *codecCtx, int sw, int sh);

int main(int argc, char **argv)
{
	//sdl变量----------------------------------------------
	int screen_w, screen_h;
	SDL_Window *screen;
	SDL_Renderer *sdlRenderer;
	SDL_Texture *sdlTexture;
	SDL_Rect sdlRect;
	SDL_Thread *video_tid;
	SDL_Event event;
	SDL_AudioSpec wanted_spec;

	//ffmpeg变量------------------------------------------
	AVFormatContext *pFormatCtx;
	int i, videoindex, audioindex;
	AVCodecContext *pCodecCtx, *aCodecCtx;
	AVCodec *pCodec, *aCodec;
	AVFrame *pFrame, *pFrameYUV, *audioFrame;
	uint8_t *out_buffer;
	//uint8_t *out_buffer_audio;
	
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

	getFilePath(filepath, filename);

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
		return -1;
	}
	//获取视频文件信息
	if (avformat_find_stream_info(pFormatCtx, nullptr) < 0) {
		printf("Couldn't find stream information.\n");
		return -1;
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
		return -1;
	}
	if (audioindex == -1) {
		printf("Didn't find a audio stream.\n");
		return -1;
	}
	//解码器信息
	pCodecCtx = pFormatCtx->streams[videoindex]->codec;
	aCodecCtx = pFormatCtx->streams[audioindex]->codec;
	//查找解码器
	pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
	aCodec = avcodec_find_decoder(aCodecCtx->codec_id);
	if (pCodec == nullptr) {
		printf("VideoCodec not found.\n");
		return -1;
	}
	if (aCodec == nullptr) {
		printf("AudioCodec not found.\n");
		return -1;
	}
	//打开解码器
	if (avcodec_open2(pCodecCtx, pCodec, nullptr) < 0) {
		printf("VideoCould not open codec.\n");
		return -1;
	}
	if (avcodec_open2(aCodecCtx, aCodec, nullptr) < 0) {
		printf("AudioCould not open codec.\n");
		return -1;
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
	//out_buffer_audio = (uint8_t *)av_malloc(MAX_AUDIO_FRAME_SIZE * 2);
	copy_buf = (uint8_t *)av_malloc(MAX_AUDIO_FRAME_SIZE * 2);
	if (copy_buf == 0) {
		return -1;
	}

	//裁剪无效数据
	img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, nullptr, nullptr, nullptr);

	//计算帧率
	fps = pFormatCtx->streams[videoindex]->r_frame_rate.num / 1000 - (double)pFormatCtx->streams[videoindex]->r_frame_rate.den / 10000;
	printf("%f", fps);

	
	//------------------------------------SDL初始化---------------------------------------------------
	//初始化视音频等模块
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
		printf("Could not initialize SDL - %s\n", SDL_GetError());
		return -1;
	}
	//初始化窗口大小
	screen_w = pCodecCtx->width;
	screen_h = pCodecCtx->height;
	//创建窗口
	sprintf(filebuffer, "%s - SSPlayer", filename);
	screen = SDL_CreateWindow(filebuffer, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, screen_w, screen_h, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	if (!screen) {
		printf("SDL:could not create window - exiting:%s\n", SDL_GetError());
		return -1;
	}
	SDL_SetWindowMinimumSize(screen, 300, 200);
	//创建渲染器
	sdlRenderer = SDL_CreateRenderer(screen, -1, 0);
	//创建纹理
	sdlTexture = SDL_CreateTexture(sdlRenderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, pCodecCtx->width, pCodecCtx->height);
	//初始化矩形
	sdlRect.x = 0;
	sdlRect.y = 0;
	sdlRect.w = screen_w;
	sdlRect.h = screen_h;

	//建立音频信息
	wanted_spec.freq = out_sample_rate;
	wanted_spec.format = AUDIO_S16SYS;
	wanted_spec.channels = out_channels;
	wanted_spec.silence = 0;
	wanted_spec.samples = out_nb_samples;
	wanted_spec.callback = fill_audio;
	wanted_spec.userdata = aCodecCtx;

	if (SDL_OpenAudio(&wanted_spec, nullptr) < 0) {
		printf("SDL_OpenAudio:%s\n", SDL_GetError());
		return -1;
	}

	//FIX:Some Codec's Context Information is missing  
	in_channel_layout = av_get_default_channel_layout(aCodecCtx->channels);

	//Swr
	au_convert_ctx = swr_alloc();
	au_convert_ctx = swr_alloc_set_opts(au_convert_ctx, out_channel_layout, out_sample_fmt, out_sample_rate, in_channel_layout, aCodecCtx->sample_fmt, aCodecCtx->sample_rate, 0, nullptr);
	swr_init(au_convert_ctx);

	//创建多线程
	video_tid = SDL_CreateThread(sfp_refresh_thread, nullptr, nullptr);

	//输出视频信息
	printf("File Information------------------\n");
	av_dump_format(pFormatCtx, 0, filepath, false);
	printf("------------------------------------\n");

	//Play
	SDL_PauseAudio(0);

	while (true) {
		//Wait
		SDL_WaitEvent(&event);
		if (event.type == SFM_REFRESH_EVENT) {
			while (true) {
				//从输入文件读取一帧压缩数据
				if (av_read_frame(pFormatCtx, packet) < 0) {
					thread_exit = 1;
				}

				if (packet->stream_index == videoindex) {

					//解码一帧压缩数据
					ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
					if (ret < 0) {
						printf("Decode Error.\n");
						return -1;
					}
					if (got_picture) {
						sws_scale(img_convert_ctx, (const unsigned char* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height, pFrameYUV->data, pFrameYUV->linesize);

						//同步窗口尺寸
						update_sdlRect(sdlRect, pCodecCtx, screen_w, screen_h);

						SDL_UpdateTexture(sdlTexture, nullptr, pFrameYUV->data[0], pFrameYUV->linesize[0]);
						SDL_RenderClear(sdlRenderer);
						SDL_RenderCopy(sdlRenderer, sdlTexture, nullptr, &sdlRect);
						SDL_RenderPresent(sdlRenderer);
					}

					break;
				}
				else if (packet->stream_index = audioindex) {

					//解码一帧压缩数据
					ret = avcodec_decode_audio4(aCodecCtx, audioFrame, &got_picture, packet);
					if (ret < 0) {
						printf("Error in decoding audio frame.\n");
						return -1;
					}
					//if (got_picture > 0) {
					//	swr_convert(au_convert_ctx, &out_buffer_audio, MAX_AUDIO_FRAME_SIZE, (const uint8_t **)audioFrame->data, audioFrame->nb_samples);
					//}

					//while (audio_len > 0)//Wait until finish
					//	SDL_Delay(1);

					////Set audio buffer (PCM data)
					//audio_chunk = (Uint8 *)out_buffer_audio;
					////Audio buffer length
					//audio_len = out_buffer_size;
					//audio_pos = audio_chunk;

					if (got_picture > 0) {
						int rr= swr_convert(au_convert_ctx, &out_buffer, MAX_AUDIO_FRAME_SIZE, (const uint8_t **)audioFrame->data, audioFrame->nb_samples);
						while (true) {
							if (rr * 4 > out_buffer_audio.restSpace()) {
								SDL_Delay(1);
							}
							else {
								out_buffer_audio.write((char*)out_buffer, rr * 4);
								break;
							}
						}
					}

				}
			}

			av_free_packet(packet);

		}
		else if (event.type == SDL_MOUSEBUTTONDOWN) {
			if (event.button.button == SDL_BUTTON_LEFT) {
				thread_pause = !thread_pause;
			}
		}
		else if (event.type == SDL_KEYDOWN) {
			if (event.key.keysym.sym == SDLK_SPACE) {
				thread_pause = !thread_pause;
			}
			else if (event.key.keysym.sym == SDLK_ESCAPE) {
				thread_exit = true;
			}
			else if (event.key.keysym.sym == SDLK_RIGHT) {

			}
			else if (event.key.keysym.sym == SDLK_LEFT) {

			}
			else if (event.key.keysym.sym == SDLK_r) {
				SDL_SetWindowSize(screen, pCodecCtx->width, pCodecCtx->height);
			}
		}
		else if (event.type == SDL_WINDOWEVENT) {
			//If Resize
			SDL_GetWindowSize(screen, &screen_w, &screen_h);
		}
		else if (event.type == SDL_QUIT) {
			thread_exit = true;
		}
		else if (event.type == SFM_BREAK_EVENT) {
			break;
		}
	}


	SDL_CloseAudio();
	SDL_Quit();

	swr_free(&au_convert_ctx);
	sws_freeContext(img_convert_ctx);

	av_free(out_buffer);
	//av_free(out_buffer_audio);
	av_frame_free(&pFrameYUV);
	av_frame_free(&pFrame);
	av_frame_free(&audioFrame);
	avcodec_close(pCodecCtx);//关闭解码器
	avcodec_close(aCodecCtx);
	avformat_close_input(&pFormatCtx);//关闭输入视频文件

	return 0;
}

void getFilePath(char path[], char name[])
{
	FILE *fp;

	fp = fopen("data/path.txt", "rb+");
	fscanf(fp, "%s", name);

	sprintf(path, "data/resources/%s", name);

	fclose(fp);
}

int sfp_refresh_thread(void *opaque)
{
	thread_exit = false;
	thread_pause = false;

	while (!thread_exit) {
		if (!thread_pause) {
			SDL_Event event;
			event.type = SFM_REFRESH_EVENT;
			SDL_PushEvent(&event);
		}
		SDL_Delay(1000 / fps);
	}
	thread_exit = false;
	thread_pause = false;
	//Break
	SDL_Event event;
	event.type = SFM_BREAK_EVENT;
	SDL_PushEvent(&event);

	return 0;
}

void fill_audio(void * udata, Uint8 * stream, int len)
{
	SDL_memset(stream, 0, len);
	//if (audio_len == 0)
	//	return;

	//len = (len > audio_len ? audio_len : len);/*  Mix  as  much  data  as  possible  */

	//SDL_MixAudio(stream, audio_pos, len, SDL_MIX_MAXVOLUME);
	//audio_pos += len;
	//audio_len -= len;

	int n = out_buffer_audio.read((char*)copy_buf, len);
	SDL_MixAudio(stream, copy_buf, n, SDL_MIX_MAXVOLUME);
}

void update_sdlRect(SDL_Rect & Rect, const AVCodecContext   *codecCtx, int sw, int sh)
{
	if ((double)sw / (double)sh < (double)codecCtx->width / (double)codecCtx->height) {
		Rect.w = sw;
		Rect.h = codecCtx->height *((double)sw / (double)codecCtx->width);
		Rect.x = 0;
		Rect.y = (sh - Rect.h) / 2;
	}
	else if ((double)sw / (double)sh > (double)codecCtx->width / (double)codecCtx->height) {
		Rect.w = codecCtx->width*((double)sh / (double)codecCtx->height);
		Rect.h = sh;
		Rect.x = (sw - Rect.w) / 2;
		Rect.y = 0;
	}
	else {
		Rect.w = sw;
		Rect.h = sh;
		Rect.x = 0;
		Rect.y = 0;
	}

}