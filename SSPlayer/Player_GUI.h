#pragma once
#include "Player_Button.h"

class Player;

class Player_GUI
{
friend Player;

public:
	Player_GUI(Player *player);
	~Player_GUI();

	void event(SDL_Event event);
	void update();
	void render();

private:
	void update_CounterSecond();
	void update_Rect_DownButton();
	void update_MouseLAction();
	void update_Button();

	void render_DownButton();
	void render_Button();

	void mouseButtonDownevent(SDL_Event event);
	void mouseMotionevent(SDL_Event event);
	void buttonMotionevent(SDL_Event event);

	void buttonSingleClick();
private:
	Player *player;
	
	Player_Button *buttonPlaye;
	Player_Button *buttonStop;
	Player_Button *buttonVolume;
	Player_Button *buttonScreenShot;
	Player_Button *buttonFullScreen;

	SDL_Rect rect_DownButton;

	int cnt_time_second;
	int cnt_click_L;
	int cnt_time_L;
	int cnt_time_rect_down;
	int cnt_rect_down;

	int mouseClick_x;
	int mouseClick_y;

};

