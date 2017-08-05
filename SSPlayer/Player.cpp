#include "Player.h"

extern "C"
{
#pragma comment(lib,"SDL2_image.lib")
#include <SDL_image.h>

#include <sys/timeb.h>
}

#include <string>


void Player::Running()
{

	SDL_CreateThread(Thread::sfp_refresh_thread, nullptr, nullptr);
	for (; isrunning(); SDL_Delay(1)) {
		events();

		update();

		render();
	}

}

bool Player::init()
{
	//------------------------------------SDL初始化---------------------------------------------------
	//初始化视音频等模块
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
		printf("Could not initialize SDL - %s\n", SDL_GetError());
		return -1;
	}
	if (TTF_Init() == -1)
	{
		printf("Could not initialize TTF - %s\n", SDL_GetError());
		return -1;
	}

	//初始化窗口大小
	screen_w = decoder.pCodecCtx->width;
	screen_h = decoder.pCodecCtx->height;
	//创建窗口
	sprintf(decoder.filebuffer, "%s - SSPlayer", decoder.filename);
	screen = SDL_CreateWindow(decoder.filebuffer, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, screen_w, screen_h, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	if (!screen) {
		printf("SDL:could not create window - exiting:%s\n", SDL_GetError());
		return -1;
	}
	SDL_SetWindowMinimumSize(screen, 300, 200);

	//创建渲染器
	sdlRenderer = SDL_CreateRenderer(screen, -1, 0);

	//init Picture
	sdlpicture = new Picture(sdlRenderer);
	if (sdlpicture == nullptr)
		throw std::runtime_error("Picture Init Failed");

	//设置图标
	SDL_SetWindowIcon(screen, sdlpicture->icon);

	//创建纹理
	sdlTexture = SDL_CreateTexture(sdlRenderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, decoder.pCodecCtx->width, decoder.pCodecCtx->height);
	//初始化矩形
	sdlRect.x = 0;
	sdlRect.y = 0;
	sdlRect.w = screen_w;
	sdlRect.h = screen_h;

	//建立音频信息
	wanted_spec.freq = decoder.out_sample_rate;
	wanted_spec.format = AUDIO_S16SYS;
	wanted_spec.channels = decoder.out_channels;
	wanted_spec.silence = 0;
	wanted_spec.samples = decoder.out_nb_samples;
	wanted_spec.callback = Thread::fill_audio;
	wanted_spec.userdata = decoder.aCodecCtx;

	if (SDL_OpenAudio(&wanted_spec, nullptr) < 0) {
		printf("SDL_OpenAudio:%s\n", SDL_GetError());
		return -1;
	}

	//FIX:Some Codec's Context Information is missing  
	decoder.in_channel_layout = av_get_default_channel_layout(decoder.aCodecCtx->channels);

	//Swr
	decoder.au_convert_ctx = swr_alloc();
	decoder.au_convert_ctx = swr_alloc_set_opts(decoder.au_convert_ctx, decoder.out_channel_layout, decoder.out_sample_fmt, decoder.out_sample_rate, decoder.in_channel_layout, decoder.aCodecCtx->sample_fmt, decoder.aCodecCtx->sample_rate, 0, nullptr);
	swr_init(decoder.au_convert_ctx);

	//输出视频信息
	printf("File Information------------------\n");
	av_dump_format(decoder.pFormatCtx, 0, decoder.filepath, false);
	printf("------------------------------------\n");

	counter_click_L = 0;
	counter_time_L = 0;
	counter_time_infor = 0;

	return true;
}

void Player::update()
{
	update_running();

	//同步窗口尺寸
	update_sdlRect();

	update_MouseLAction();
}

void Player::render()
{
	SDL_RenderClear(sdlRenderer);

	SDL_RenderCopy(sdlRenderer, sdlTexture, nullptr, &sdlRect);

	render_infor();

	SDL_RenderPresent(sdlRenderer);
}

void Player::events()
{
	if (SDL_PollEvent(&event)) {

		switch (event.type)
		{
		case SFM_REFRESH_EVENT:
			if (!thread_pause)
				update_decode();
			break;
		case SDL_MOUSEBUTTONDOWN:
			if (event.button.button == SDL_BUTTON_LEFT) {
				counter_click_L++;
				counter_time_L = 0;
			}
			else if (event.button.button == SDL_BUTTON_MIDDLE) {

			}
			break;
		case SDL_MOUSEWHEEL:
			if (event.wheel.y > 0) {
				setVolumeUP();
			}
			else if (event.wheel.y < 0) {
				setVolumeDown();
			}
			break;
		case SDL_KEYDOWN:
			if (event.key.keysym.sym == SDLK_SPACE) {
				thread_pause = !thread_pause;
			}
			else if (event.key.keysym.sym == SDLK_ESCAPE) {
				if (SDL_GetWindowFlags(screen) == 0x1627) {
					SDL_SetWindowFullscreen(screen, 0);
				}
				else
					thread_exit = true;
			}
			else if (event.key.keysym.sym == SDLK_UP) {
				setVolumeUP();
			}
			else if (event.key.keysym.sym == SDLK_DOWN) {
				setVolumeDown();
			}
			else if (event.key.keysym.sym == SDLK_RIGHT) {
				
			}
			else if (event.key.keysym.sym == SDLK_LEFT) {

			}
			else if (event.key.keysym.sym == SDLK_v) {
				setVolumeMute();
			}
			else if (event.key.keysym.sym == SDLK_r) {
				SDL_SetWindowFullscreen(screen, 0);
				SDL_SetWindowSize(screen, decoder.pCodecCtx->width, decoder.pCodecCtx->height);
			}
			else if (event.key.keysym.sym == SDLK_s) {
				Screenshot();
			}
			else if (event.key.keysym.sym == SDLK_f) {
				SDL_SetWindowFullscreen(screen, SDL_WINDOW_FULLSCREEN_DESKTOP);
			}
			break;
		case SDL_WINDOWEVENT:
			//If Resize
			SDL_GetWindowSize(screen, &screen_w, &screen_h);
			break;
		case SDL_QUIT:
			thread_exit = true;
			break;
		default:
			break;
		}

	}
}

void Player::update_decode()
{
	decoder.decode();
	SDL_UpdateTexture(sdlTexture, nullptr, decoder.pFrameYUV->data[0], decoder.pFrameYUV->linesize[0]);
}

void Player::update_sdlRect()
{
	if ((double)screen_w / (double)screen_h < (double)decoder.pCodecCtx->width / (double)decoder.pCodecCtx->height) {
		sdlRect.w = screen_w;
		sdlRect.h = decoder.pCodecCtx->height *((double)screen_w / (double)decoder.pCodecCtx->width);
		sdlRect.x = 0;
		sdlRect.y = (screen_h - sdlRect.h) / 2;
	}
	else if ((double)screen_w / (double)screen_h > (double)decoder.pCodecCtx->width / (double)decoder.pCodecCtx->height) {
		sdlRect.w = decoder.pCodecCtx->width*((double)screen_h / (double)decoder.pCodecCtx->height);
		sdlRect.h = screen_h;
		sdlRect.x = (screen_w - sdlRect.w) / 2;
		sdlRect.y = 0;
	}
	else {
		sdlRect.w = screen_w;
		sdlRect.h = screen_h;
		sdlRect.x = 0;
		sdlRect.y = 0;
	}
}

void Player::update_MouseLAction()
{
	if (counter_time_L == 400)
		counter_time_L == 0;

	counter_time_L++;

	if (counter_click_L&&counter_time_L > 100) {
		thread_pause = !thread_pause;
		counter_click_L = 0;
	}
	else if (counter_click_L == 2 && counter_time_L < 100) {
		if (SDL_GetWindowFlags(screen) == 0x1627) {
			SDL_SetWindowFullscreen(screen, 0);
		}
		else
			SDL_SetWindowFullscreen(screen, SDL_WINDOW_FULLSCREEN_DESKTOP);
		counter_click_L = 0;
	}

}

void Player::update_infor_volume()
{
	if (isMute==true) {
		sprintf(buffer_infor, "Volume:Mute");
	}
	else {
		double volume = (128 - silence) / 128 * 100;
		sprintf(buffer_infor, "Volume:%.2f%%", volume);
	}
}

/**
*  Draw a SDL_Texture to the screen at dstRect with various other options
*  @param tex The SDL_Texture to draw
*  @param dstRect The destination position and width/height to draw the texture with
*  @param clip The clip to apply to the image, if desired
*  @param angle The rotation angle to apply to the texture, default is 0
*  @param xPivot The x coordinate of the pivot, relative to (0, 0) being center of dstRect
*  @param yPivot The y coordinate of the pivot, relative to (0, 0) being center of dstRect
*  @param flip The flip to apply to the image, default is none
*/
void Player::render_Draw(SDL_Texture * tex, SDL_Rect & dstRect, SDL_Rect * clip, float angle, int xPivot, int yPivot, SDL_RendererFlip flip)
{
	//Convert pivot pos from relative to object's center to screen space
	xPivot += dstRect.w / 2;
	yPivot += dstRect.h / 2;
	//SDL expects an SDL_Point as the pivot location
	SDL_Point pivot = { xPivot, yPivot };
	//Draw the texture
	SDL_RenderCopyEx(sdlRenderer, tex, clip, &dstRect, angle, &pivot, flip);
}

void Player::render_Draw(SDL_Texture * tex, int x, int y)
{
	SDL_Rect dstRect;
	dstRect.x = x;
	dstRect.y = y;
	SDL_QueryTexture(tex, NULL, NULL, &dstRect.w, &dstRect.h);
	SDL_RenderCopyEx(sdlRenderer, tex, NULL, &dstRect, 0.0, NULL, SDL_FLIP_NONE);
}

/**
*  Generate a texture containing the message we want to display
*  @param message The message we want to display
*  @param fontFile The font we want to use to render the text
*  @param color The color we want the text to be
*  @param fontSize The size we want the font to be
*  @return An SDL_Texture* to the rendered message
*/
SDL_Texture * Player::render_Text(const std::string & message, const std::string & fontFile, int fontSize, SDL_Color color)
{
	//Open the font
	TTF_Font *font = nullptr;
	font = TTF_OpenFont(fontFile.c_str(), fontSize);
	if (font == nullptr)
		throw std::runtime_error("Failed to load font: " + fontFile + TTF_GetError());

	//Render the message to an SDL_Surface, as that's what TTF_RenderText_X returns
	SDL_Surface *surf = TTF_RenderText_Blended(font, message.c_str(), color);
	SDL_Texture *texture = SDL_CreateTextureFromSurface(sdlRenderer, surf);
	//Clean up unneeded stuff
	SDL_FreeSurface(surf);
	TTF_CloseFont(font);

	return texture;
}

void Player::render_Text(const std::string & message, const std::string & fontFile, int x, int y, int fontSize, SDL_Color color)
{
	render_Draw(render_Text(message, fontFile, fontSize, color), x, y);
}

void Player::render_infor()
{
	if (counter_time_infor) {
		render_Text(buffer_infor, Font_songti, 20, 40, 18, SDL_Color{ 0xFF,0xFF,0x00 });
		counter_time_infor--;
	}

}

void Player::setVolumeUP()
{
	silence -= 6.4;
	if (silence < 0)
		silence = 0;
	counter_time_infor = 1000;
	isMute = false;
	update_infor_volume();
}

void Player::setVolumeDown()
{
	silence += 6.4;
	if (silence > 128)
		silence = 128;
	counter_time_infor = 1000;
	isMute = false;
	update_infor_volume();
}

void Player::setVolumeMute()
{
	counter_time_infor = 1000;
	isMute = !isMute;
	update_infor_volume();
}

int Player::Screenshot()
{
	int  depth, pitch;

	SDL_Rect rect;
	rect.x = 0;
	rect.y = 0;
	SDL_GetWindowSize(screen, &rect.w, &rect.h);

	pitch = rect.w * 4;   // 每行图像所占字节数  
	depth = 4 * 8;        // 每像素所占位数(R8位G8位B8位A8位)   

	int rmask, gmask, bmask, amask;
	rmask = 0x00FF0000; gmask = 0x0000FF00; bmask = 0x000000FF; amask = 0x00000000; // RGBA8888模式  

	SDL_Surface *surface;
	surface = SDL_CreateRGBSurface(0, rect.w, rect.h, depth, rmask, gmask, bmask, amask);
	if (NULL == surface)
		return 0;

	SDL_RenderReadPixels(sdlRenderer, &rect, 0, surface->pixels, pitch);

	timeb tp;
	tm *tm;

	ftime(&tp);
	tm = localtime(&tp.time);

	char buffer[255];
	sprintf(buffer, "data/screenshot/SSPlayer_ScreenShot_%.4d%.2d%.2d%.2d%.2d%.2d.png", 1900 + tm->tm_year, 1 + tm->tm_mon, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);

	IMG_SavePNG(surface, buffer);

	sprintf(buffer_infor, "ScreenShot Success");
	counter_time_infor = 1000;

	SDL_free(surface);

	return 1;
}
