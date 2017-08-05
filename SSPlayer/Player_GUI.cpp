#include "Player.h"
#include "Player_GUI.h"

extern "C"
{
#pragma comment(lib,"SDL2_gfx.lib")

#include <SDL2_gfxPrimitives.h>

}

#include <Windows.h>

Player_GUI::Player_GUI(Player * player)
{
	this->player = player;

	rect_DownButton.x = 0;
	rect_DownButton.y = player->screen_h;
	rect_DownButton.w = player->screen_w;
	rect_DownButton.h = 80;

	cnt_time_second = 0;
	cnt_click_L = 0;
	cnt_time_L = 0;
	cnt_time_rect_down = 0;
	cnt_rect_down = 0;
}

Player_GUI::~Player_GUI()
{
	this->player = nullptr;
}

void Player_GUI::event(SDL_Event event)
{
	switch (event.type)
	{
	case SDL_MOUSEBUTTONDOWN:
		mouseButtonDownevent(event);
		break;
	case SDL_MOUSEMOTION:
		mouseMotionevent(event);
		break;
	default:
		break;
	}

}

void Player_GUI::update()
{
	update_CounterSecond();
	update_Rect_DownButton();
	update_MouseLAction();
}

void Player_GUI::render()
{
	render_DownButton();
}

void Player_GUI::update_CounterSecond()
{
	if (cnt_time_second < 1000) {
		cnt_time_second++;
	}
	else {
		cnt_time_second = 0;
	}
}

void Player_GUI::update_Rect_DownButton()
{
	if ((player->mouse_x>0 && player->mouse_x<player->screen_w - 1) && (player->mouse_y > player->screen_h - 80 && player->mouse_y<player->screen_h - 1)) {
		cnt_rect_down = player->screen_h - 80;
	}

	if (rect_DownButton.y < cnt_rect_down) {
		if (this->cnt_time_second % 2 == 0) {
			rect_DownButton.y++;
		}
	}
	else if (rect_DownButton.y > cnt_rect_down) {
		if (this->cnt_time_second % 2 == 0) {
			rect_DownButton.y--;
		}
	}

	rect_DownButton.w = player->screen_w;
	if (cnt_time_rect_down) {
		cnt_time_rect_down--;
	}
	else {
		cnt_rect_down = player->screen_h;
	}
}

void Player_GUI::update_MouseLAction()
{
	if (cnt_time_L == 400)
		cnt_time_L = 0;

	cnt_time_L++;

	if (cnt_click_L&&cnt_time_L > 150) {
		player->setPause();
		cnt_click_L = 0;
	}
	else if (cnt_click_L == 2 && cnt_time_L < 150) {
		player->setFullScreen();
		cnt_click_L = 0;
	}

}

void Player_GUI::render_DownButton()
{
	//±³¾°É« ºÚ °ëÍ¸Ã÷
	boxRGBA(player->sdlRenderer, rect_DownButton.x, rect_DownButton.y, player->screen_w, player->screen_h, 0x00, 0x00, 0x00, 150);

}

void Player_GUI::mouseButtonDownevent(SDL_Event event)
{
	if (event.button.button == SDL_BUTTON_LEFT) {
		cnt_click_L++;
		cnt_time_L = 0;
	}
	else if (event.button.button == SDL_BUTTON_MIDDLE) {

	}
}

void Player_GUI::mouseMotionevent(SDL_Event event)
{
	player->mouse_x = event.motion.x;
	player->mouse_y = event.motion.y;

	cnt_rect_down = player->screen_h - 80;
	cnt_time_rect_down = 2000;
}
