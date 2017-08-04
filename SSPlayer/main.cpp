//===============================
//程序名称：SSPlayer
//版本号：Demo
//制作人：杨新瑞
//
//创建时间：2017-07-20   21:19:03
//完工时间：
//代码量：589行
//制作周期：4 天
//
//最近一次修改时间：2017-08-03   17:56:53
//
//===============================

//#include "Player.h"
#include "GUI.h"

int main(int argc, char **argv)
{
	/*Player ssplayer;
	SDL_CreateThread(Thread::sfp_refresh_thread, nullptr, nullptr);
	for (; ssplayer.running();SDL_Delay(1)) {
		ssplayer.events();

		ssplayer.update();

		ssplayer.render();
	}*/

	GUI SSPlayer;

	SSPlayer.Running();

	return 0;
}