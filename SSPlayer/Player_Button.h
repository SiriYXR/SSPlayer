#pragma once
#include "function.h"

#include <string>


class Player_Button
{
public:
	Player_Button(SDL_Renderer *renderer,int x,int y,int w,int h,SDL_Texture *Texture_unclick, SDL_Texture *Texture_unclick_on, SDL_Texture *Texture_click=nullptr, SDL_Texture *Texture_click_on=nullptr);
	~Player_Button();

	void event(SDL_Event event);
	bool singleClick(int x,int y);
	void render();

	void setClickState(bool isclick);
	void setRect(int x,int y,int w,int h);
	void setButtonInfor(const std::string & string_unclick, const std::string & string_click, int fontSize=18);
	
private:
	void Player_Button::render_Draw(SDL_Texture * tex, int x, int y);
	SDL_Texture * render_Text(const std::string & message, const std::string & fontFile, int fontSize, SDL_Color color);
	void render_Text(const std::string & message, const std::string & fontFile, int x, int y, int fontSize, SDL_Color color);

private:
	SDL_Renderer *renderer;
	SDL_Texture *Texture_unclick;
	SDL_Texture *Texture_unclick_on;
	SDL_Texture *Texture_click;
	SDL_Texture *Texture_click_on;
	SDL_Rect rect;

	std::string string_click;
	std::string string_unclick;

	int fontSize;
	int backr;
	int backg;
	int backb;
	int backa;
	SDL_Color color;

	bool ison;
	bool isclick;
};

