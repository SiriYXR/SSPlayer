#include "Thread.h"
#include "acyclebuffer.h"

bool Thread::thread_exit;
bool Thread::thread_pause;
ACycleBuffer Thread::out_buffer_audio(MAX_AUDIO_FRAME_SIZE * 10);

int Thread::sfp_refresh_thread(void * opaque)
{
	thread_exit = false;

	while (!thread_exit) {
		SDL_Event event;
		event.type = SFM_REFRESH_EVENT;
		SDL_PushEvent(&event);
		SDL_Delay(1000 / fps);
	}

	return 0;
}

void Thread::fill_audio(void * udata, Uint8 * stream, int len)
{
	SDL_memset(stream, 0, len);
	int n = out_buffer_audio.read((char*)copy_buf, len);
	if (isMute) {
		SDL_MixAudio(stream, copy_buf, n, 0);
	}
	else {
		SDL_MixAudio(stream, copy_buf, n, SDL_MIX_MAXVOLUME - silence);
	}
}