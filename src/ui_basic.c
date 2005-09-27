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
#ifdef HAVE_DIRECT_H
#include <direct.h> /* getcwd on MSVC*/
#endif
/* XXX: <sys/dir.h>, <ndir.h>, <sys/ndir.h> */
#ifdef HAVE_DIRENT_H
#include <dirent.h>
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef WIN32
#include <windows.h>
#endif

#include "antic.h"
#include "atari.h"
#include "input.h"
#include "log.h"
#include "memory.h"
#include "platform.h"
#include "rt-config.h"
#include "screen.h" /* atari_screen */
#include "ui.h"
#include "util.h"

#ifdef USE_CURSES
void curses_clear_screen(void);
void curses_putch(int x, int y, int ascii, UBYTE fg, UBYTE bg);
#endif

extern int current_disk_directory;

static int initialised = FALSE;
static UBYTE charset[1024];

static const unsigned char key_to_ascii[256] =
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

static void InitializeUI(void)
{
	if (!initialised) {
		get_charset(charset);
		initialised = TRUE;
	}
}

#define KB_DELAY       20
#define KB_AUTOREPEAT  3

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
#ifdef USE_CURSES
	curses_putch(x, y, ch, (UBYTE) fg, (UBYTE) bg);
#else /* USE_CURSES */
	const UBYTE *font_ptr = charset + (ch & 0x7f) * 8;
	UBYTE *ptr = screen + 24 * ATARI_WIDTH + 32 + y * (8 * ATARI_WIDTH) + x * 8;
	int i;
	int j;

	for (i = 0; i < 8; i++) {
		UBYTE data = *font_ptr++;
		for (j = 0; j < 8; j++) {
#ifdef USE_COLOUR_TRANSLATION_TABLE
			video_putbyte(ptr++, (UBYTE) colour_translation_table[data & 0x80 ? fg : bg]);
#else
			video_putbyte(ptr++, (UBYTE) (data & 0x80 ? fg : bg));
#endif
			data <<= 1;
		}
		ptr += ATARI_WIDTH - 8;
	}
#endif /* USE_CURSES */
}

static void Print(UBYTE *screen, int fg, int bg, const char *string, int x, int y)
{
	while (*string != '\0' && *string != '\n') {
		Plot(screen, fg, bg, *string++, x, y);
		x++;
	}
}

static void CenterPrint(UBYTE *screen, int fg, int bg, const char *string, int y)
{
	int length;
	const char *eol = strchr(string, '\n');
	while (eol != NULL && string[0] != '\0') {
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
#endif /* USE_CURSES */
}

static int CountLines(const char *string)
{
	int lines;
	if (string == NULL || *string == '\0')
		return 0;

	lines = 1;
	while ((string = strchr(string, '\n')) != NULL) {
		lines++;
		string++;
	}
	return lines;
}

static void TitleScreen(UBYTE *screen, const char *title)
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

static void Message(UBYTE *screen, const char *msg)
{
	char buf[40];
	ShortenItem(msg, buf, 38);
	CenterPrint(screen, 0x94, 0x9a, buf, 22);
	GetKeyPress(screen);
}

static void SelectItem(UBYTE *screen, int fg, int bg,
                       int index, const char *items[],
                       const char *prefix[], const char *suffix[],
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

#if 0
	if (strlen(szOrig) > 4) {
		char *ext = szOrig + strlen(szOrig) - 4;
		if (Util_stricmp(ext, ".ATR") == 0 || Util_stricmp(ext, ".XFD") == 0)
			*ext = '\0';
	}
#endif

	ShortenItem(szOrig, szString, iMaxXsize);

	Print(screen, fg, bg, szString, x, y);

	if (active) {
		char empty[41];
		memset(empty, ' ', 38);
		empty[38] = '\0';
		Print(screen, bg, fg, empty, 1, 22);
		ShortenItem(szOrig, szString, 38);
		if (strlen(szString) > iMaxXsize)
			/* the selected item was shortened */
			CenterPrint(screen, fg, bg, szString, 22);
	}
}

static int Select(UBYTE *screen,
                  int default_item,
                  int nitems, const char *items[],
                  const char *prefix[], const char *suffix[],
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
			else if (column > 0) {
				column--;
				row = nrows - 1;
			}
			else if (scrollable)
				return index + nitems + (nrows - 1);
			break;
		case 0x1d:				/* Down */
			if (row < (nrows - 1))
				row++;
			else if (column < (ncolumns - 1)) {
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
		case 0x9b:				/* Return=Select */
			return index;
		case 0x1b:				/* Esc=Cancel */
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
	Util_trim(fname);
	return fname[0] != '\0';
}

#ifdef WIN32

static WIN32_FIND_DATA wfd;
static HANDLE dh = INVALID_HANDLE_VALUE;

#ifdef _WIN32_WCE
/* WinCE's FindFirstFile/FindNext file don't return "." or "..". */
/* We check if the parent folder exists and add ".." if necessary. */
static char parentdir[FILENAME_MAX];
#endif

static int BasicUIOpenDir(const char *dirname)
{
#ifdef UNICODE
	WCHAR wfilespec[FILENAME_MAX];
	if (MultiByteToWideChar(CP_ACP, 0, dirname, -1, wfilespec, FILENAME_MAX - 4) <= 0)
		return FALSE;
	wcscat(wfilespec, (dirname[0] != '\0' && dirname[strlen(dirname) - 1] != '\\')
		? L"\\*.*" : L"*.*");
	dh = FindFirstFile(wfilespec, &wfd);
#else /* UNICODE */
	char filespec[FILENAME_MAX];
	Util_strlcpy(filespec, dirname, FILENAME_MAX - 4);
	strcat(filespec, (dirname[0] != '\0' && dirname[strlen(dirname) - 1] != '\\')
		? "\\*.*" : "*.*");
	dh = FindFirstFile(filespec, &wfd);
#endif /* UNICODE */
#ifdef _WIN32_WCE
	Util_splitpath(dirname, parentdir, NULL);
#endif
	return dh != INVALID_HANDLE_VALUE;
}

static int BasicUIReadDir(char *filename, int *isdir)
{
	if (dh == INVALID_HANDLE_VALUE) {
#ifdef _WIN32_WCE
		if (parentdir[0] != '\0' && Util_direxists(parentdir)) {
			strcpy(filename, "..");
			*isdir = TRUE;
			parentdir[0] = '\0';
			return TRUE;
		}
#endif /* _WIN32_WCE */
		return FALSE;
	}
#ifdef UNICODE
	if (WideCharToMultiByte(CP_ACP, 0, wfd.cFileName, -1, filename, FILENAME_MAX, NULL, NULL) <= 0)
		filename[0] = '\0';
#else
	Util_strlcpy(filename, wfd.cFileName, FILENAME_MAX);
#endif /* UNICODE */
#ifdef _WIN32_WCE
	/* just in case they will implement it some day */
	if (strcmp(filename, "..") == 0)
		parentdir[0] = '\0';
#endif
	*isdir = (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? TRUE : FALSE;
	if (!FindNextFile(dh, &wfd)) {
		FindClose(dh);
		dh = INVALID_HANDLE_VALUE;
	}
	return TRUE;
}

#define DO_DIR

#elif defined(HAVE_OPENDIR)

static char dir_path[FILENAME_MAX];
static DIR *dp = NULL;

static int BasicUIOpenDir(const char *dirname)
{
	Util_strlcpy(dir_path, dirname, FILENAME_MAX);
	dp = opendir(dir_path);
	return dp != NULL;
}

static int BasicUIReadDir(char *filename, int *isdir)
{
	struct dirent *entry;
	char fullfilename[FILENAME_MAX];
	struct stat st;
	entry = readdir(dp);
	if (entry == NULL) {
		closedir(dp);
		dp = NULL;
		return FALSE;
	}
	strcpy(filename, entry->d_name);
	Util_catpath(fullfilename, dir_path, entry->d_name);
	stat(fullfilename, &st);
	*isdir = (st.st_mode & S_IFDIR) ? TRUE : FALSE;
	return TRUE;
}

#define DO_DIR

#endif /* defined(HAVE_OPENDIR) */

#ifdef DO_DIR

static char **filenames;
#define FILENAMES_INITIAL_SIZE 256 /* preallocate 1 KB */
static int n_filenames;

/* filename must be malloc'ed or strdup'ed */
static void FilenamesAdd(char *filename)
{
	if (n_filenames >= FILENAMES_INITIAL_SIZE && (n_filenames & (n_filenames - 1)) == 0) {
		/* n_filenames is a power of two: allocate twice as much */
		filenames = (char **) Util_realloc(filenames, 2 * n_filenames * sizeof(char *));
	}
	filenames[n_filenames++] = filename;
}

static int FilenamesCmp(const char *filename1, const char *filename2)
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

	return Util_stricmp(filename1, filename2);
}

/* quicksort */
static void FilenamesSort(const char **start, const char **end)
{
	while (start + 1 < end) {
		const char **left = start + 1;
		const char **right = end;
		const char *pivot = *start;
		const char *tmp;
		while (left < right) {
			if (FilenamesCmp(*left, pivot) <= 0)
				left++;
			else {
				right--;
				tmp = *left;
				*left = *right;
				*right = tmp;
			}
		}
		left--;
		tmp = *left;
		*left = *start;
		*start = tmp;
		FilenamesSort(start, left);
		start = right;
	}
}

static void FilenamesFree(void)
{
	while (n_filenames > 0)
		free(filenames[--n_filenames]);
	free(filenames);
}

static void GetDirectory(char *directory)
{
#ifdef __DJGPP__
	unsigned short s_backup = _djstat_flags;
	_djstat_flags = _STAT_INODE | _STAT_EXEC_EXT | _STAT_EXEC_MAGIC | _STAT_DIRSIZE |
		_STAT_ROOT_TIME | _STAT_WRITEBIT;
	/* we do not need any of those 'hard-to-get' informations */
#endif	/* DJGPP */

	filenames = (char **) Util_malloc(FILENAMES_INITIAL_SIZE * sizeof(char *));
	n_filenames = 0;

	if (BasicUIOpenDir(directory)) {
		char filename[FILENAME_MAX];
		int isdir;

		while (BasicUIReadDir(filename, &isdir)) {
			char *filename2;

			if (filename[0] == '\0' ||
				(filename[0] == '.' && filename[1] == '\0'))
				continue;

			if (isdir) {
				/* add directories as [dir] */
				size_t len = strlen(filename);
				filename2 = (char *) Util_malloc(len + 3);
				memcpy(filename2 + 1, filename, len);
				filename2[0] = '[';
				filename2[len + 1] = ']';
				filename2[len + 2] = '\0';
			}
			else
				filename2 = Util_strdup(filename);

			FilenamesAdd(filename2);
		}

		FilenamesSort((const char **) filenames, (const char **) filenames + n_filenames);
	}
	else {
		Aprint("Error opening '%s' directory", directory);
	}
#ifdef DOS_DRIVES
	/* in DOS/Windows, add all existing disk letters */
	{
		char letter;
		for (letter = 'A'; letter <= 'Z'; letter++) {
#ifdef __DJGPP__
			static char drive[3] = "C:";
			struct stat st;
			drive[0] = letter;
			/* don't check floppies - it's slow */
			if (letter < 'C' || (stat(drive, &st) == 0 && (st.st_mode & S_IXUSR) != 0))
#elif defined(WIN32)
#ifdef UNICODE
			static WCHAR rootpath[4] = L"C:\\";
#else
			static char rootpath[4] = "C:\\";
#endif
			rootpath[0] = letter;
			if (GetDriveType(rootpath) != DRIVE_NO_ROOT_DIR)
#endif /* defined(WIN32) */
			{
				static char drive2[5] = "[C:]";
				drive2[1] = letter;
				FilenamesAdd(Util_strdup(drive2));
			}
		}
	}
#endif /* DOS_DRIVES */
#ifdef __DJGPP__
	_djstat_flags = s_backup;	/* restore the original state */
#endif
}

#endif /* DO_DIR */

static int FileSelector(UBYTE *screen, char *directory, char *full_filename)
{
#ifdef DO_DIR
	int flag = FALSE;
	int next_dir;

	do {
		int nitems = 0;
		int item = 0;
		int offset = 0;

#define NROWS 18
#define NCOLUMNS 2
#define MAX_FILES (NROWS * NCOLUMNS)

		const char **files;

#ifdef __DJGPP__
		char helpdir[FILENAME_MAX];
		_fixpath(directory, helpdir);
		strcpy(directory, helpdir);
#elif defined(HAVE_GETCWD)
		if (directory[0] == '\0' || strcmp(directory, ".") == 0)
			getcwd(directory, FILENAME_MAX);
#else
		if (directory[0] == '\0') {
			directory[0] = '.';
			directory[1] = '\0';
		}
#endif
		next_dir = FALSE;

		/* The WinCE version may spend several seconds when there are many
		   files in the directory. */
		/* The extra spaces are needed to clear the previous window title. */
		TitleScreen(screen, "            Please wait...            ");
		Atari_DisplayScreen(screen);

		GetDirectory(directory);

		if (n_filenames == 0) {
			FilenamesFree();
			Message(screen, "No files inside directory");
			break;
		}

		for (;;) {
			files = (const char **) filenames + offset;
			if (offset + MAX_FILES <= n_filenames)
				nitems = MAX_FILES;
			else
				nitems = n_filenames - offset;

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
			item = Select(screen, item, nitems, files, NULL, NULL, NROWS, NCOLUMNS, 1, 4, 37 / NCOLUMNS, TRUE, NULL);

			if (item >= nitems * 2 + NROWS) {
				/* Scroll Right */
				if (offset + NROWS + NROWS < n_filenames) {
					offset += NROWS;
					item %= nitems;
				}
				else
					item = nitems - 1;
			}
			else if (item >= nitems) {
				/* Scroll Left */
				if (offset - NROWS >= 0) {
					offset -= NROWS;
					item %= nitems;
				}
				else
					item = 0;
			}
			else if (item == -2) {
				/* Next directory */
				/* FIXME!!! possible endless loop!!! */
				do {
					current_disk_directory = (current_disk_directory + 1) % disk_directories;
					strcpy(directory, atari_disk_dirs[current_disk_directory]);
				} while (!Util_direxists(directory));
				next_dir = TRUE;
				break;
			}
			else if (item != -1) {
				if (files[item][0] == '[') {
					/* Change directory */
					char help[FILENAME_MAX]; /* new directory */

					if (strcmp(files[item], "[..]") == 0) {
						/* go up */
						char *pos;
						strcpy(help, directory);
						pos = strrchr(help, '/');
						if (pos == NULL)
							pos = strrchr(help, '\\');
						if (pos != NULL) {
							*pos = '\0';
							/* if there is no slash in directory, add one at the end */
							if (strchr(help, '/') == NULL && strchr(help, '\\') == NULL) {
								*pos = DIR_SEP_CHAR;
								pos[1] = '\0';
							}
						}

					}
#ifdef DOS_DRIVES
					else if (files[item][2] == ':' && files[item][3] == ']') {
						/* disk selected */
						help[0] = files[item][1];
						help[1] = ':';
						help[2] = '\\';
						help[3] = '\0';
					}
#endif
					else {
						/* directory selected */
						char *pbracket = strrchr(files[item], ']');
						if (pbracket != NULL)
							*pbracket = '\0';	/*cut ']' */
						Util_catpath(help, directory, files[item] + 1);
					}
					/* check if new directory is valid */
					if (Util_direxists(help)) {
						strcpy(directory, help);
						next_dir = TRUE;
						break;
					}
				}
				else {
					/* normal filename selected */
					Util_catpath(full_filename, directory, files[item]);
					flag = TRUE;
					break;
				}
			}
			else
				break;
		}

		FilenamesFree();
	} while (next_dir);
	return flag;
#else /* DO_DIR */
	char fname[FILENAME_SIZE + 1];
	if (EditFilename(screen, fname)) {
		if (directory[0] == '\0' || strcmp(directory, ".") == 0)
#ifdef HAVE_GETCWD
			getcwd(directory, FILENAME_MAX);
#else
			strcpy(directory, ".");
#endif
		Util_catpath(full_filename, directory, fname);
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


static int MenuSelectEx(UBYTE *screen, const char *title, int subitem,
                        int default_item, tMenuItem *items, int *seltype)
{
	int scrollable;
	int i;
	int w;
	int x1, y1, x2, y2;
	int nitems;
	const char *prefix[100];
	const char *root[100];
	const char *suffix[100];
	int retval;
	int ascii;

	nitems = 0;
	retval = 0;
	for (i = 0; items[i].sig[0] != '\0'; i++) {
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
			nitems++;
		}
	}

	if (nitems == 0)
		return -1; /* cancel immediately */

	if (subitem) {
		w = 0;
		for (i = 0; i < nitems; i++) {
			int ws = strlen(root[i]);
			if (prefix[i] != NULL)
				ws += strlen(prefix[i]);
			if (suffix[i] != NULL)
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
		ClearScreen(screen);
		TitleScreen(screen, title);
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
	for (i = 0; items[i].sig[0] != '\0'; i++) {
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
						break;
					}
				}
				return items[i].retval;
			}
			retval--;
		}
	}
	return 0;
}

/* UI Driver entries */

#ifdef CURSES_BASIC
#define atari_screen NULL
#endif

int BasicUISelect(const char *pTitle, int bFloat, int nDefault, tMenuItem *menu, int *ascii)
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

void BasicUIMessage(const char *pMessage)
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

tUIDriver basic_ui_driver =
{
	&BasicUISelect,
	&BasicUIGetSaveFilename,
	&BasicUIGetLoadFilename,
	&BasicUIMessage,
	&BasicUIAboutBox,
	&BasicUIInit
};

/*
$Log: ui_basic.c,v $
Revision 1.37  2005/09/27 21:41:08  pfusik
UI's charset is now in ATASCII order; curses_putch()

Revision 1.36  2005/09/18 15:08:03  pfusik
fixed file selector: last directory entry wasn't sorted;
saved a few bytes per tMenuItem

Revision 1.35  2005/09/14 20:32:18  pfusik
".." in Win32 API based file selector on WINCE;
include B: in DOS_DRIVES; detect floppies on WIN32

Revision 1.34  2005/09/11 20:38:43  pfusik
implemented file selector on MSVC

Revision 1.33  2005/09/11 07:19:22  pfusik
fixed file selector which I broke yesterday;
use Util_realloc() instead of realloc(); fixed a warning

Revision 1.32  2005/09/10 12:37:25  pfusik
char * -> const char *; Util_splitpath() and Util_catpath()

Revision 1.31  2005/09/07 22:00:29  pfusik
shorten the messages to fit on screen

Revision 1.30  2005/09/06 22:58:29  pfusik
improved file selector; fixed MSVC warnings

Revision 1.29  2005/09/04 18:16:18  pfusik
don't hide ATR/XFD file extensions in the file selector

Revision 1.28  2005/08/27 10:36:07  pfusik
MSVC declares getcwd() in <direct.h>

Revision 1.27  2005/08/24 21:03:41  pfusik
use stricmp() if there's no strcasecmp()

Revision 1.26  2005/08/21 17:40:53  pfusik
DO_DIR -> HAVE_OPENDIR

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
