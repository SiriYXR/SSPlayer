#include "Picture.h"

extern "C"
{
#pragma comment(lib,"SDL2_image.lib")
#include <SDL_image.h>
}


Picture::Picture(SDL_Renderer * Ren)
{
	IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG);

	Renderer = Ren;

	icon = IMG_Load("data/picture/icon/SSPlayer_red.png");

	play_white =LoadImage("data/picture/button/play_white.png");
	pause_white =LoadImage("data/picture/button/pause_white.png");
	stop_white =LoadImage("data/picture/button/stop_white.png");
	volume_white =LoadImage("data/picture/button/volume_white.png");
	mute_white =LoadImage("data/picture/button/mute_white.png");
	fullscreen_white = LoadImage("data/picture/button/fullscreen_white.png");
	exitfullscreen_white = LoadImage("data/picture/button/exitfullscreen_white.png");
	screenshot_white =LoadImage("data/picture/button/screenshot_white.png");

	play_blue =LoadImage("data/picture/button/play_blue.png");
	pause_blue =LoadImage("data/picture/button/pause_blue.png");
	stop_blue =LoadImage("data/picture/button/stop_blue.png");
	volume_blue =LoadImage("data/picture/button/volume_blue.png");
	mute_blue =LoadImage("data/picture/button/mute_blue.png");
	fullscreen_blue = LoadImage("data/picture/button/fullscreen_blue.png");
	exitfullscreen_blue = LoadImage("data/picture/button/exitfullscreen_blue.png");
	screenshot_blue =LoadImage("data/picture/button/screenshot_blue.png");
}

Picture::~Picture()
{
	SDL_FreeSurface(icon);

	delimage(play_white);
	delimage(pause_white);
	delimage(stop_white);
	delimage(volume_white);
	delimage(mute_white);
	delimage(fullscreen_white);
	delimage(exitfullscreen_white);
	delimage(screenshot_white);

	delimage(play_blue);
	delimage(pause_blue);
	delimage(stop_blue);
	delimage(volume_blue);
	delimage(mute_blue);
	delimage(fullscreen_blue);
	delimage(exitfullscreen_blue);
	delimage(screenshot_blue);

	Renderer = nullptr;
}

/**
*  Loads an image directly to texture using SDL_image's
*  built in function IMG_LoadTexture
*  @param file The image file to load
*  @return SDL_Texture* to the loaded texture
*/
SDL_Texture * Picture::LoadImage(const std::string & file)
{
	SDL_Texture *texture = IMG_LoadTexture(Renderer, file.c_str());
	if (texture == nullptr) {
		throw std::runtime_error("LoadTexture");
	}
	return texture;
}

void Picture::delimage(SDL_Texture * image)
{
	SDL_DestroyTexture(image);
}
