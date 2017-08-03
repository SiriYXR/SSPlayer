#include "function.h"

bool thread_exit = false;
bool thread_pause = false;
bool isMute = false;

double fps = 25;
int silence = SDL_MIX_MAXVOLUME / 2;

uint8_t *copy_buf;