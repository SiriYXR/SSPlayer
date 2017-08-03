#pragma once

#include "function.h"

class ACycleBuffer;

class Thread
{
public:
	static int sfp_refresh_thread(void *opaque);

	static void fill_audio(void * udata, Uint8 * stream, int len);

	static bool thread_exit;
	static bool thread_pause;
	static ACycleBuffer out_buffer_audio;
};

