#pragma once
#include "function.h"

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

	void render_DownButton();

	void mouseButtonDownevent(SDL_Event event);
	void mouseMotionevent(SDL_Event event);
private:
	Player *player;

	SDL_Rect rect_DownButton;

	int cnt_time_second;
	int cnt_click_L;
	int cnt_time_L;
	int cnt_time_rect_down;
	int cnt_rect_down;

};

