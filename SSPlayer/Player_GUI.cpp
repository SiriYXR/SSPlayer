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

	mouseClick_x = player->mouse_x;
	mouseClick_y = player->mouse_y;

	buttonPlaye = new Player_Button(player->sdlRenderer,20,rect_DownButton.y+30,40,40,player->sdlpicture->play_white,player->sdlpicture->play_blue, player->sdlpicture->pause_white, player->sdlpicture->pause_blue);
	buttonPlaye->setButtonInfor("Play","Pause");

	buttonStop = new Player_Button(player->sdlRenderer, 70, rect_DownButton.y + 30, 40, 40, player->sdlpicture->stop_white, player->sdlpicture->stop_blue);
	buttonStop->setButtonInfor("Stop", "Stop");

	buttonVolume = new Player_Button(player->sdlRenderer, player->screen_w - 150, rect_DownButton.y + 30, 40, 40, player->sdlpicture->volume_white, player->sdlpicture->volume_blue, player->sdlpicture->mute_white, player->sdlpicture->mute_blue);
	buttonVolume->setButtonInfor("Volume", "Mute");

	buttonScreenShot = new Player_Button(player->sdlRenderer, player->screen_w - 100, rect_DownButton.y + 30, 40, 40, player->sdlpicture->screenshot_white, player->sdlpicture->screenshot_blue);
	buttonScreenShot->setButtonInfor("ScreenShot", "ScreenShot");

	buttonFullScreen = new Player_Button(player->sdlRenderer, player->screen_w - 50, rect_DownButton.y + 30, 40, 40, player->sdlpicture->fullscreen_white, player->sdlpicture->fullscreen_blue, player->sdlpicture->exitfullscreen_white, player->sdlpicture->exitfullscreen_blue);
	buttonFullScreen->setButtonInfor("FullScreen", "ExitFullScreen");
}

Player_GUI::~Player_GUI()
{
	this->player = nullptr;

	delete(buttonPlaye);
	delete(buttonStop);
	delete(buttonVolume);
	delete(buttonScreenShot);
	delete(buttonFullScreen);
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
	update_Button();
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
	if ((player->mouse_x > 0 && player->mouse_x < player->screen_w - 1) && (player->mouse_y > player->screen_h - 80 && player->mouse_y < player->screen_h - 1)) {
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

	if (mouseClick_y < player->screen_h - 80) {
		if (cnt_click_L&&cnt_time_L > 150) {
			player->setPause();
			cnt_click_L = 0;
		}
		else if (cnt_click_L == 2 && cnt_time_L < 150) {
			player->setFullScreen();
			cnt_click_L = 0;
		}
	}
	else {
		if (cnt_click_L&&cnt_time_L > 150) {
			buttonSingleClick();
			cnt_click_L = 0;
		}
	}
		

}

void Player_GUI::update_Button()
{
	buttonPlaye->setRect(rect_DownButton.x + 20, rect_DownButton.y + 30, 40, 40);
	thread_pause ? buttonPlaye->setClickState(false) : buttonPlaye->setClickState(true);
	
	buttonStop->setRect(rect_DownButton.x + 70, rect_DownButton.y + 30, 40, 40);

	buttonVolume->setRect(player->screen_w - 150, rect_DownButton.y + 30, 40, 40);
	!isMute ? buttonVolume->setClickState(false) : buttonVolume->setClickState(true);
	if(!isMute)
		silence!=128 ? buttonVolume->setClickState(false) : buttonVolume->setClickState(true);

	buttonScreenShot->setRect(player->screen_w - 100, rect_DownButton.y + 30, 40, 40);

	buttonFullScreen->setRect(player->screen_w - 50, rect_DownButton.y + 30, 40, 40);
	!player->isFullScreen() ? buttonFullScreen->setClickState(false) : buttonFullScreen->setClickState(true);
}

void Player_GUI::render_DownButton()
{
	//背景色 黑 半透明
	boxRGBA(player->sdlRenderer, rect_DownButton.x, rect_DownButton.y, player->screen_w, player->screen_h, 0x00, 0x00, 0x00, 150);

	//进度条
	boxRGBA(player->sdlRenderer, 10, rect_DownButton.y + 3, player->screen_w - 10, rect_DownButton.y + 6, 0xff, 0xff, 0xff, 250);

	render_Button();
}

void Player_GUI::render_Button()
{
	buttonPlaye->render();
	buttonStop->render();
	buttonVolume->render();
	buttonScreenShot->render();
	buttonFullScreen->render();
}

void Player_GUI::mouseButtonDownevent(SDL_Event event)
{
	if (event.button.button == SDL_BUTTON_LEFT) {
		cnt_click_L++;
		cnt_time_L = 0;
		mouseClick_x = player->mouse_x;
		mouseClick_y = player->mouse_y;
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

	buttonMotionevent(event);
}

void Player_GUI::buttonMotionevent(SDL_Event event)
{
	buttonPlaye->event(event);
	buttonStop->event(event);
	buttonVolume->event(event);
	buttonScreenShot->event(event);
	buttonFullScreen->event(event);
}

void Player_GUI::buttonSingleClick()
{
	if (buttonPlaye->singleClick(mouseClick_x, mouseClick_y)) {
		player->setPause();
	}

	if (buttonStop->singleClick(mouseClick_x, mouseClick_y)) {
		player->setExit();
	}
	
	if (buttonVolume->singleClick(mouseClick_x, mouseClick_y)) {
		player->setVolumeMute();
	}

	if (buttonScreenShot->singleClick(mouseClick_x, mouseClick_y)) {
		player->Screenshot();
	}

	if (buttonFullScreen->singleClick(mouseClick_x, mouseClick_y)) {
		player->setFullScreen();
	}
}
