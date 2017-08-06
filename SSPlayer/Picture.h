#pragma once

extern "C"
{
#pragma comment(lib,"SDL2.lib")
#pragma comment(lib,"SDL2main.lib")

#include <SDL.h>
}

#include <string>

class Picture
{
public:
	Picture(SDL_Renderer *Ren);
	~Picture();

	//Load an image
	SDL_Texture * Picture::LoadImage(const std::string & file);
	void delimage(SDL_Texture* image);

private:
	SDL_Renderer *Renderer = nullptr;

public:
	SDL_Surface *icon = nullptr;

	SDL_Texture *play_white = nullptr;
	SDL_Texture *pause_white = nullptr;
	SDL_Texture *stop_white = nullptr;
	SDL_Texture *volume_white = nullptr;
	SDL_Texture *mute_white = nullptr;
	SDL_Texture *fullscreen_white = nullptr;
	SDL_Texture *exitfullscreen_white = nullptr;
	SDL_Texture *screenshot_white = nullptr;
	
	SDL_Texture *play_blue = nullptr;
	SDL_Texture *pause_blue = nullptr;
	SDL_Texture *stop_blue = nullptr;
	SDL_Texture *volume_blue = nullptr;
	SDL_Texture *mute_blue = nullptr;
	SDL_Texture *fullscreen_blue = nullptr;
	SDL_Texture *exitfullscreen_blue = nullptr;
	SDL_Texture *screenshot_blue = nullptr;

};

