#pragma once

#include "Thread.h"
#include "Decoder.h"
#include "Picture.h"

#include <string>

class Player
{
public:
	Player() :m_bRunning(true) { init(); };
	~Player() {
		SDL_CloseAudio();
		SDL_Quit();
	};

	void Running();

private:
	bool init();
	void update();
	void render();
	void events();
	bool isrunning() { return m_bRunning; };

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

	void setVolumeUP();
	void setVolumeDown();
	void setVolumeMute();

	int Screenshot();

private:
	//sdl±‰¡ø----------------------------------------------
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