#include "atari.h"
#include "config.h"
#include "input.h"
#include "monitor.h"

void Atari_Initialise(int *argc, char *argv[])
{
}

int Atari_Exit(int run_monitor)
{
	if (run_monitor)
		return monitor();
	return FALSE;
}

int Atari_Keyboard(void)
{
	return AKEY_NONE;
}

int Atari_PORT(int num)
{
	return 0xff;
}

int Atari_TRIG(int num)
{
	return 1;
}

int Atari_POT(int num)
{
	return 228;
}

int Atari_CONSOL(void)
{
	return 7;
}

int Atari_PEN(int vertical)
{
	return vertical ? 0xff : 0;
}

int main(int argc, char **argv)
{
	/* initialise Atari800 core */
	if (!Atari800_Initialise(&argc, argv))
		return 3;

	/* main loop */
	while (TRUE) {
		Atari800_Frame(EMULATE_BASIC);
#ifndef	DONT_SYNC_WITH_HOST
		atari_sync();
#endif
	}
}
