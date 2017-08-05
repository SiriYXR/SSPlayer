#pragma once

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

}

//Refresh event
#define SFM_REFRESH_EVENT (SDL_USEREVENT+1)

#define MAX_AUDIO_FRAME_SIZE 192000//1 second of 48khz 32bit audio 

//Font path
#define Font_songti "data/font/simsun.ttc"
#define Font_kaiti "data/font/simkai.ttf"

extern bool thread_exit;
extern bool thread_pause;
extern bool isMute;

extern double fps;
extern double silence;

extern uint8_t *copy_buf;