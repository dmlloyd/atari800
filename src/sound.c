/*
 * sound.c - high-level sound routines
 *
 * Copyright (C) 1995-1998 David Firth
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

#ifdef SOUND

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>

#include "pokeysnd.h"
#include "log.h"
#include "sndsave.h"

#define FRAGSIZE	7

#define FALSE 0
#define TRUE 1

#define DEFDSPRATE 22050

static char *dspname = "/dev/dsp";
static int dsprate = DEFDSPRATE;
static int fragstofill = 0;
static int snddelay = 60;		/* delay in milliseconds */

static int sound_enabled = TRUE;
static int dsp_fd;

void Sound_Initialise(int *argc, char *argv[])
{
	int i, j;
	struct audio_buf_info abi;
	int help_only = FALSE;

	for (i = j = 1; i < *argc; i++) {
		if (strcmp(argv[i], "-sound") == 0)
			sound_enabled = TRUE;
		else if (strcmp(argv[i], "-nosound") == 0)
			sound_enabled = FALSE;
		else if (strcmp(argv[i], "-dsprate") == 0)
			sscanf(argv[++i], "%d", &dsprate);
		else if (strcmp(argv[i], "-snddelay") == 0)
			sscanf(argv[++i], "%d", &snddelay);
		else {
			if (strcmp(argv[i], "-help") == 0) {
				help_only = TRUE;
				Aprint("\t-sound           Enable sound\n"
				       "\t-nosound         Disable sound\n"
				       "\t-dsprate <rate>  Set DSP rate in Hz\n"
				       "\t-snddelay <ms>   Set sound delay in milliseconds"
				      );
			}
			argv[j++] = argv[i];
		}
	}
	*argc = j;

	if (help_only)
		return;

	if (sound_enabled) {
		if ((dsp_fd = open(dspname, O_WRONLY | O_NONBLOCK)) == -1) {
			perror(dspname);
			sound_enabled = 0;
			return;
		}

		if (ioctl(dsp_fd, SNDCTL_DSP_SPEED, &dsprate)) {
			Aprint("%s: cannot set %d speed\n", dspname, dsprate);
			close(dsp_fd);
			sound_enabled = 0;
			return;
		}

		i = AFMT_U8;
		if (ioctl(dsp_fd, SNDCTL_DSP_SETFMT, &i)) {
			Aprint("%s: cannot set 8-bit sample\n", dspname);
			close(dsp_fd);
			sound_enabled = 0;
			return;
		}
#ifdef STEREO_SOUND
		i = 1;
		if (ioctl(dsp_fd, SNDCTL_DSP_STEREO, &i)) {
			Aprint("%s: cannot set stereo\n", dspname);
			close(dsp_fd);
			sound_enabled = 0;
			return;
		}
#endif

		fragstofill = ((dsprate * snddelay / 1000) >> FRAGSIZE) + 1;
		if (fragstofill > 100)
			fragstofill = 100;

		/* fragments of size 2^FRAGSIZE bytes */
		i = ((fragstofill + 1) << 16) | FRAGSIZE;
		if (ioctl(dsp_fd, SNDCTL_DSP_SETFRAGMENT, &i)) {
			Aprint("%s: cannot set fragments\n", dspname);
			close(dsp_fd);
			sound_enabled = 0;
			return;
		}

		if (ioctl(dsp_fd, SNDCTL_DSP_GETOSPACE, &abi)) {
			Aprint("%s: unable to get output space\n", dspname);
			close(dsp_fd);
			sound_enabled = 0;
			return;
		}

#ifdef STEREO_SOUND
		Pokey_sound_init(FREQ_17_EXACT, dsprate, 2, 0);
#else
		Pokey_sound_init(FREQ_17_EXACT, dsprate, 1, 0);
#endif
	}
}

void Sound_Pause(void)
{
	if (sound_enabled) {
		/* stop audio output */
	}
}

void Sound_Continue(void)
{
	if (sound_enabled) {
		/* start audio output */
	}
}

void Sound_Exit(void)
{
	if (sound_enabled)
		close(dsp_fd);
}

void Sound_Update(void)
{
	int i;
	struct audio_buf_info abi;
	char dsp_buffer[1 << FRAGSIZE];

	if (!sound_enabled)
		return;

	if (ioctl(dsp_fd, SNDCTL_DSP_GETOSPACE, &abi))
		return;
		
	i = (abi.fragstotal * abi.fragsize - abi.bytes) >> FRAGSIZE;

	/* we need fragstofill fragments to be filled */
#ifdef STEREO_SOUND
	for (; i < fragstofill * 2; i++) {
#else
	for (; i < fragstofill; i++) {
#endif
		Pokey_process(dsp_buffer, sizeof(dsp_buffer));
		write(dsp_fd, dsp_buffer, sizeof(dsp_buffer));
	}
}
#endif	/* SOUND */

/*
 $Log: sound.c,v $
 Revision 1.15  2005/08/16 23:07:28  pfusik
 #include "config.h" before system headers

 Revision 1.14  2005/03/24 18:09:49  pfusik
 removed extra \n from the output of -help

 Revision 1.13  2003/02/24 09:33:10  joy
 header cleanup

 Revision 1.12  2003/02/19 14:07:48  joy
 configure stuff cleanup

 Revision 1.11  2003/02/09 21:24:12  joy
 updated for different number of Pokey_sound_init parameters

 Revision 1.10  2003/01/27 13:14:53  joy
 Jason's changes: either PAGED_ATTRIB support (mostly), or just clean up.

 Revision 1.9  2002/12/08 20:33:27  knik
 new arguments in Pokey_sound_init call

 Revision 1.8  2002/08/07 08:43:58  joy
 ALL printf->Aprint, in help_only doesn't initialize the sound

 Revision 1.7  2002/08/07 06:16:50  joy
 printf->Aprint

 Revision 1.6  2001/10/10 16:15:03  knik
 improved output space calculation--can work much better with some drivers

 Revision 1.5  2001/06/18 12:08:51  joy
 non-blocking open of sound device.

 Revision 1.4  2001/04/08 05:59:32  knik
 sound_pause/continue removed if no sound used

 Revision 1.3  2001/03/24 06:28:07  knik
 help fixed and control message removed

 */
