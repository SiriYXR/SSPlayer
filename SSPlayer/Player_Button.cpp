#include "Player_Button.h"

extern "C"
{
#pragma comment(lib,"SDL2_gfx.lib")

#include <SDL2_gfxPrimitives.h>

}

Player_Button::Player_Button(SDL_Renderer *renderer,int x, int y,int w,int h, SDL_Texture * Texture_unclick, SDL_Texture * Texture_unclick_on, SDL_Texture * Texture_click, SDL_Texture * Texture_click_on)
{
	this->renderer = renderer;

	this->Texture_unclick = Texture_unclick;
	this->Texture_unclick_on = Texture_unclick_on;
	if (Texture_click == nullptr || Texture_click_on == nullptr) {
		this->Texture_click = Texture_unclick;
		this->Texture_click_on = Texture_unclick_on;
	}
	else {
		this->Texture_click = Texture_click;
		this->Texture_click_on = Texture_click_on;
	}

	rect.x = x;
	rect.y = y;
	rect.w = w;
	rect.h = h;

	string_click = "button";
	string_unclick = "button";

	fontSize = 18;
	backr = 255;
	backg = 236;
	backb = 139;
	backa = 255;
	color = { 0x8b,0x8b,0x7a };

	ison = false;
	isclick = false;
}

Player_Button::~Player_Button()
{
	renderer = nullptr;
	Texture_click = nullptr;
	Texture_click_on = nullptr;
	Texture_unclick = nullptr;
	Texture_unclick_on = nullptr;
}

void Player_Button::event(SDL_Event event)
{
	switch (event.type)
	{
	case SDL_MOUSEMOTION:
		int x, y;
		SDL_GetMouseState(&x, &y);
		if ((x > rect.x&&x < rect.x + rect.w) && (y > rect.y&&y < rect.y + rect.h)) {
			ison = true;
		}
		else {
			ison = false;
		}
		break;
	default:
		break;
	}

}

bool Player_Button::singleClick(int x, int y)
{
	if ((x > rect.x&&x < rect.x + rect.w) && (y > rect.y&&y < rect.y + rect.h)) {
		isclick = !isclick;
		return true;
	}

	return false;
}

void Player_Button::render()
{
	int x, y;
	SDL_GetMouseState(&x, &y);

	if (isclick) {
		if (ison) {
			render_Draw(Texture_click_on, rect.x, rect.y);
			boxRGBA(renderer, x-20, rect.y - fontSize, x + fontSize*string_click.length()*0.55, rect.y, backr, backg, backb, backa);
			render_Text(string_click, Font_songti, x-15, rect.y - fontSize, fontSize, color);
		}
		else {
			render_Draw(Texture_click, rect.x, rect.y);
		}
	}
	else {
		if (ison) {
			render_Draw(Texture_unclick_on, rect.x, rect.y);
			boxRGBA(renderer, x-20, rect.y - fontSize, x + fontSize*string_click.length()*0.55, rect.y, backr, backg, backb, backa);
			render_Text(string_unclick, Font_songti, x-15, rect.y - fontSize, fontSize, color);
		}
		else {
			render_Draw(Texture_unclick, rect.x, rect.y);
		}
	}

}

void Player_Button::setClickState(bool isclick)
{
	this->isclick = isclick;
}

void Player_Button::setRect(int x, int y, int w, int h)
{
	rect.x = x;
	rect.y = y;
	rect.w = w;
	rect.h = h;
}

void Player_Button::setButtonInfor(const std::string & string_unclick, const std::string & string_click, int fontSize)
{
	this->string_click = string_click;
	this->string_unclick = string_unclick;
	this->fontSize = fontSize;
}

void Player_Button::render_Draw(SDL_Texture * tex, int x, int y)
{
	SDL_Rect dstRect;
	dstRect.x = x;
	dstRect.y = y;
	SDL_QueryTexture(tex, NULL, NULL, &dstRect.w, &dstRect.h);
	SDL_RenderCopyEx(renderer, tex, NULL, &dstRect, 0.0, NULL, SDL_FLIP_NONE);
}

SDL_Texture * Player_Button::render_Text(const std::string & message, const std::string & fontFile, int fontSize, SDL_Color color)
{
	//Open the font
	TTF_Font *font = nullptr;
	font = TTF_OpenFont(fontFile.c_str(), fontSize);
	if (font == nullptr)
		throw std::runtime_error("Failed to load font: " + fontFile + TTF_GetError());

	//Render the message to an SDL_Surface, as that's what TTF_RenderText_X returns
	SDL_Surface *surf = TTF_RenderText_Blended(font, message.c_str(), color);
	SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surf);
	//Clean up unneeded stuff
	SDL_FreeSurface(surf);
	TTF_CloseFont(font);

	return texture;
}

void Player_Button::render_Text(const std::string & message, const std::string & fontFile, int x, int y, int fontSize, SDL_Color color)
{
	render_Draw(render_Text(message, fontFile, fontSize, color), x, y);
}
