/*
 * atari_svgalib.c - SVGALIB library specific port code
 *
 * Copyright (c) 1995-1998 David Firth
 * Copyright (C) 1998-2005 Atari800 development team (see DOC/CREDITS)
 *
 * This file is part of the Atari800 emulator project which emulates
 * the Atari 400, 800, 800XL, 130XE, and 5200 8-bit computers.
 *
 * Atari800 is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Atari800 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Atari800; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "config.h"
#include <stdio.h>
#include <stdlib.h> /* exit() */
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include <vga.h>

#include "atari.h"
#include "input.h"
#include "colours.h"
#include "monitor.h"
#include "sound.h"
#include "platform.h"
#include "log.h"
#include "ui.h"
#include "screen.h"

#ifdef SVGA_JOYMOUSE
#include <vgamouse.h>

#define CENTER_X 16384
#define CENTER_Y 16384
#define THRESHOLD 15
#endif

#ifdef LINUX_JOYSTICK
#include <errno.h>
#include <linux/joystick.h>

static int js0;
static int js1;

static int js0_centre_x;
static int js0_centre_y;
#if 0 /* currently not used */
static int js1_centre_x;
static int js1_centre_y;
#endif

static struct JS_DATA_TYPE js_data;
#endif /* LINUX_JOYSTICK */

#include <linux/keyboard.h>
#include <vgakeyboard.h>

extern int alt_function;
static int pause_hit = 0;
static UBYTE kbhits[NR_KEYS];
static int kbcode = 0;
static int kbjoy = 1;
static UBYTE joydefs[] =
{
	SCANCODE_KEYPAD0,	/* fire */
	SCANCODE_KEYPAD7,	/* up/left */
	SCANCODE_KEYPAD8,	/* up */
	SCANCODE_KEYPAD9,	/* up/right */
	SCANCODE_KEYPAD4,	/* left */
	SCANCODE_KEYPAD6,	/* right */
	SCANCODE_KEYPAD1,	/* down/left */
	SCANCODE_KEYPAD2,	/* down */
	SCANCODE_KEYPAD3,	/* down/right */
#ifdef USE_CURSORBLOCK
	SCANCODE_CURSORBLOCKUP,
	SCANCODE_CURSORBLOCKLEFT,
	SCANCODE_CURSORBLOCKRIGHT,
	SCANCODE_CURSORBLOCKDOWN,
#endif
};

static UBYTE joymask[] =
{
	0,       /* not used */
	~1 & ~4, /* up/left */
	~1,      /* up */
	~1 & ~8, /* up/right */
	~4,      /* left */
	~8,      /* right */
	~2 & ~4, /* down/left */
	~2,      /* down */
	~2 & ~8, /* down/right */
#ifdef USE_CURSORBLOCK
	~1,      /* up */
	~4,      /* left */
	~8,      /* right */
	~2,      /* down */
#endif
};

#ifdef SVGA_JOYMOUSE
static int vgamouse_stick;
static int vgamouse_strig;
#endif

/* cursor keys joystick */
static int ctrig;
static int cstick;
/* analog joystick */
static int atrig;
static int astick;

static int invisible = 0;

/*
   Interlace variables
 */

static int ypos_inc = 1;
static int svga_ptr_inc = 320;
static int scrn_ptr_inc = ATARI_WIDTH;


/****************************************************************************/
/* Linux raw keyboard handler                                               */
/* by Krzysztof Nikiel, 1999                                                */
/****************************************************************************/
static void kbhandler(int code, int state)
{
	if (code < 0 || code >= NR_KEYS) {
		fprintf(stderr, "keyboard code out of range (0x%x).\n", code);
		return;
	}
#if 0
	printf("code: %x(%x) - %s\n", code, state, keynames[code]);
#endif
	if (code == SCANCODE_BREAK
	 || code == SCANCODE_BREAK_ALTERNATIVE) {
		if (state)
			pause_hit = 1;
		return;
	}
	kbhits[kbcode = code] = state;
	if (!state)
		kbcode |= 0x100;
}

static void initkb(void)
{
	int i;
	if (keyboard_init()) {
		fprintf(stderr,"unable to initialize keyboard\n");
		exit(1);
	}
	for (i = 0; i < sizeof(kbhits) / sizeof(kbhits[0]); i++)
		kbhits[i] = 0;
	keyboard_seteventhandler(kbhandler);
}

int Atari_Keyboard(void)
{
	int keycode;
	int i;

	keyboard_update();

	if (kbjoy) {
		/* fire */
#ifdef USE_CURSORBLOCK
		ctrig = (kbhits[joydefs[0]] ? 0 : 1) & (kbhits[SCANCODE_LEFTCONTROL] ? 0 : 1);
#else
		ctrig = kbhits[joydefs[0]] ? 0 : 1;
#endif
		cstick |= 0xf;
		for (i = 1; i < sizeof(joydefs) / sizeof(joydefs[0]); i++)
			if (kbhits[joydefs[i]])
				cstick &= joymask[i];
	}

	key_shift = (kbhits[SCANCODE_LEFTSHIFT]
	           | kbhits[SCANCODE_RIGHTSHIFT]) ? 1 : 0;

	alt_function = -1;		/* no alt function */
	if (kbhits[0x38]) {		/* left Alt key is pressed */
		if (kbcode == SCANCODE_R)
			alt_function = MENU_RUN;		/* ALT+R .. Run file */
		else if (kbcode == SCANCODE_Y)
			alt_function = MENU_SYSTEM;		/* ALT+Y .. Select system */
		else if (kbcode == SCANCODE_O)
			alt_function = MENU_SOUND;		/* ALT+O .. mono/stereo sound */
		else if (kbcode == SCANCODE_W)
			alt_function = MENU_SOUND_RECORDING;	/* ALT+W .. record sound */
		else if (kbcode == SCANCODE_A)
			alt_function = MENU_ABOUT;		/* ALT+A .. About */
		else if (kbcode == SCANCODE_S)
			alt_function = MENU_SAVESTATE;	/* ALT+S .. Save state */
		else if (kbcode == SCANCODE_D)
			alt_function = MENU_DISK;		/* ALT+D .. Disk management */
		else if (kbcode == SCANCODE_L)
			alt_function = MENU_LOADSTATE;	/* ALT+L .. Load state */
		else if (kbcode == SCANCODE_C)
			alt_function = MENU_CARTRIDGE;	/* ALT+C .. Cartridge management */
	}
	if (alt_function != -1)
		return AKEY_UI;

	/* need to set shift mask here to avoid conflict with PC layout */
	keycode = (key_shift ? 0x40 : 0)
	        | ((kbhits[SCANCODE_LEFTCONTROL]
	          | kbhits[SCANCODE_RIGHTCONTROL]) ? 0x80 : 0);

	switch (kbcode) {
	case SCANCODE_F9:
		return AKEY_EXIT;
	case SCANCODE_F8:
		kbcode = 0;
		return Atari_Exit(1) ? AKEY_NONE : AKEY_EXIT;
	case SCANCODE_SCROLLLOCK: /* pause */
		while (kbhits[SCANCODE_SCROLLLOCK])
			keyboard_update();
		while (!kbhits[SCANCODE_SCROLLLOCK])
			keyboard_update();
		kbcode = 0;
		break;
	case SCANCODE_PRINTSCREEN:
	case SCANCODE_F10:
		kbcode = 0;
		return key_shift ? AKEY_SCREENSHOT_INTERLACE : AKEY_SCREENSHOT;
	case SCANCODE_F1:
		return AKEY_UI;
	case SCANCODE_F5:
		return key_shift ? AKEY_COLDSTART : AKEY_WARMSTART;
	case SCANCODE_F11:
		for (i = 0; i < 4; i++) {
			if (++joy_autofire[i] > 2)
				joy_autofire[i] = 0;
		}
		kbcode = 0;
		break;
	default:
		break;
	}

	if (machine_type == MACHINE_5200 && !ui_is_active) {
		switch (kbcode) {
		case SCANCODE_F4:
			keycode |= AKEY_5200_START;
			break;
		case SCANCODE_P:
			keycode |= AKEY_5200_PAUSE;
			break;
		case SCANCODE_R:
			keycode |= AKEY_5200_RESET;
			break;
		case SCANCODE_0:
			keycode |= AKEY_5200_0;
			break;
		case SCANCODE_1:
			keycode |= AKEY_5200_1;
			break;
		case SCANCODE_2:
			keycode |= AKEY_5200_2;
			break;
		case SCANCODE_3:
			keycode = key_shift ? AKEY_5200_HASH : AKEY_5200_3;
			break;
		case SCANCODE_4:
			keycode |= AKEY_5200_4;
			break;
		case SCANCODE_5:
			keycode |= AKEY_5200_5;
			break;
		case SCANCODE_6:
			keycode |= AKEY_5200_6;
			break;
		case SCANCODE_7:
			keycode |= AKEY_5200_7;
			break;
		case SCANCODE_8:
			keycode = key_shift ? AKEY_5200_ASTERISK : AKEY_5200_8;
			break;
		case SCANCODE_9:
			keycode |= AKEY_5200_9;
			break;
		case SCANCODE_EQUAL:
			keycode = AKEY_5200_HASH;
			break;
		default:
			keycode = AKEY_NONE;
			break;
		}
		return keycode;
	}

	key_consol = (kbhits[SCANCODE_F2] ? 0 : CONSOL_OPTION)
	           | (kbhits[SCANCODE_F3] ? 0 : CONSOL_SELECT)
	           | (kbhits[SCANCODE_F4] ? 0 : CONSOL_START);

	if (pause_hit) {
		pause_hit = 0;
		return AKEY_BREAK;
	}

	switch (kbcode) {
	case SCANCODE_F6:
		return AKEY_HELP;
	case SCANCODE_F7:
		return AKEY_BREAK;
#define KBSCAN(name) \
		case SCANCODE_##name: \
			keycode |= (AKEY_##name & ~(AKEY_CTRL | AKEY_SHFT)); \
			break;
	KBSCAN(ESCAPE)
	KBSCAN(1)
	case SCANCODE_2:
		if (key_shift)
			keycode = AKEY_AT;
		else
			keycode |= AKEY_2;
		break;
	KBSCAN(3)
	KBSCAN(4)
	KBSCAN(5)
	case SCANCODE_6:
		if (key_shift)
			keycode = AKEY_CARET;
		else
			keycode |= AKEY_6;
		break;
	case SCANCODE_7:
		if (key_shift)
			keycode = AKEY_AMPERSAND;
		else
			keycode |= AKEY_7;
		break;
	case SCANCODE_8:
		if (key_shift)
			keycode = AKEY_ASTERISK;
		else
			keycode |= AKEY_8;
		break;
	KBSCAN(9)
	KBSCAN(0)
	KBSCAN(MINUS)
	case SCANCODE_EQUAL:
		if (key_shift)
			keycode = AKEY_PLUS;
		else
			keycode |= AKEY_EQUAL;
		break;
	KBSCAN(BACKSPACE)
	KBSCAN(TAB)
	KBSCAN(Q)
	KBSCAN(W)
	KBSCAN(E)
	KBSCAN(R)
	KBSCAN(T)
	KBSCAN(Y)
	KBSCAN(U)
	KBSCAN(I)
	KBSCAN(O)
	KBSCAN(P)
	case SCANCODE_BRACKET_LEFT:
		keycode |= AKEY_BRACKETLEFT;
		break;
	case SCANCODE_BRACKET_RIGHT:
		keycode |= AKEY_BRACKETRIGHT;
		break;
	case SCANCODE_ENTER:
		keycode |= AKEY_RETURN;
		break;
	KBSCAN(A)
	KBSCAN(S)
	KBSCAN(D)
	KBSCAN(F)
	KBSCAN(G)
	KBSCAN(H)
	KBSCAN(J)
	KBSCAN(K)
	KBSCAN(L)
	KBSCAN(SEMICOLON)
	case SCANCODE_APOSTROPHE:
		if (key_shift)
			keycode = AKEY_DBLQUOTE;
		else
			keycode |= AKEY_QUOTE;
		break;
	case SCANCODE_GRAVE:
		keycode |= AKEY_ATARI;
		break;
	case SCANCODE_BACKSLASH:
		if (key_shift)
			keycode = AKEY_BAR;
		else
			keycode |= AKEY_BACKSLASH;
		break;
	KBSCAN(Z)
	KBSCAN(X)
	KBSCAN(C)
	KBSCAN(V)
	KBSCAN(B)
	KBSCAN(N)
	KBSCAN(M)
	case SCANCODE_COMMA:
		if (key_shift)
			keycode = AKEY_LESS;
		else
			keycode |= AKEY_COMMA;
		break;
	case SCANCODE_PERIOD:
		if (key_shift)
			keycode = AKEY_GREATER;
		else
			keycode |= AKEY_FULLSTOP;
		break;
	KBSCAN(SLASH)
	KBSCAN(SPACE)
	KBSCAN(CAPSLOCK)
	case SCANCODE_CURSORBLOCKUP:
		keycode = AKEY_UP;
		break;
	case SCANCODE_CURSORBLOCKDOWN:
		keycode = AKEY_DOWN;
		break;
	case SCANCODE_CURSORBLOCKLEFT:
		keycode = AKEY_LEFT;
		break;
	case SCANCODE_CURSORBLOCKRIGHT:
		keycode = AKEY_RIGHT;
		break;
	case SCANCODE_REMOVE:
		if (key_shift)
			keycode = AKEY_DELETE_LINE;
		else
			keycode |= AKEY_DELETE_CHAR;
		break;
	case SCANCODE_INSERT:
		if (key_shift)
			keycode = AKEY_INSERT_LINE;
		else
			keycode |= AKEY_INSERT_CHAR;
		break;
	case SCANCODE_HOME:
		keycode = AKEY_CLEAR;
		break;
	case SCANCODE_END:
		keycode = AKEY_HELP;
		break;
	default:
		keycode = AKEY_NONE;
	}

	return keycode;
}
/****************************************************************************/
/* end of keyboard handler                                                  */
/****************************************************************************/

void invisible_start(void)
{
	invisible = 1;
}

void invisible_stop(void)
{
	invisible = 0;
}

void Atari_Initialise(int *argc, char *argv[])
{
	int VGAMODE = G320x240x256;

	int i;
	int j;

#ifdef SOUND
	Sound_Initialise(argc, argv);
#endif

	for (i = j = 1; i < *argc; i++) {
		if (strcmp(argv[i], "-interlace") == 0) {
			ypos_inc = 2;
			svga_ptr_inc = 320 + 320;
			scrn_ptr_inc = ATARI_WIDTH + ATARI_WIDTH;
		}
		else {
			if (strcmp(argv[i], "-help") == 0) {
				Aprint("\t-interlace       Generate screen with interlace");
			}
			argv[j++] = argv[i];
		}
	}

	*argc = j;

#ifdef SVGA_JOYMOUSE
	if (mouse_init("/dev/mouse", vga_getmousetype(), MOUSE_DEFAULTSAMPLERATE) == -1) {
		perror("/dev/mouse");
		exit(1);
	}
	mouse_setposition(CENTER_X, CENTER_Y);
#endif

#ifdef LINUX_JOYSTICK
	js0 = open("/dev/js0", O_RDONLY, 0777);
	if (js0 != -1) {
		int status;

		status = read(js0, &js_data, JS_RETURN);
		if (status != JS_RETURN) {
			perror("/dev/js0");
			exit(1);
		}
		js0_centre_x = js_data.x;
		js0_centre_y = js_data.y;
	}
	else {
		printf("cannot open joystick /dev/js0: %s\n"
		       "joystick disabled\n", strerror(errno));
	}
#if 0 /* currently not used */
	js1 = open("/dev/js1", O_RDONLY, 0777);
	if (js1 != -1) {
		int status;

		status = read(js1, &js_data, JS_RETURN);
		if (status != JS_RETURN) {
			perror("/dev/js1");
			exit(1);
		}
		js1_centre_x = js_data.x;
		js1_centre_y = js_data.y;
	}
#endif
#endif

	vga_init();

	if (!vga_hasmode(VGAMODE)) {
		fprintf(stderr,"Mode not available\n");
		exit(1);
	}
	vga_setmode(VGAMODE);

	if (vga_runinbackground_version() >= 1) {
		vga_runinbackground(1);
		vga_runinbackground(VGA_GOTOBACK,invisible_start);
		vga_runinbackground(VGA_COMEFROMBACK,invisible_stop);
	}

	initkb();

	for (i = 0; i < 256; i++) {
		int rgb = colortable[i];
		int red;
		int green;
		int blue;

		red = (rgb & 0x00ff0000) >> 18;
		green = (rgb & 0x0000ff00) >> 10;
		blue = (rgb & 0x000000ff) >> 2;

		vga_setpalette(i, red, green, blue);
	}

	ctrig = 1;
	cstick = 15;
	atrig = 1;
	astick = 15;
	key_consol = CONSOL_NONE;
}

int Atari_Exit(int run_monitor)
{
	int restart;

	keyboard_close();
	vga_setmode(TEXT);

	if (run_monitor)
		restart = monitor();
	else
		restart = FALSE;

	if (restart) {
		int VGAMODE = G320x240x256;
		int i;

		if (!vga_hasmode(VGAMODE)) {
			fprintf(stderr,"Mode not available\n");
			exit(1);
		}
		vga_setmode(VGAMODE);

		initkb();

		for (i = 0; i < 256; i++) {
			int rgb = colortable[i];
			int red;
			int green;
			int blue;

			red = (rgb & 0x00ff0000) >> 18;
			green = (rgb & 0x0000ff00) >> 10;
			blue = (rgb & 0x000000ff) >> 2;

			vga_setpalette(i, red, green, blue);
		}
	}
	else {
#ifdef SVGA_JOYMOUSE
		mouse_close();
#endif

#ifdef LINUX_JOYSTICK
		if (js0 != -1)
			close(js0);

		if (js1 != -1)
			close(js1);
#endif

#ifdef SOUND
		Sound_Exit();
#endif

		Aflushlog();
	}

	return restart;
}

void Atari_DisplayScreen(void)
{
	UBYTE *vbuf = (UBYTE *) atari_screen + 32;

	if (invisible)
		return;

#ifdef SVGA_SPEEDUP
	if (!ui_is_active) {
		static int writestrip = 0;
		int y1;
		int y2;
		if (writestrip >= refresh_rate)
			writestrip = 0;
		y1 = 240 * writestrip / refresh_rate;
		y2 = 240 * (writestrip + 1) / refresh_rate;
		vga_copytoplanar256(vbuf + ATARI_WIDTH * y1, ATARI_WIDTH,
		                    (320 >> 2) * y1,
		                    320 >> 2, 320, y2 - y1);
		vga_setdisplaystart(0);
		writestrip++;
	}
	else
#endif
	{
		static int writepage = 0;
		vga_copytoplanar256(vbuf, ATARI_WIDTH,
		                   ((320 * 240) >> 2) * writepage,
		                   320 >> 2, 320, 240);
		vga_setdisplaystart(320 * 240 * writepage);
		vga_setpage(0);
		writepage ^= 1;
	}

#ifdef SVGA_JOYMOUSE
	vgamouse_stick = 0xff;

	if (mouse_update() != 0) {
		int x;
		int y;

		x = mouse_getx();
		y = mouse_gety();

		if (x < (CENTER_X - THRESHOLD))
			vgamouse_stick &= 0xfb;
		else if (x > (CENTER_X + THRESHOLD))
			vgamouse_stick &= 0xf7;

		if (y < (CENTER_Y - THRESHOLD))
			vgamouse_stick &= 0xfe;
		else if (y > (CENTER_Y + THRESHOLD))
			vgamouse_stick &= 0xfd;

		mouse_setposition(CENTER_X, CENTER_Y);
	}
	if (mouse_getbutton())
		vgamouse_strig = 0;
	else
		vgamouse_strig = 1;
#endif
}

#ifdef LINUX_JOYSTICK
void read_joystick(int js, int centre_x, int centre_y)
{
	const int threshold = 50;
	int status;

	astick = 0x0f;

	status = read(js, &js_data, JS_RETURN);
	if (status != JS_RETURN) {
		perror("/dev/js");
		exit(1);
	}
	if (js_data.x < (centre_x - threshold))
		astick &= 0xfb;
	if (js_data.x > (centre_x + threshold))
		astick &= 0xf7;
	if (js_data.y < (centre_y - threshold))
		astick &= 0xfe;
	if (js_data.y > (centre_y + threshold))
		astick &= 0xfd;
}
#endif


int Atari_PORT(int num)
{
	if (num == 0) {
/* sorry, no time for this now

#ifdef SVGA_JOYMOUSE
		stick0 = vgamouse_stick;
#endif
*/

#ifdef LINUX_JOYSTICK
		if (js0 != -1)
			read_joystick(js0, js0_centre_x, js0_centre_y);
#endif
		return (astick << 4) | cstick;
	}
	return 0xff;
}


int Atari_TRIG(int num)
{
	if (num == 0)
		return ctrig;
#ifdef LINUX_JOYSTICK
	if (num == 1) {

/* sorry, no time for this now
#ifdef SVGA_JOYMOUSE
		trig0 = vgamouse_strig;
#endif
*/

		if (js0 != -1) {
			int status;
			status = read(js0, &js_data, JS_RETURN);
			if (status != JS_RETURN) {
				perror("/dev/js0");
				exit(1);
			}
			if (js_data.buttons & 0x01)
				atrig = 0;
			else
				atrig = 1;
		}
/*
		if (js_data.buttons & 0x02)
			special_keycode = ' ';
*/
/*
		trig0 = (js_data.buttons & 0x0f) ? 0 : 1;
 */
		return atrig;
	}
#endif
	return 1;
}

int main(int argc, char **argv)
{
	/* initialise Atari800 core */
	if (!Atari800_Initialise(&argc, argv))
		return 3;

	/* main loop */
	for (;;) {
		key_code = Atari_Keyboard();
		Atari800_Frame();
#ifndef SVGA_SPEEDUP
		if (display_screen)
#endif
			Atari_DisplayScreen();
	}
}
