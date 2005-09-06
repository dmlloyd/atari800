/*
 * rtime.c - Emulate ICD R-Time 8 cartridge
 *
 * Copyright (C) 2000 Jason Duerstock
 * Copyright (C) 2000-2005 Atari800 development team (see DOC/CREDITS)
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
#include <stdlib.h>	/* for NULL */
#include <string.h>	/* for strcmp() */
#ifdef HAVE_TIME_H
#include <time.h>
#endif

#include "atari.h"
#include "log.h"
#include "util.h"

int rtime_enabled = 1;

static int rtime_state = 0;
				/* 0 = waiting for register # */
				/* 1 = got register #, waiting for hi nybble */
				/* 2 = got hi nybble, waiting for lo nybble */
static int rtime_tmp = 0;
static int rtime_tmp2 = 0;

static UBYTE regset[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

void RTIME_Initialise(int *argc, char *argv[])
{
	int i;
	int j;
	for (i = j = 1; i < *argc; i++) {
		if (strcmp(argv[i], "-rtime") == 0)
			rtime_enabled = Util_sscanbool(argv[++i]);
		else
			argv[j++] = argv[i];
	}
	*argc = j;
}

#if defined(HAVE_TIME) && defined(HAVE_LOCALTIME)

static int hex2bcd(int h)
{
	return ((h / 10) << 4) | (h % 10);
}

static int gettime(int p)
{
	time_t tt;
	struct tm *lt;

	tt = time(NULL);
	lt = localtime(&tt);

	switch (p) {
	case 5:
		return hex2bcd(lt->tm_year);
	case 4:
		return hex2bcd(lt->tm_mon + 1);
	case 3:
		return hex2bcd(lt->tm_mday);
	case 2:
		return hex2bcd(lt->tm_hour);
	case 1:
		return hex2bcd(lt->tm_min);
	case 0:
		return hex2bcd(lt->tm_sec);
	case 6:
		return hex2bcd(((lt->tm_wday + 2) % 7) + 1);
	}
	return 0;
}

#endif /* defined(HAVE_TIME) && defined(HAVE_LOCALTIME) */

UBYTE RTIME_GetByte(void)
{
	switch (rtime_state) {
	case 0:
		/* Aprint("pretending rtime not busy, returning 0"); */
		return 0;
	case 1:
		rtime_state = 2;
		return (
#if defined(HAVE_TIME) && defined(HAVE_LOCALTIME)
			rtime_tmp <= 6 ?
			gettime(rtime_tmp) :
#endif
			regset[rtime_tmp]) >> 4;
	case 2:
		rtime_state = 0;
		return (
#if defined(HAVE_TIME) && defined(HAVE_LOCALTIME)
			rtime_tmp <= 6 ?
			gettime(rtime_tmp) :
#endif
			regset[rtime_tmp]) & 0x0f;
	}
	return 0;
}

void RTIME_PutByte(UBYTE byte)
{
	switch (rtime_state) {
	case 0:
		rtime_tmp = byte & 0x0f;
		rtime_state = 1;
		break;
	case 1:
		rtime_tmp2 = byte << 4;
		rtime_state = 2;
		break;
	case 2:
		regset[rtime_tmp] = rtime_tmp2 | (byte & 0x0f);
		rtime_state = 0;
		break;
	}
}
