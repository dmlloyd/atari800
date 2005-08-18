/*
 * ui_basic.c - main user interface
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
#include <stdio.h>
#include <string.h>
#include <stdlib.h> /* free() */

#ifdef HAVE_UNISTD_H
#include <unistd.h> /* getcwd() */
#endif

#if defined(HAVE_DIRENT_H) && defined(HAVE_OPENDIR)
/* XXX: <sys/dir.h>, <ndir.h>, <sys/ndir.h> */
#define DO_DIR
#endif

#ifdef DO_DIR
#include <dirent.h>
#endif

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#include "rt-config.h"
#include "atari.h"
#include "antic.h"
#include "list.h"
#include "ui.h"
#include "log.h"
#include "input.h"
#include "prompts.h"
#include "platform.h"
#include "memory.h"
#include "screen.h" /* for atari_screen */

#ifdef USE_CURSES
extern UBYTE curses_screen[24][40];
void curses_clear_screen(void);
#endif

extern int current_disk_directory;

static int initialised = FALSE;
static UBYTE charset[1024];

/* Basic UI driver calls */

int BasicUISelect(char *pTitle, int bFloat, int nDefault, tMenuItem *menu, int *ascii);
int BasicUIGetSaveFilename(char *pFilename);
int BasicUIGetLoadFilename(char *pDirectory, char *pFilename);
void BasicUIMessage(char *pMessage);
void BasicUIAboutBox(void);
void BasicUIInit(void);

tUIDriver basic_ui_driver =
{
	&BasicUISelect,
	&BasicUIGetSaveFilename,
	&BasicUIGetLoadFilename,
	&BasicUIMessage,
	&BasicUIAboutBox,
	&BasicUIInit
};


unsigned char key_to_ascii[256] =
{
	0x6C, 0x6A, 0x3B, 0x00, 0x00, 0x6B, 0x2B, 0x2A, 0x6F, 0x00, 0x70, 0x75, 0x9B, 0x69, 0x2D, 0x3D,
	0x76, 0x00, 0x63, 0x00, 0x00, 0x62, 0x78, 0x7A, 0x34, 0x00, 0x33, 0x36, 0x1B, 0x35, 0x32, 0x31,
	0x2C, 0x20, 0x2E, 0x6E, 0x00, 0x6D, 0x2F, 0x00, 0x72, 0x00, 0x65, 0x79, 0x7F, 0x74, 0x77, 0x71,
	0x39, 0x00, 0x30, 0x37, 0x7E, 0x38, 0x3C, 0x3E, 0x66, 0x68, 0x64, 0x00, 0x00, 0x67, 0x73, 0x61,

	0x4C, 0x4A, 0x3A, 0x00, 0x00, 0x4B, 0x5C, 0x5E, 0x4F, 0x00, 0x50, 0x55, 0x9B, 0x49, 0x5F, 0x7C,
	0x56, 0x00, 0x43, 0x00, 0x00, 0x42, 0x58, 0x5A, 0x24, 0x00, 0x23, 0x26, 0x1B, 0x25, 0x22, 0x21,
	0x5B, 0x20, 0x5D, 0x4E, 0x00, 0x4D, 0x3F, 0x00, 0x52, 0x00, 0x45, 0x59, 0x9F, 0x54, 0x57, 0x51,
	0x28, 0x00, 0x29, 0x27, 0x9C, 0x40, 0x7D, 0x9D, 0x46, 0x48, 0x44, 0x00, 0x00, 0x47, 0x53, 0x41,

	0x0C, 0x0A, 0x7B, 0x00, 0x00, 0x0B, 0x1E, 0x1F, 0x0F, 0x00, 0x10, 0x15, 0x9B, 0x09, 0x1C, 0x1D,
	0x16, 0x00, 0x03, 0x00, 0x00, 0x02, 0x18, 0x1A, 0x00, 0x00, 0x9B, 0x00, 0x1B, 0x00, 0xFD, 0x00,
	0x00, 0x20, 0x60, 0x0E, 0x00, 0x0D, 0x00, 0x00, 0x12, 0x00, 0x05, 0x19, 0x9E, 0x14, 0x17, 0x11,
	0x00, 0x00, 0x00, 0x00, 0xFE, 0x00, 0x7D, 0xFF, 0x06, 0x08, 0x04, 0x00, 0x00, 0x07, 0x13, 0x01,

	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

unsigned char ascii_to_screen[128] =
{
	0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
	0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
	0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,
	0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
	0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
	0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
	0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
	0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
	0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
	0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,
	0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
	0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f
};


#define KB_DELAY		20
#define KB_AUTOREPEAT		3

static int GetKeyPress(UBYTE *screen)
{
	int keycode;

#ifndef BASIC
#ifdef SVGA_SPEEDUP
	int i;
	for (i = 0; i < refresh_rate; i++)
#endif
		Atari_DisplayScreen(screen);
#endif

	for (;;) {
		static int rep = KB_DELAY;
		if (Atari_Keyboard() == AKEY_NONE) {
			rep = KB_DELAY;
			atari_sync();
			break;
		}
		if (rep == 0) {
			rep = KB_AUTOREPEAT;
			break;
		}
		rep--;
		atari_sync();
	}

	do {
		atari_sync();
		keycode = Atari_Keyboard();
		switch (keycode) {
		case AKEY_WARMSTART:
			alt_function = MENU_RESETW;
			return 0x1b; /* escape */
		case AKEY_COLDSTART:
			alt_function = MENU_RESETC;
			return 0x1b; /* escape */
		case AKEY_EXIT:
			alt_function = MENU_EXIT;
			return 0x1b; /* escape */
		case AKEY_UI:
			if (alt_function >= 0) /* Alt+letter, not F1 */
				return 0x1b; /* escape */
			break;
		case AKEY_SCREENSHOT:
			alt_function = MENU_PCX;
			return 0x1b; /* escape */
		case AKEY_SCREENSHOT_INTERLACE:
			alt_function = MENU_PCXI;
			return 0x1b; /* escape */
		default:
			break;
		}
	} while (keycode < 0);

	return key_to_ascii[keycode];
}

static void Plot(UBYTE *screen, int fg, int bg, int ch, int x, int y)
{
#ifndef USE_CURSES
	int offset = ascii_to_screen[(ch & 0x07f)] * 8;
	int i;
	int j;

	UBYTE *ptr;

	ptr = screen + 24 * ATARI_WIDTH + 32 + y * (8 * ATARI_WIDTH) + (x << 3);

	for (i = 0; i < 8; i++) {
		UBYTE data;

		data = charset[offset++];

		for (j = 0; j < 8; j++) {
#ifdef USE_COLOUR_TRANSLATION_TABLE
			video_putbyte(ptr++, colour_translation_table[data & 0x80 ? fg : bg]);
#else
			video_putbyte(ptr++, data & 0x80 ? fg : bg);
#endif
			data <<= 1;
		}

		ptr += ATARI_WIDTH - 8;
	}
#else
	UBYTE mask = fg == 0x94 ? 0x80 : 0;

	/* handle line drawing chars */
	switch (ch) {
	case 18:
		curses_screen[y][x] = '-' - 32 + mask;
		break;
	case 17:
	case 3:
		curses_screen[y][x] = '/' - 32 + mask;
		break;
	case 26:
	case 5:
		curses_screen[y][x] = '\\' - 32 + mask;
		break;
	case 124:
		curses_screen[y][x] = '|' + mask;
		break;
	default:
		if (ch >= 'a' && ch <= 'z')
			curses_screen[y][x] = ch + mask;
		else
			curses_screen[y][x] = ch - 32 + mask;
		break;
	}
#endif
}

static void Print(UBYTE *screen, int fg, int bg, const char *string, int x, int y)
{
	while (*string && *string != '\n') {
		Plot(screen, fg, bg, *string++, x, y);
		x++;
	}
}

static void CenterPrint(UBYTE *screen, int fg, int bg, const char *string, int y)
{
	int length;
	char *eol = strchr(string, '\n');
	while (eol != NULL && string[0]) {
		length = eol - string - 1;
		Print(screen, fg, bg, string, (40 - length) / 2, y++);
		string = eol + 1;
		eol = strchr(string, '\n');
	}
	length = strlen(string);
	Print(screen, fg, bg, string, (40 - length) / 2, y);
}

static int EditString(UBYTE *screen, int fg, int bg,
                      int len, char *string, int x, int y)
{
	int offset = 0;

	Print(screen, fg, bg, string, x, y);

	for (;;) {
		int ascii;

		Plot(screen, bg, fg, string[offset], x + offset, y);

		ascii = GetKeyPress(screen);
		switch (ascii) {
		case 0x1e:				/* Cursor Left */
			Plot(screen, fg, bg, string[offset], x + offset, y);
			if (offset > 0)
				offset--;
			break;
		case 0x1f:				/* Cursor Right */
			Plot(screen, fg, bg, string[offset], x + offset, y);
			if ((offset + 1) < len)
				offset++;
			break;
		case 0x7e:				/* Backspace */
			Plot(screen, fg, bg, string[offset], x + offset, y);
			if (offset > 0) {
				offset--;
				string[offset] = ' ';
			}
			break;
		case 0x9b:				/* Return */
			return TRUE;
		case 0x1b:				/* Esc */
			return FALSE;
		default:
			string[offset] = (char) ascii;
			Plot(screen, fg, bg, string[offset], x + offset, y);
			if ((offset + 1) < len)
				offset++;
			break;
		}
	}
}

static void Box(UBYTE *screen, int fg, int bg, int x1, int y1, int x2, int y2)
{
	int x;
	int y;

	for (x = x1 + 1; x < x2; x++) {
		Plot(screen, fg, bg, 18, x, y1);
		Plot(screen, fg, bg, 18, x, y2);
	}

	for (y = y1 + 1; y < y2; y++) {
		Plot(screen, fg, bg, 124, x1, y);
		Plot(screen, fg, bg, 124, x2, y);
	}

	Plot(screen, fg, bg, 17, x1, y1);
	Plot(screen, fg, bg, 5, x2, y1);
	Plot(screen, fg, bg, 3, x2, y2);
	Plot(screen, fg, bg, 26, x1, y2);
}

static void ClearScreen(UBYTE *screen)
{
#ifdef USE_CURSES
	curses_clear_screen();
#else
	UBYTE *ptr;
#ifdef USE_COLOUR_TRANSLATION_TABLE
	video_memset(screen, colour_translation_table[0x00], ATARI_HEIGHT * ATARI_WIDTH);
	for (ptr = screen + ATARI_WIDTH * 24 + 32; ptr < screen + ATARI_WIDTH * (24 + 192); ptr += ATARI_WIDTH)
		video_memset(ptr, colour_translation_table[0x94], 320);
#else
	video_memset(screen, 0x00, ATARI_HEIGHT * ATARI_WIDTH);
	for (ptr = screen + ATARI_WIDTH * 24 + 32; ptr < screen + ATARI_WIDTH * (24 + 192); ptr += ATARI_WIDTH)
		video_memset(ptr, 0x94, 320);
#endif
#endif
}

static int CountLines(const char *string)
{
	int lines;
	if (string == NULL || *string == 0)
		return 0;

	lines = 1;
	while ((string = strchr(string, '\n')) != NULL) {
		lines++;
		string++;
	}
	return lines;
}

static void TitleScreen(UBYTE *screen, char *title)
{
	Box(screen, 0x9a, 0x94, 0, 0, 39, 1 + CountLines(title));
	CenterPrint(screen, 0x9a, 0x94, title, 1);
}

static void ShortenItem(const char *source, char *destination, int iMaxXsize)
{
	if ((int) strlen(source) > iMaxXsize) {

		int iFirstLen = (iMaxXsize - 3) / 2;
		int iLastStart = strlen(source) - (iMaxXsize - 3 - iFirstLen);
		strncpy(destination, source, iFirstLen);
		destination[iFirstLen] = '\0';
		strcat(destination, "...");
		strcat(destination, source + iLastStart);

	}
	else
		strcpy(destination, source);
}

static void SelectItem(UBYTE *screen, int fg, int bg,
                       int index, char *items[],
                       char *prefix[], char *suffix[],
                       int nrows, int ncolumns,
                       int xoffset, int yoffset,
                       int itemwidth,
                       int active)
{
	int x;
	int y;
	int iMaxXsize = ((40 - xoffset) / ncolumns) - 1;
	char szOrig[FILENAME_MAX + 40]; /* allow for prefix and suffix */
	char szString[41];
	int spaceToAdd;

	x = index / nrows;
	y = index - (x * nrows);

	x = x * (40 / ncolumns);

	x += xoffset;
	y += yoffset;

	szOrig[0] = 0;
	if (prefix && prefix[index])
		strcat(szOrig, prefix[index]);
	strcat(szOrig, items[index]);
	if (suffix && suffix[index]) {
		spaceToAdd = itemwidth - strlen(szOrig) - strlen(suffix[index]);
		if (spaceToAdd > 0)
			do
				strcat(szOrig, " ");
			while (--spaceToAdd);
		strcat(szOrig, suffix[index]);
	}
	else {
		spaceToAdd = itemwidth - strlen(szOrig);
		if (spaceToAdd > 0)
			do
				strcat(szOrig, " ");
			while (--spaceToAdd);
	}

	if (strlen(szOrig) > 3) {
		int iKnownExt = FALSE;

		if (!strcasecmp(szOrig + strlen(szOrig) - 3, "ATR"))
			iKnownExt = TRUE;

		if (!strcasecmp(szOrig + strlen(szOrig) - 3, "XFD"))
			iKnownExt = TRUE;

		if (iKnownExt) {
			szOrig[strlen(szOrig) - 4] = '\0';
		}
	}

	ShortenItem(szOrig, szString, iMaxXsize);

	Print(screen, fg, bg, szString, x, y);

	if (active) {
		char empty[41];
		int ln;

		memset(empty, ' ', 38);
		empty[38] = '\0';
		Print(screen, bg, fg, empty, 1, 22);

		ShortenItem(szOrig, szString, 38);
		ln = strlen(szString);

		if (ln > iMaxXsize)
			CenterPrint(screen, fg, bg, szString, 22);	/*the selected item was shortened */
	}
}

static int Select(UBYTE *screen,
                  int default_item,
                  int nitems, char *items[],
                  char *prefix[], char *suffix[],
                  int nrows, int ncolumns,
                  int xoffset, int yoffset,
                  int itemwidth,
                  int scrollable,
                  int *ascii)
{
	int index = 0;
	int localascii;

	if (ascii == NULL)
		ascii = &localascii;

	for (index = 0; index < nitems; index++)
		SelectItem(screen, 0x9a, 0x94, index, items, prefix, suffix, nrows, ncolumns, xoffset, yoffset, itemwidth, FALSE);

	index = default_item;
	SelectItem(screen, 0x94, 0x9a, index, items, prefix, suffix, nrows, ncolumns, xoffset, yoffset, itemwidth, TRUE);

	for (;;) {
		int row;
		int column;
		int new_index;

		column = index / nrows;
		row = index - (column * nrows);

		*ascii = GetKeyPress(screen);
		switch (*ascii) {
		case 0x1c:				/* Up */
			if (row > 0)
				row--;
			else
				/*GOLDA CHANGED */ if (column > 0) {
				column--;
				row = nrows - 1;
			}
			else if (scrollable)
				return index + nitems + (nrows - 1);
			break;
		case 0x1d:				/* Down */
			if (row < (nrows - 1))
				row++;
			else
				/*GOLDA CHANGED */ if (column < (ncolumns - 1)) {
				row = 0;
				column++;
			}
			else if (scrollable)
				return index + nitems * 2 - (nrows - 1);
			break;
		case 0x1e:				/* Left */
			if (column > 0)
				column--;
			else if (scrollable)
				return index + nitems;
			break;
		case 0x1f:				/* Right */
			if (column < (ncolumns - 1))
				column++;
			else if (scrollable)
				return index + nitems * 2;
			break;
		case 0x7f:				/* Tab (for exchanging disk directories) */
			return -2;			/* GOLDA CHANGED */
		case 0x20:				/* Space */
		case 0x7e:				/* Backspace */
		case 0x9b:				/* Select */
			return index;
		case 0x1b:				/* Cancel */
			return -1;
		default:
			break;
		}

		new_index = (column * nrows) + row;
		if ((new_index >= 0) && (new_index < nitems)) {
			SelectItem(screen, 0x9a, 0x94, index, items, prefix, suffix, nrows, ncolumns, xoffset, yoffset, itemwidth, FALSE);

			index = new_index;
			SelectItem(screen, 0x94, 0x9a, index, items, prefix, suffix, nrows, ncolumns, xoffset, yoffset, itemwidth, TRUE);
		}
	}
}

/* returns TRUE if valid filename */
static int EditFilename(UBYTE *screen, char *fname)
{
	memset(fname, ' ', FILENAME_SIZE);
	fname[FILENAME_SIZE] = '\0';
	Box(screen, 0x9a, 0x94, 3, 9, 36, 11);
	Print(screen, 0x94, 0x9a, "Filename", 4, 9);
	if (!EditString(screen, 0x9a, 0x94, FILENAME_SIZE, fname, 4, 10))
		return FALSE;
	RemoveSpaces(fname);
	return fname[0] != '\0';
}

#ifdef DO_DIR

static int FilenameSort(const char *filename1, const char *filename2)
{
	if (*filename1 == '[' && *filename2 != '[')
		return -1;
	if (*filename1 != '[' && *filename2 == '[')
		return 1;
	if (*filename1 == '[' && *filename2 == '[') {
		if (filename1[1] == '.')
			return -1;
		else if (filename2[1] == '.')
			return 1;
	}

	return strcmp(filename1, filename2);
}

static List *GetDirectory(char *directory)
{
	DIR *dp = NULL;
	List *list = NULL;
	struct stat st;
	char fullfilename[FILENAME_MAX];
	char *filepart;
#ifdef DOS_DRIVES
	static char letter[3] = { 'C', ':', '\0' };
	static char letter2[5] = { '[', 'C', ':', ']', '\0' };
#ifdef __DJGPP__
	unsigned short s_backup = _djstat_flags;
	_djstat_flags = _STAT_INODE | _STAT_EXEC_EXT | _STAT_EXEC_MAGIC | _STAT_DIRSIZE |
		_STAT_ROOT_TIME | _STAT_WRITEBIT;
	/*we do not need any of those 'hard-to-get' informations */
#endif	/* DJGPP */
#endif	/* DOS_DRIVES */
	strcpy(fullfilename, directory);
	filepart = fullfilename + strlen(fullfilename);
#ifdef BACK_SLASH
	if ((filepart == fullfilename) || *(filepart - 1) != '\\')
		*filepart++ = '\\';
#else
	if ((filepart == fullfilename) || *(filepart - 1) != '/')
		*filepart++ = '/';
#endif

	dp = opendir(directory);
	if (dp) {
		struct dirent *entry;

		list = ListCreate();
		if (!list) {
			Aprint("ListCreate(): Failed\n");
			return NULL;
		}
		while ((entry = readdir(dp))) {
			char *filename;

			if (strcmp(entry->d_name, ".") == 0)
				continue;

			strcpy(filepart, entry->d_name);	/*create full filename */
			stat(fullfilename, &st);
			if (st.st_mode & S_IFDIR) {		/*directories add as  [dir] */
				int len;

				len = strlen(entry->d_name);
				if ( (filename = (char *) malloc(len + 3)) ) {
					strcpy(filename + 1, entry->d_name);
					filename[0] = '[';
					filename[len + 1] = ']';
					filename[len + 2] = '\0';
				}
			}
			else
				filename = (char *) strdup(entry->d_name);

			if (!filename) {
				perror("strdup");
				return NULL;
			}
			ListAddTail(list, filename);
		}

		closedir(dp);

		ListSort(list, (void *) FilenameSort);
	}
	else {
		Aprint("Error opening '%s' directory", directory);
	}
#ifdef DOS_DRIVES
	/* in DOS, add all existing disk letters */
	ListAddTail(list, strdup("[A:]"));	/* do not check A: - it's slow */
	letter[0] = 'C';
	while (letter[0] <= 'Z') {
#ifdef __DJGPP__
		stat(letter, &st);
		if (st.st_mode & S_IXUSR)
#endif
		{
			letter2[1] = letter[0];
			ListAddTail(list, strdup(letter2));
		}
		(letter[0])++;
	}
#ifdef __DJGPP__
	_djstat_flags = s_backup;	/* return the original state */
#endif
#endif

	return list;
}

#endif /* DO_DIR */

static int FileSelector(UBYTE *screen, char *directory, char *full_filename)
{
#ifdef DO_DIR
	List *list;
	int flag = FALSE;
	int next_dir;

	do {
#ifdef __DJGPP__
		char helpdir[FILENAME_MAX];
		_fixpath(directory, helpdir);
		strcpy(directory, helpdir);
#else
		if (directory[0] == '\0' || strcmp(directory, ".") == 0)
			getcwd(directory, FILENAME_MAX);
#endif
		next_dir = FALSE;
		list = GetDirectory(directory);
		if (list) {
			char *filename;
			int nitems = 0;
			int item = 0;
			int done = FALSE;
			int offset = 0;
			int nfiles = 0;

#define NROWS 18
#define NCOLUMNS 2
#define MAX_FILES (NROWS * NCOLUMNS)

			char *files[MAX_FILES];

			ListReset(list);
			while (ListTraverse(list, (void *) &filename))
				nfiles++;

			if (!nfiles) {
				ListFree(list, (void *) free);
				BasicUIMessage("No files inside directory");
				break;
			}

			while (!done) {
				int ascii;

				ListReset(list);
				for (nitems = 0; nitems < offset; nitems++)
					ListTraverse(list, (void *) &filename);

				for (nitems = 0; nitems < MAX_FILES; nitems++) {
					if (ListTraverse(list, (void *) &filename)) {
						files[nitems] = filename;
					}
					else
						break;
				}

				ClearScreen(screen);
#if 1
				TitleScreen(screen, "Select File");
#else
				TitleScreen(screen, directory);
#endif
				Box(screen, 0x9a, 0x94, 0, 3, 39, 23);

				if (item < 0)
					item = 0;
				else if (item >= nitems)
					item = nitems - 1;
				item = Select(screen, item, nitems, files, NULL, NULL, NROWS, NCOLUMNS, 1, 4, 37/NCOLUMNS, TRUE, &ascii);

				if (item >= (nitems * 2 + NROWS)) {		/* Scroll Right */
					if ((offset + NROWS + NROWS) < nfiles)
						offset += NROWS;
					item = item % nitems;
				}
				else if (item >= nitems) {	/* Scroll Left */
					if ((offset - NROWS) >= 0) {
						offset -= NROWS;
						item = item % nitems;
					}
					else
						item = 0;
				}
				else if (item == -2) {	/* Next directory */
					DIR *dr;
					do {
						current_disk_directory = (current_disk_directory + 1) % disk_directories;
						strcpy(directory, atari_disk_dirs[current_disk_directory]);
						dr = opendir(directory);
					} while (dr == NULL);
					closedir(dr);
/*                  directory = atari_disk_dirs[current_disk_directory]; */
					next_dir = TRUE;
					break;
				}
				else if (item != -1) {
					if (files[item][0] == '[') {	/* directory selected */
						DIR *dr;
						char help[FILENAME_MAX];	/* new directory */

						if (strcmp(files[item], "[..]") == 0) {		/* go up */
							char *pos, *pos2;

							strcpy(help, directory);
							pos = strrchr(help, '/');
							if (!pos)
								pos = strrchr(help, '\\');
							if (pos) {
								*pos = '\0';
								/* if there is no slash in directory, add one at the end */
								pos2 = strrchr(help, '/');
								if (!pos2)
									pos2 = strrchr(help, '\\');
								if (!pos2) {
#ifdef BACK_SLASH
									*pos++ = '\\';
#else
									*pos++ = '/';
#endif
									*pos++ = '\0';
								}
							}

						}
#ifdef DOS_DRIVES
						else if (files[item][2] == ':' && files[item][3] == ']') {	/* disk selected */
							strcpy(help, files[item] + 1);
							help[2] = '\\';
							help[3] = '\0';
						}
#endif
						else {	/* directory selected */
							char lastchar = directory[strlen(directory) - 1];
							char *pbracket = strchr(files[item], ']');

							if (pbracket)
								*pbracket = '\0';	/*cut ']' */
							if (lastchar == '/' || lastchar == '\\')
								sprintf(help, "%s%s", directory, files[item] + 1);	/* directory already ends with slash */
							else
#ifndef BACK_SLASH
								sprintf(help, "%s/%s", directory, files[item] + 1);
#else
								sprintf(help, "%s\\%s", directory, files[item] + 1);
#endif
						}
						dr = opendir(help);		/* check, if new directory is valid */
						if (dr) {
							strcpy(directory, help);
							closedir(dr);
							next_dir = TRUE;
							break;
						}
					}
					else {		/* normal filename selected */
						char lastchar = directory[strlen(directory) - 1];
						if (lastchar == '/' || lastchar == '\\')
							sprintf(full_filename, "%s%s", directory, files[item]);		/* directory already ends with slash */
						else
#ifndef BACK_SLASH
							sprintf(full_filename, "%s/%s", directory, files[item]);
#else							/* DOS, TOS fs */
							sprintf(full_filename, "%s\\%s", directory, files[item]);
#endif
						flag = TRUE;
						break;
					}
				}
				else
					break;
			}

			ListFree(list, (void *) free);
		}
	} while (next_dir);
	return flag;
#else /* DO_DIR */
	char fname[FILENAME_SIZE + 1];
	if (EditFilename(screen, fname)) {
		char lastchar;
		if (directory[0] == '\0' || strcmp(directory, ".") == 0)
#ifdef HAVE_GETCWD
			getcwd(directory, FILENAME_MAX);
#else
			strcpy(directory, ".");
#endif
		lastchar = directory[strlen(directory) - 1];
		if (lastchar == '/' || lastchar == '\\' || fname[0] == '/' || fname[0] == '\\')
			/* directory already ends with slash or fname is absolute */
			sprintf(full_filename, "%s%s", directory, fname);
		else
#ifndef BACK_SLASH
			sprintf(full_filename, "%s/%s", directory, fname);
#else							/* DOS, TOS fs */
			sprintf(full_filename, "%s\\%s", directory, fname);
#endif
		return TRUE;
	}
	return FALSE;
#endif /* DO_DIR */
}

static void AboutEmulator(UBYTE *screen)
{
	ClearScreen(screen);

	Box(screen, 0x9a, 0x94, 0, 0, 39, 8);
	CenterPrint(screen, 0x9a, 0x94, ATARI_TITLE, 1);
	CenterPrint(screen, 0x9a, 0x94, "Copyright (c) 1995-1998 David Firth", 2);
	CenterPrint(screen, 0x9a, 0x94, "and", 3);
	CenterPrint(screen, 0x9a, 0x94, "(c)1998-2005 Atari800 Development Team", 4);
	CenterPrint(screen, 0x9a, 0x94, "See CREDITS file for details.", 5);
	CenterPrint(screen, 0x9a, 0x94, "http://atari800.atari.org/", 7);

	Box(screen, 0x9a, 0x94, 0, 9, 39, 23);
	CenterPrint(screen, 0x9a, 0x94, "This program is free software; you can", 10);
	CenterPrint(screen, 0x9a, 0x94, "redistribute it and/or modify it under", 11);
	CenterPrint(screen, 0x9a, 0x94, "the terms of the GNU General Public", 12);
	CenterPrint(screen, 0x9a, 0x94, "License as published by the Free", 13);
	CenterPrint(screen, 0x9a, 0x94, "Software Foundation; either version 1,", 14);
	CenterPrint(screen, 0x9a, 0x94, "or (at your option) any later version.", 15);

	CenterPrint(screen, 0x94, 0x9a, "Press any Key to Continue", 22);
	GetKeyPress(screen);
}


static int MenuSelectEx(UBYTE *screen, char *title, int subitem,
                        int default_item, tMenuItem *items, int *seltype)
{
	int scrollable;
	int i;
	int w, ws;
	int x1, y1, x2, y2;
	int nitems;
	char *prefix[100];
	char *root[100];
	char *suffix[100];
	int retval;
	int ascii;

	nitems = 0;
	retval = 0;
	for (i = 0; items[i].sig; i++) {
		if (items[i].flags & ITEM_ENABLED) {
			prefix[nitems] = items[i].prefix;
			root[nitems] = items[i].item;
			if (items[i].flags & ITEM_CHECK) {
				if (items[i].flags & ITEM_CHECKED)
					suffix[nitems] = "Yes";
				else
					suffix[nitems] = "No ";
			}
			else
				suffix[nitems] = items[i].suffix;
			if (items[i].retval == default_item)
				retval = nitems;
			nitems ++;
		}
	}

	if (nitems == 0)
		return -1; /* cancel immediately */

	if (!subitem) {
		ClearScreen(screen);
		TitleScreen(screen, title);
	}

	if (subitem) {
		w = 0;
		for (i = 0; i  <nitems; i++) {
			ws = strlen(root[i]);
			if (prefix && prefix[i])
				ws += strlen(prefix[i]);
			if (suffix && suffix[i])
				ws += strlen(suffix[i]);
			if (ws > w)
				w = ws;
		}
		if (w > 38)
			w = 38;

		x1 = (40 - w) / 2 - 1;
		x2 = x1 + w + 1;
		y1 = (24 - nitems) / 2 - 1;
		y2 = y1 + nitems + 1;
	}
	else {
		w = 38;
		x1 = 0;
		y1 = CountLines(title) + 2;
		x2 = 39;
		y2 = 23;
	}

	Box(screen, 0x9a, 0x94, x1, y1, x2, y2);
	scrollable = (nitems > y2 - y1 - 1);
	retval = Select(screen, retval, nitems, root, prefix, suffix, nitems, 1, x1 + 1, y1 + 1, w, scrollable, &ascii);
	if (retval < 0)
		return retval;
	for (i = 0; items[i].sig; i++) {
		if (items[i].flags & ITEM_ENABLED) {
			if (retval == 0) {
				if ((items[i].flags & ITEM_MULTI) && seltype) {
					switch (ascii) {
					case 0x9b:
						*seltype = USER_SELECT;
						break;
					case 0x20:
						*seltype = USER_TOGGLE;
						break;
					default:
						*seltype = USER_OTHER;
					}
				}
				return items[i].retval;
			}
			else
				retval--;
		}
	}
	return 0;
}

static void Message(UBYTE* screen, char* msg)
{
	CenterPrint(screen, 0x94, 0x9a, msg, 22);
	GetKeyPress(screen);
}

static void InitializeUI(void)
{
	if (!initialised) {
		get_charset(charset);
		initialised = TRUE;
	}
}

/* UI Driver entries */

#ifdef CURSES_BASIC
#define atari_screen NULL
#endif

int BasicUISelect(char *pTitle, int bFloat, int nDefault, tMenuItem *menu, int *ascii)
{
	return MenuSelectEx((UBYTE *) atari_screen, pTitle, bFloat, nDefault, menu, ascii);
}

int BasicUIGetSaveFilename(char *pFilename)
{
	return EditFilename((UBYTE *) atari_screen, pFilename);
}

int BasicUIGetLoadFilename(char *pDirectory, char *pFilename)
{
	return FileSelector((UBYTE *) atari_screen, pDirectory, pFilename);
}

void BasicUIMessage(char *pMessage)
{
	Message((UBYTE *) atari_screen, pMessage);
}

void BasicUIAboutBox(void)
{
	AboutEmulator((UBYTE *) atari_screen);
}

void BasicUIInit(void)
{
	InitializeUI();
}


/*
$Log: ui_basic.c,v $
Revision 1.25  2005/08/18 23:34:00  pfusik
shortcut keys in UI

Revision 1.24  2005/08/17 22:49:15  pfusik
compile without <dirent.h>

Revision 1.23  2005/08/16 23:07:28  pfusik
#include "config.h" before system headers

Revision 1.22  2005/08/15 17:27:00  pfusik
char charset[] -> UBYTE charset[]

Revision 1.21  2005/08/14 08:44:23  pfusik
avoid negative array indexes with special keys pressed in UI;
fixed indentation

Revision 1.20  2005/08/13 08:53:42  pfusik
CURSES_BASIC; fixed indentation

Revision 1.19  2005/08/06 18:25:40  pfusik
changed () function signatures to (void)

Revision 1.18  2005/05/20 09:08:17  pfusik
fixed some warnings

Revision 1.17  2005/03/10 04:41:26  pfusik
fixed a memory leak

Revision 1.16  2005/03/08 04:32:46  pfusik
killed gcc -W warnings

Revision 1.15  2005/03/05 12:34:08  pfusik
fixed "Error opening '' directory"

Revision 1.14  2005/03/03 09:27:46  pfusik
moved atari_screen to screen.h

Revision 1.13  2004/09/24 15:28:40  sba
Fixed NULL pointer access in filedialog, which happened if no files are within the directory.

Revision 1.12  2004/08/08 08:41:47  joy
copyright year increased

Revision 1.11  2003/12/21 11:00:26  joy
problem with opening invalid folders in UI identified

Revision 1.10  2003/02/24 09:33:13  joy
header cleanup

Revision 1.9  2003/02/08 23:52:17  joy
little cleanup

Revision 1.8  2003/01/27 13:14:51  joy
Jason's changes: either PAGED_ATTRIB support (mostly), or just clean up.

Revision 1.7  2002/06/12 06:40:41  vasyl
Fixed odd behavior of Up button on the first item in file selector

Revision 1.6  2002/03/30 06:19:28  vasyl
Dirty rectangle scheme implementation part 2.
All video memory accesses everywhere are going through the same macros
in ANTIC.C. UI_BASIC does not require special handling anymore. Two new
functions are exposed in ANTIC.H for writing to video memory.

Revision 1.5  2001/11/29 12:36:42  joy
copyright notice updated

Revision 1.4  2001/10/16 17:11:27  knik
keyboard autorepeat rate changed

Revision 1.3  2001/10/11 17:27:22  knik
added atari_sync() call in keyboard loop--keyboard is sampled
at reasonable rate

*/
