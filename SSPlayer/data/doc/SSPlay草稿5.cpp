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
//#pragma comment(lib,"SDL2_image.lib")
#pragma comment(lib,"SDL2_ttf.lib")

#include <SDL.h>
//#include <SDL_image.h>
#include <SDL_ttf.h>

#include <windows.h>
}
#include <string>


#include "acyclebuffer.h"
#include "Picture.h"

//Refresh event
#define SFM_REFRESH_EVENT (SDL_USEREVENT+1)

#define MAX_AUDIO_FRAME_SIZE 192000//1 second of 48khz 32bit audio 

//Font path
#define Font_songti "data/font/simsun.ttc"
#define Font_kaiti "data/font/simkai.ttf"

bool thread_exit = false;
bool thread_pause = false;
bool isMute=false;

double fps = 25;
int silence = SDL_MIX_MAXVOLUME / 2;

uint8_t *copy_buf;

class Timeer
{
public:
	static int sfp_refresh_thread(void *opaque)
	{
		thread_exit = false;

		while (!thread_exit) {
			SDL_Event event;
			event.type = SFM_REFRESH_EVENT;
			SDL_PushEvent(&event);
			SDL_Delay(1000 / fps);
		}

		return 0;
	}

	static void fill_audio(void * udata, Uint8 * stream, int len)
	{
		SDL_memset(stream, 0, len);
		int n = out_buffer_audio.read((char*)copy_buf, len);
		if (isMute) {
			SDL_MixAudio(stream, copy_buf, n, 0);
		}
		else {
			SDL_MixAudio(stream, copy_buf, n, SDL_MIX_MAXVOLUME - silence);
		}
	}

	static bool thread_exit;
	static bool thread_pause;
	static ACycleBuffer out_buffer_audio;
};

bool Timeer::thread_exit;
bool Timeer::thread_pause;
ACycleBuffer Timeer::out_buffer_audio(MAX_AUDIO_FRAME_SIZE * 10);

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

class Player
{
public:
	Player() :m_bRunning(true) { init(); };
	~Player() {
		SDL_CloseAudio();
		SDL_Quit();
	};
	bool init();
	void update();
	void render();
	void events();
	bool running() { return m_bRunning; };

private:
	void update_running() { m_bRunning = !thread_exit; };
	void update_decode();
	void update_sdlRect();
	void update_MouseLAction();
	void update_infor_volume();

	void render_Draw(SDL_Texture * tex, SDL_Rect & dstRect, SDL_Rect * clip, float angle, int xPivot, int yPivot, SDL_RendererFlip flip);
	void render_Draw(SDL_Texture * tex, int x, int y);
	SDL_Texture * render_Text(const std::string & message, const std::string & fontFile, int fontSize, SDL_Color color);
	void render_Text(const std::string & message, const std::string & fontFile, int x, int y, int fontSize, SDL_Color color);
	void render_infor();


private:
	//sdl变量----------------------------------------------
	int screen_w, screen_h;
	SDL_Window *screen;
	SDL_Renderer *sdlRenderer;
	SDL_Texture *sdlTexture;
	SDL_Rect sdlRect;
	SDL_Thread *video_tid;
	SDL_Event event;
	SDL_AudioSpec wanted_spec;

	Decoder decoder;
	Picture *sdlpicture;

	int counter_click_L;
	int counter_time_L;
	int counter_time_infor;

	char buffer_infor[255];

	bool m_bRunning;

};

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
					if (rr * 4 > Timeer::out_buffer_audio.restSpace()) {
						SDL_Delay(1);
					}
					else {
						Timeer::out_buffer_audio.write((char*)out_buffer, rr * 4);
						break;
					}
				}
			}
		}
		av_free_packet(packet);
	}
	return false;
}

bool Player::init()
{
	//------------------------------------SDL初始化---------------------------------------------------
	//初始化视音频等模块
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
		printf("Could not initialize SDL - %s\n", SDL_GetError());
		return -1;
	}
	if (TTF_Init() == -1)
	{
		printf("Could not initialize TTF - %s\n", SDL_GetError());
		return -1;
	}

	//初始化窗口大小
	screen_w = decoder.pCodecCtx->width;
	screen_h = decoder.pCodecCtx->height;
	//创建窗口
	sprintf(decoder.filebuffer, "%s - SSPlayer", decoder.filename);
	screen = SDL_CreateWindow(decoder.filebuffer, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, screen_w, screen_h, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	if (!screen) {
		printf("SDL:could not create window - exiting:%s\n", SDL_GetError());
		return -1;
	}
	SDL_SetWindowMinimumSize(screen, 300, 200);

	//创建渲染器
	sdlRenderer = SDL_CreateRenderer(screen, -1, 0);

	//init Picture
	sdlpicture = new Picture(sdlRenderer);
	if (sdlpicture == nullptr)
		throw std::runtime_error("Picture Init Failed");

	//设置图标
	SDL_SetWindowIcon(screen, sdlpicture->icon);

	//创建纹理
	sdlTexture = SDL_CreateTexture(sdlRenderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, decoder.pCodecCtx->width, decoder.pCodecCtx->height);
	//初始化矩形
	sdlRect.x = 0;
	sdlRect.y = 0;
	sdlRect.w = screen_w;
	sdlRect.h = screen_h;

	//建立音频信息
	wanted_spec.freq = decoder.out_sample_rate;
	wanted_spec.format = AUDIO_S16SYS;
	wanted_spec.channels = decoder.out_channels;
	wanted_spec.silence = 0;
	wanted_spec.samples = decoder.out_nb_samples;
	wanted_spec.callback = Timeer::fill_audio;
	wanted_spec.userdata = decoder.aCodecCtx;

	if (SDL_OpenAudio(&wanted_spec, nullptr) < 0) {
		printf("SDL_OpenAudio:%s\n", SDL_GetError());
		return -1;
	}

	//FIX:Some Codec's Context Information is missing  
	decoder.in_channel_layout = av_get_default_channel_layout(decoder.aCodecCtx->channels);

	//Swr
	decoder.au_convert_ctx = swr_alloc();
	decoder.au_convert_ctx = swr_alloc_set_opts(decoder.au_convert_ctx, decoder.out_channel_layout, decoder.out_sample_fmt, decoder.out_sample_rate, decoder.in_channel_layout, decoder.aCodecCtx->sample_fmt, decoder.aCodecCtx->sample_rate, 0, nullptr);
	swr_init(decoder.au_convert_ctx);

	//输出视频信息
	printf("File Information------------------\n");
	av_dump_format(decoder.pFormatCtx, 0, decoder.filepath, false);
	printf("------------------------------------\n");

	counter_click_L=0;
	counter_time_L=0;
	counter_time_infor = 0;

	return true;
}

void Player::update()
{
	update_running();

	//同步窗口尺寸
	update_sdlRect();

	update_infor_volume();

	update_MouseLAction();
}

void Player::render()
{
	SDL_RenderClear(sdlRenderer);

	SDL_RenderCopy(sdlRenderer, sdlTexture, nullptr, &sdlRect);
	
	render_infor();

	SDL_RenderPresent(sdlRenderer);
}

void Player::events()
{
	if (SDL_PollEvent(&event)) {

		switch (event.type)
		{
		case SFM_REFRESH_EVENT:
			if (!thread_pause)
				update_decode();
			break;
		case SDL_MOUSEBUTTONDOWN:
			if (event.button.button == SDL_BUTTON_LEFT) {
				counter_click_L++;
				counter_time_L = 0;
			}
			else if (event.button.button == SDL_BUTTON_MIDDLE) {
				
			}
			break;
		case SDL_MOUSEWHEEL:
			if (event.wheel.y > 0) {
				silence -= 6;
				if (silence <0)
					silence = 0;
				counter_time_infor = 1000;
				isMute = false;
			}
			else if (event.wheel.y < 0) {
				silence += 6;
				if (silence > 128)
					silence = 128;
				counter_time_infor = 1000;
				isMute = false;
			}
			break;
		case SDL_KEYDOWN:
			if (event.key.keysym.sym == SDLK_SPACE) {
				thread_pause = !thread_pause;
			}
			else if (event.key.keysym.sym == SDLK_ESCAPE) {
				if (SDL_GetWindowFlags(screen) == 0x1627) {
					SDL_SetWindowFullscreen(screen, 0);
				}
				else
					thread_exit = true;
			}
			else if (event.key.keysym.sym == SDLK_UP) {
				silence -= 6;
				if (silence <0)
					silence = 0;
				counter_time_infor = 1000;
				isMute = false;
			}
			else if (event.key.keysym.sym == SDLK_DOWN) {
				silence += 6;
				if (silence > 128)
					silence = 128;
				counter_time_infor = 1000;
				isMute = false;
			}
			else if (event.key.keysym.sym == SDLK_RIGHT) {

			}
			else if (event.key.keysym.sym == SDLK_LEFT) {

			}
			else if (event.key.keysym.sym == SDLK_v) {
				counter_time_infor = 1000;
				isMute = !isMute;
			}
			else if (event.key.keysym.sym == SDLK_r) {
				SDL_SetWindowFullscreen(screen, 0);
				SDL_SetWindowSize(screen, decoder.pCodecCtx->width, decoder.pCodecCtx->height);
			}
			else if (event.key.keysym.sym == SDLK_f) {
				SDL_SetWindowFullscreen(screen, SDL_WINDOW_FULLSCREEN_DESKTOP);
			}
			break;
		case SDL_WINDOWEVENT:
			//If Resize
			SDL_GetWindowSize(screen, &screen_w, &screen_h);
			break;
		case SDL_QUIT:
			thread_exit = true;
			break;
		default:
			break;
		}

	}
}

void Decoder::init_getFilePath(char path[], char name[])
{
	FILE *fp;

	fp = fopen("data/path.txt", "rb+");
	fscanf(fp, "%s", name);

	sprintf(path, "data/resources/%s", name);

	fclose(fp);
}

void Player::update_decode()
{
	decoder.decode();
	SDL_UpdateTexture(sdlTexture, nullptr, decoder.pFrameYUV->data[0], decoder.pFrameYUV->linesize[0]);
}

void Player::update_sdlRect()
{
	if ((double)screen_w / (double)screen_h < (double)decoder.pCodecCtx->width / (double)decoder.pCodecCtx->height) {
		sdlRect.w = screen_w;
		sdlRect.h = decoder.pCodecCtx->height *((double)screen_w / (double)decoder.pCodecCtx->width);
		sdlRect.x = 0;
		sdlRect.y = (screen_h - sdlRect.h) / 2;
	}
	else if ((double)screen_w / (double)screen_h > (double)decoder.pCodecCtx->width / (double)decoder.pCodecCtx->height) {
		sdlRect.w = decoder.pCodecCtx->width*((double)screen_h / (double)decoder.pCodecCtx->height);
		sdlRect.h = screen_h;
		sdlRect.x = (screen_w - sdlRect.w) / 2;
		sdlRect.y = 0;
	}
	else {
		sdlRect.w = screen_w;
		sdlRect.h = screen_h;
		sdlRect.x = 0;
		sdlRect.y = 0;
	}
}

void Player::update_MouseLAction()
{
	if (counter_time_L == 400)
		counter_time_L == 0;

	counter_time_L++;

	if (counter_click_L&&counter_time_L > 100) {
		thread_pause = !thread_pause;
		counter_click_L = 0;
	}
	else if (counter_click_L == 2 && counter_time_L < 100) {
		if (SDL_GetWindowFlags(screen) == 0x1627) {
			SDL_SetWindowFullscreen(screen, 0);
		}
		else
			SDL_SetWindowFullscreen(screen, SDL_WINDOW_FULLSCREEN_DESKTOP);
		counter_click_L = 0;
	}
	
}

void Player::update_infor_volume()
{
	if (isMute) {
		sprintf(buffer_infor, "Volume:Mute");
	}
	else {
		float volume = (float)(128 - silence) / 128 * 100;
		sprintf(buffer_infor, "Volume:%.2f%%", volume);
	}
}

/**
*  Draw a SDL_Texture to the screen at dstRect with various other options
*  @param tex The SDL_Texture to draw
*  @param dstRect The destination position and width/height to draw the texture with
*  @param clip The clip to apply to the image, if desired
*  @param angle The rotation angle to apply to the texture, default is 0
*  @param xPivot The x coordinate of the pivot, relative to (0, 0) being center of dstRect
*  @param yPivot The y coordinate of the pivot, relative to (0, 0) being center of dstRect
*  @param flip The flip to apply to the image, default is none
*/
void Player::render_Draw(SDL_Texture * tex, SDL_Rect & dstRect, SDL_Rect * clip, float angle, int xPivot, int yPivot, SDL_RendererFlip flip)
{
	//Convert pivot pos from relative to object's center to screen space
	xPivot += dstRect.w / 2;
	yPivot += dstRect.h / 2;
	//SDL expects an SDL_Point as the pivot location
	SDL_Point pivot = { xPivot, yPivot };
	//Draw the texture
	SDL_RenderCopyEx(sdlRenderer, tex, clip, &dstRect, angle, &pivot, flip);
}

void Player::render_Draw(SDL_Texture * tex, int x, int y)
{
	SDL_Rect dstRect;
	dstRect.x = x;
	dstRect.y = y;
	SDL_QueryTexture(tex, NULL, NULL, &dstRect.w, &dstRect.h);
	SDL_RenderCopyEx(sdlRenderer, tex, NULL, &dstRect, 0.0, NULL, SDL_FLIP_NONE);
}

/**
*  Generate a texture containing the message we want to display
*  @param message The message we want to display
*  @param fontFile The font we want to use to render the text
*  @param color The color we want the text to be
*  @param fontSize The size we want the font to be
*  @return An SDL_Texture* to the rendered message
*/
SDL_Texture * Player::render_Text(const std::string & message, const std::string & fontFile, int fontSize, SDL_Color color)
{
	//Open the font
	TTF_Font *font = nullptr;
	font = TTF_OpenFont(fontFile.c_str(), fontSize);
	if (font == nullptr)
		throw std::runtime_error("Failed to load font: " + fontFile + TTF_GetError());

	//Render the message to an SDL_Surface, as that's what TTF_RenderText_X returns
	SDL_Surface *surf = TTF_RenderText_Blended(font, message.c_str(), color);
	SDL_Texture *texture = SDL_CreateTextureFromSurface(sdlRenderer, surf);
	//Clean up unneeded stuff
	SDL_FreeSurface(surf);
	TTF_CloseFont(font);

	return texture;
}

void Player::render_Text(const std::string & message, const std::string & fontFile, int x, int y, int fontSize, SDL_Color color)
{
	render_Draw(render_Text(message, fontFile, fontSize, color), x, y);
}

void Player::render_infor()
{
	if (counter_time_infor) {
		render_Text(buffer_infor, Font_songti, 20, 40, 18, SDL_Color{ 0xFF,0xFF,0x00 });
		counter_time_infor--;
	}

}


int main(int argc, char **argv)
{
	Player ssplayer;
	SDL_CreateThread(Timeer::sfp_refresh_thread, nullptr, nullptr);
	for (; ssplayer.running();SDL_Delay(1)) {
		ssplayer.events();

		ssplayer.update();

		ssplayer.render();
	}

	return 0;
}