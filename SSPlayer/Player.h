#pragma once

#include "Thread.h"
#include "Decoder.h"
#include "Picture.h"


#include <string>

class Player_GUI;

class Player
{
friend Player_GUI;

public:
	Player() :m_bRunning(true) { init(); };
	~Player();

	void Running();

private:
	int init();
	void update();
	void render();
	void events();
	bool isrunning() { return m_bRunning; };

	void update_running() { m_bRunning = !thread_exit; };
	void update_decode();
	void update_sdlRect();
	void update_infor_volume();

	void render_Draw(SDL_Texture * tex, SDL_Rect & dstRect, SDL_Rect * clip, float angle, int xPivot, int yPivot, SDL_RendererFlip flip);
	void render_Draw(SDL_Texture * tex, int x, int y);
	SDL_Texture * render_Text(const std::string & message, const std::string & fontFile, int fontSize, SDL_Color color);
	void render_Text(const std::string & message, const std::string & fontFile, int x, int y, int fontSize, SDL_Color color);
	void render_infor();

	void mouseWheelevent();
	void keyDownevent();
	void Windowevent();

	bool isFullScreen();

	void setVolumeUP();
	void setVolumeDown();
	void setPause();
	void setVolumeMute();
	void setFullScreen();
	void exitFullScreen();
	void setExit();
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
	Player_GUI *gui;

	int cnt_time_infor;

	int mouse_x;
	int mouse_y;

	char buffer_infor[255];

	bool m_bRunning;

};