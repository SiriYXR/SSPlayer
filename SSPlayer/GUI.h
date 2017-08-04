#pragma once
#include "Player.h"

class Player;

class GUI
{
public:
	GUI();
	~GUI();

	void Running();
	
private:
	bool init();

	void update();
	void update_isrun();
	void update_MouseLAction();

	void render();

	void events();
	void mouseButtonevent();
	
	bool isRunning();

private:
	Player player;
	SDL_Event event;

	int counter_click_L;
	int counter_time_L;

	bool isrunning;
};

