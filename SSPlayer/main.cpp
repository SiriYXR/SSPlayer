//===============================
//�������ƣ�SSPlayer
//�汾�ţ�Demo
//�����ˣ�������
//
//����ʱ�䣺2017-07-20   21:19:03
//�깤ʱ�䣺
//��������589��
//�������ڣ�4 ��
//
//���һ���޸�ʱ�䣺2017-08-03   17:56:53
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