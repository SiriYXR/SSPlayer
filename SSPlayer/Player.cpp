#include "Player.h"
#include "Player_GUI.h"

extern "C"
{
#pragma comment(lib,"SDL2_image.lib")
#pragma comment(lib,"SDL2_gfx.lib")

#include <SDL2_gfxPrimitives.h>
#include <SDL_image.h>

#include <sys/timeb.h>
}

#include <string>


Player::~Player()
{
	delete sdlpicture;
	delete gui;
	SDL_CloseAudio();
	SDL_Quit();
}

void Player::Running()
{

	SDL_CreateThread(Thread::sfp_refresh_thread, nullptr, nullptr);
	for (; isrunning(); SDL_Delay(1)) {
		events();

		update();

		render();
	}

}

int Player::init()
{
	//------------------------------------SDL��ʼ��---------------------------------------------------
	//��ʼ������Ƶ��ģ��
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
		printf("Could not initialize SDL - %s\n", SDL_GetError());
		return -1;
	}
	if (TTF_Init() == -1)
	{
		printf("Could not initialize TTF - %s\n", SDL_GetError());
		return -1;
	}

	//��ʼ�����ڴ�С
	screen_w = decoder.pCodecCtx->width;
	screen_h = decoder.pCodecCtx->height;
	//��������
	sprintf(decoder.filebuffer, "%s - SSPlayer", decoder.filename);
	screen = SDL_CreateWindow(decoder.filebuffer, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, screen_w, screen_h, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	if (!screen) {
		printf("SDL:could not create window - exiting:%s\n", SDL_GetError());
		return -1;
	}
	SDL_SetWindowMinimumSize(screen, 300, 200);
	SDL_SetWindowMaximumSize(screen,1920,1017);

	//������Ⱦ��
	sdlRenderer = SDL_CreateRenderer(screen, -1, 0);

	//init Picture
	sdlpicture = new Picture(sdlRenderer);
	if (sdlpicture == nullptr)
		throw std::runtime_error("Picture Init Failed");

	gui = new Player_GUI(this);

	//����ͼ��
	SDL_SetWindowIcon(screen, sdlpicture->icon);

	//��������
	sdlTexture = SDL_CreateTexture(sdlRenderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, decoder.pCodecCtx->width, decoder.pCodecCtx->height);
	//��ʼ������
	sdlRect.x = 0;
	sdlRect.y = 0;
	sdlRect.w = screen_w;
	sdlRect.h = screen_h;

	//������Ƶ��Ϣ
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

	//�����Ƶ��Ϣ
	printf("File Information------------------\n");
	av_dump_format(decoder.pFormatCtx, 0, decoder.filepath, false);
	printf("------------------------------------\n");

	cnt_time_infor = 0;

	mouse_x = 0;
	mouse_y = 0;

	return true;
}

void Player::update()
{
	update_running();

	//ͬ�����ڳߴ�
	update_sdlRect();

	gui->update();
}

void Player::render()
{
	SDL_RenderClear(sdlRenderer);

	boxRGBA(sdlRenderer,0, 0, screen_w,screen_h, 0x00, 0x00, 0x00, 255);

	SDL_RenderCopy(sdlRenderer, sdlTexture, nullptr, &sdlRect);

	gui->render();

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
		case SDL_MOUSEMOTION:
			gui->event(event);
			break;
		case SDL_MOUSEWHEEL:
			mouseWheelevent();
			break;
		case SDL_KEYDOWN:
			keyDownevent();
			break;
		case SDL_WINDOWEVENT:
			Windowevent();
			break;
		case SDL_QUIT:
			setExit();
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
	if (cnt_time_infor) {
		render_Text(buffer_infor, Font_songti, 20, 40, 18, SDL_Color{ 0xFF,0xFF,0x00 });
		cnt_time_infor--;
	}

}

void Player::mouseWheelevent()
{
	if (event.wheel.y > 0) {
		setVolumeUP();
	}
	else if (event.wheel.y < 0) {
		setVolumeDown();
	}
}

void Player::keyDownevent()
{
	if (event.key.keysym.sym == SDLK_SPACE) {
		setPause();
	}
	else if (event.key.keysym.sym == SDLK_ESCAPE) {
		if (SDL_GetWindowFlags(screen) == 0x1627) {
			exitFullScreen();
		}
		else
			setExit();
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
		exitFullScreen();
		SDL_SetWindowSize(screen, decoder.pCodecCtx->width, decoder.pCodecCtx->height);
	}
	else if (event.key.keysym.sym == SDLK_s) {
		Screenshot();
	}
	else if (event.key.keysym.sym == SDLK_f) {
		setFullScreen();
	}
}

void Player::Windowevent()
{
	int sw, sh;
	sw = screen_w;
	sh = screen_h;
	//If Resize
	SDL_GetWindowSize(screen, &screen_w, &screen_h);

	if ((sw != screen_w) || (sh != screen_h)) {
		gui->rect_DownButton.y = screen_h - 80;
	}
	gui->rect_DownButton.w = screen_w;
	gui->cnt_rect_down = screen_h - 80;

}

bool Player::isFullScreen()
{
	if (SDL_GetWindowFlags(screen) == 0x1627) {
		return true;
	}
	return false;
}

void Player::setVolumeUP()
{
	silence -= 6.4;
	if (silence < 0)
		silence = 0;
	cnt_time_infor = 1000;
	isMute = false;
	update_infor_volume();
}

void Player::setVolumeDown()
{
	silence += 6.4;
	if (silence > 128)
		silence = 128;
	cnt_time_infor = 1000;
	isMute = false;
	update_infor_volume();
}

void Player::setVolumeMute()
{
	cnt_time_infor = 1000;
	isMute = !isMute;
	update_infor_volume();
}

void Player::setFullScreen()
{
	if (isFullScreen()) {
		exitFullScreen();
	}
	else {
		SDL_SetWindowFullscreen(screen, SDL_WINDOW_FULLSCREEN_DESKTOP);
	}
}

void Player::exitFullScreen()
{
	SDL_SetWindowFullscreen(screen, 0);
}

void Player::setPause()
{
	thread_pause = !thread_pause;
}

void Player::setExit()
{
	thread_exit = true;
}

int Player::Screenshot()
{
	int  depth, pitch;

	SDL_Rect rect;
	rect.x = 0;
	rect.y = 0;
	SDL_GetWindowSize(screen, &rect.w, &rect.h);

	pitch = rect.w * 4;   // ÿ��ͼ����ռ�ֽ���  
	depth = 4 * 8;        // ÿ������ռλ��(R8λG8λB8λA8λ)   

	int rmask, gmask, bmask, amask;
	rmask = 0x00FF0000; gmask = 0x0000FF00; bmask = 0x000000FF; amask = 0x00000000; // RGBA8888ģʽ  

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
	cnt_time_infor = 1000;

	SDL_free(surface);

	return 1;
}
