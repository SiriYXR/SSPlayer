#include "Picture.h"

extern "C"
{
#pragma comment(lib,"SDL2_image.lib")
#include <SDL_image.h>
}

Picture::Picture(SDL_Renderer * Ren)
{
	IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG);

	icon = IMG_Load("data/picture/icon/SSPlayer_red.png");
}

Picture::~Picture()
{
	SDL_FreeSurface(icon);
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
