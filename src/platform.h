/* $Id: platform.h,v 1.2 2001/03/18 06:34:58 knik Exp $ */
#ifndef __PLATFORM__
#define __PLATFORM__

#include "atari.h"

/*
 * This include file defines prototypes for platforms specific functions.
 */

void Atari_Initialise(int *argc, char *argv[]);
int Atari_Exit(int run_monitor);
int Atari_Keyboard(void);
void Atari_DisplayScreen (UBYTE *screen);

int Atari_PORT(int num);
int Atari_TRIG(int num);
int Atari_POT(int num);
int Atari_CONSOL(void);
int Atari_PEN(int vertical);
#if defined(SET_LED) && defined(NO_LED_ON_SCREEN)
void Atari_Set_LED(int how);
#endif

#endif

/*
$Log: platform.h,v $
Revision 1.2  2001/03/18 06:34:58  knik
WIN32 conditionals removed

*/
