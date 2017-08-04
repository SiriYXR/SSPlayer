#define __STDC_CONSTANT_MACROS

extern "C"
{

#pragma comment(lib,"avcodec.lib")
#pragma comment(lib,"avdevice.lib")
//#pragma comment(lib,"avfilter.lib")
#pragma comment(lib,"avformat.lib")
#pragma comment(lib,"avutil.lib")
#pragma comment(lib,"postproc.lib")
#pragma comment(lib,"swresample.lib")
#pragma comment(lib,"swscale.lib")

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libswresample/swresample.h>
#include <libavdevice/avdevice.h>
#include <libpostproc/postprocess.h>

#pragma comment(lib,"SDL2.lib")
#pragma comment(lib,"SDL2main.lib")

#include <SDL.h>

}

//Refresh event
#define SFM_REFRESH_EVENT (SDL_USEREVENT+1)
#define SFM_BREAK_EVENT (SDL_USEREVENT+2)

bool thread_exit = false;
bool thread_pause = false;

float fps = 25;

typedef struct PacketQueue
{
	AVPacketList *first_pkt, *last_pkt;
	int nb_packets;
	int size;
	SDL_mutex *mutex;
	SDL_cond *cond;
}PacketQueue;

void packet_queue_init(PacketQueue *q);
int packet_queue_put(PacketQueue *q, AVPacket *pkt);
static int packet_queue_get(PacketQueue *q,AVPacket *pkt,int block);
int decode_interrupt_cb(void);
void getFilePath(char path[]);
int sfp_refresh_thread(void *opaque);
void fill_audio(void *udata, Uint8 *stream, int len);
void update_sdlRect(SDL_Rect &Rect, const AVCodecContext  *codecCtx, int sw, int sh);

int main(int argc, char **argv)
{
	//sdl����----------------------------------------------
	int screen_w, screen_h;
	SDL_Window *screen;
	SDL_Renderer *sdlRenderer;
	SDL_Texture *sdlTexture;
	SDL_Rect sdlRect;
	SDL_Thread *video_tid;
	SDL_Event event;
	SDL_AudioSpec wanted_spec;

	//ffmpeg����------------------------------------------
	AVFormatContext *pFormatCtx;
	int i, videoindex,audioindex;
	AVCodecContext *pCodecCtx,*aCodecCtx;
	AVCodec *pCodec,*aCodec;
	AVFrame *pFrame, *pFrameYUV,*audioFrame;
	unsigned char *out_buffer;
	AVPacket *packet;
	int ret, got_picture;

	//��������
	struct SwsContext *img_convert_ctx;
	SwrContext *au_convert_ctx;
	char filepath[255] = {0};

	getFilePath(filepath);

	//---------------------------------------FFmpeg��ʼ��----------------------------------------------------
	//ע���������
	av_register_all();

	pFormatCtx = avformat_alloc_context();
	pFrame = av_frame_alloc();
	pFrameYUV = av_frame_alloc();
	packet = (AVPacket *)av_malloc(sizeof(AVPacket));

	//��������Ƶ�ļ�
	if (avformat_open_input(&pFormatCtx, filepath, nullptr, nullptr) != 0) {
		printf("Couldn't open input stream.\n");
		return -1;
	}
	//��ȡ��Ƶ�ļ���Ϣ
	if (avformat_find_stream_info(pFormatCtx, nullptr) < 0) {
		printf("Couldn't find stream information.\n");
		return -1;
	}
	//��������Ƶ��
	videoindex = -1;
	audioindex = -1;
	for (i = 0; i < pFormatCtx->nb_streams; i++) {
		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO&&videoindex<0) {
			videoindex = i;
		}
		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO&&audioindex<0) {
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
	//��������Ϣ
	pCodecCtx = pFormatCtx->streams[videoindex]->codec;
	aCodecCtx = pFormatCtx->streams[audioindex]->codec;
	//���ҽ�����
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
	//�򿪽�����
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

	//�ü���Ч����
	img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, nullptr, nullptr, nullptr);

	//����֡��
	fps = pFormatCtx->streams[videoindex]->r_frame_rate.num / 1000 - (double)pFormatCtx->streams[videoindex]->r_frame_rate.den / 10000;
	printf("%f", fps);

	

	//------------------------------------SDL��ʼ��---------------------------------------------------
	//��ʼ������Ƶ��ģ��
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
		printf("Could not initialize SDL - %s\n", SDL_GetError());
		return -1;
	}
	//��ʼ�����ڴ�С
	screen_w = pCodecCtx->width;
	screen_h = pCodecCtx->height;
	//��������
	screen = SDL_CreateWindow("SSPlayer", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, screen_w, screen_h, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	if (!screen) {
		printf("SDL:could not create window - exiting:%s\n", SDL_GetError());
		return -1;
	}
	SDL_SetWindowMinimumSize(screen, 300, 200);
	//������Ⱦ��
	sdlRenderer = SDL_CreateRenderer(screen, -1, 0);
	//��������
	sdlTexture = SDL_CreateTexture(sdlRenderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, pCodecCtx->width, pCodecCtx->height);
	//��ʼ������
	sdlRect.x = 0;
	sdlRect.y = 0;
	sdlRect.w = screen_w;
	sdlRect.h = screen_h;

	//������Ƶ��Ϣ
	wanted_spec.freq = aCodecCtx->sample_rate;
	wanted_spec.format = AUDIO_S16SYS;
	wanted_spec.channels = aCodecCtx->channels;
	wanted_spec.silence = 0;
	wanted_spec.samples = aCodecCtx->frame_size;
	wanted_spec.callback = fill_audio;
	wanted_spec.userdata = aCodecCtx;

	if (SDL_OpenAudio(&wanted_spec, nullptr) < 0) {
		printf("SDL_OpenAudio:%s\n",SDL_GetError());
		return -1;
	}

	//�������߳�
	video_tid = SDL_CreateThread(sfp_refresh_thread, nullptr, nullptr);



	//�����Ƶ��Ϣ
	printf("File Information------------------\n");
	av_dump_format(pFormatCtx, 0, filepath, 0);
	printf("------------------------------------\n");

	while (true) {
		//Wait
		SDL_WaitEvent(&event);
		if (event.type == SFM_REFRESH_EVENT) {
			while (true) {
				//�������ļ���ȡһ֡ѹ������
				if (av_read_frame(pFormatCtx, packet) < 0) {
					thread_exit = 1;
				}

				if (packet->stream_index == videoindex) {
					break;
				}
			}

			//����һ֡ѹ������
			ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
			if (ret < 0) {
				printf("Decode Error.\n");
				return -1;
			}
			if (got_picture) {
				sws_scale(img_convert_ctx, (const unsigned char* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height, pFrameYUV->data, pFrameYUV->linesize);

				//ͬ�����ڳߴ�
				update_sdlRect(sdlRect, pCodecCtx, screen_w, screen_h);

				SDL_UpdateTexture(sdlTexture, nullptr, pFrameYUV->data[0], pFrameYUV->linesize[0]);
				SDL_RenderClear(sdlRenderer);
				SDL_RenderCopy(sdlRenderer, sdlTexture, nullptr, &sdlRect);
				SDL_RenderPresent(sdlRenderer);

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
				SDL_SetWindowSize(screen,pCodecCtx->width,pCodecCtx->height);
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

	SDL_Quit();

	sws_freeContext(img_convert_ctx);
	av_frame_free(&pFrameYUV);
	av_frame_free(&pFrame);
	avcodec_close(pCodecCtx);//�رս�����
	avformat_close_input(&pFormatCtx);//�ر�������Ƶ�ļ�

	return 0;
}

void packet_queue_init(PacketQueue * q)
{
	memset(q, 0, sizeof(PacketQueue));
	q->mutex = SDL_CreateMutex();
	q->cond = SDL_CreateCond();
}

int packet_queue_put(PacketQueue * q, AVPacket * pkt)
{
	AVPacketList *pkt1;
	if (av_dup_packet(pkt) < 0) {
		return -1;
	}
	pkt1 = (AVPacketList*)av_malloc(sizeof(AVPacketList));
	if (!pkt1) {
		return -1;
	}
	pkt1->pkt = *pkt;
	pkt1->next = NULL;

	SDL_LockMutex(q->mutex);

	if (!q->last_pkt) {
		q->first_pkt = pkt1;
	}
	else {
		q->last_pkt = pkt1;
	}
	q->last_pkt = pkt1;
	q->nb_packets++;
	q->size += pkt1->pkt.size;
	SDL_CondSignal(q->cond);

	SDL_UnlockMutex(q->mutex);
	return 0;
}

int packet_queue_get(PacketQueue * q, AVPacket * pkt, int block)
{
	AVPacketList *pkt1;
	int ret;

	SDL_LockMutex(q->mutex);

	while (true) {
		if (thread_exit) {
			ret = -1;
			break;
		}

		pkt1 = q->first_pkt;
		if (pkt1) {
			q->first_pkt = pkt1->next;
			if (!q->first_pkt) {
				q->last_pkt = nullptr;
			}
			q->nb_packets--;
			q->size -= pkt1->pkt.size;
			*pkt = pkt1->pkt;
			av_free(pkt1);
			ret = 1;
			break;
		}
		else if (!block) {
			ret = 0;
			break;
		}
		else {
			SDL_CondWait(q->cond, q->mutex);
		}
	}
		SDL_UnlockMutex(q->mutex);
		return ret;
}

int decode_interrupt_cb(void) {
	return thread_exit;
}
void getFilePath(char path[])
{
	char file[255];

	FILE *fp;

	fp = fopen("data/path.txt","rb+");
	fscanf(fp,"%s", file);

	sprintf(path, "data/resources/%s", file);

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