#include "GUI.h"



GUI::GUI()
{
	isrunning = true;
	init();
}

GUI::~GUI()
{

}

void GUI::Running()
{

	for (; isRunning(); SDL_Delay(1)) {

		update();

		render();

		events();
	}

	thread_exit = true;
}

bool GUI::init()
{
	SDL_CreateThread(Thread::sfp_refresh_thread, nullptr, nullptr);

	counter_click_L = 0;
	counter_time_L = 0;

	return true;
}

void GUI::update()
{
	player.update();
	update_isrun();

	update_MouseLAction();
}

void GUI::update_isrun()
{
	isrunning = player.running();
}

void GUI::update_MouseLAction()
{
	if (counter_time_L == 400)
		counter_time_L = 0;

	counter_time_L++;

	if (counter_click_L&&counter_time_L > 100) {
		player.setPause();
		counter_click_L = 0;
	}
	else if (counter_click_L == 2 && counter_time_L < 100) {
		player.setDoubleClick();
		counter_click_L = 0;
	}

}

void GUI::render()
{
	player.render();
}

void GUI::events()
{
	if (SDL_PollEvent(&event)) {

		switch (event.type)
		{
		case SDL_MOUSEBUTTONDOWN:
			mouseButtonevent();
			break;
		default:
			player.events(event);
			break;
		}
	}
}

void GUI::mouseButtonevent()
{

	if (event.button.button == SDL_BUTTON_LEFT) {
		counter_click_L++;
		counter_time_L = 0;
	}
	else if (event.button.button == SDL_BUTTON_MIDDLE) {

	}

}

bool GUI::isRunning()
{
	return isrunning;
}
