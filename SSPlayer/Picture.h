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
};

