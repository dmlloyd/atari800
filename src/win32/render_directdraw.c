#include "render_directdraw.h"
#include "main.h"
#include "colours.h"
#include "screen_win32.h"
#include "screen.h"
#include <ddraw.h>
#include <stdio.h>

#define MAX_CLR 0x100
static PALETTEENTRY pal[MAX_CLR]; /* palette */

static LPDIRECTDRAW4 lpDD = NULL;
static LPDIRECTDRAWSURFACE4 lpDDSPrimary = NULL;
static LPDIRECTDRAWSURFACE4 lpDDSBack = NULL;
static LPDIRECTDRAWSURFACE4 lpDDSsrc = NULL;
static LPDIRECTDRAWPALETTE lpDDPal = NULL;

static int linesize = 0;
static int scrwidth = SCREENWIDTH;
static int scrheight = SCREENHEIGHT;
static UBYTE *scraddr = NULL;

/* Platform-specific function to update the palette if it changed */
void PLATFORM_PaletteUpdate(void)
{
	if(lpDDPal != NULL) {
		int i;
		for (i = 0; i < MAX_CLR; i++) {
			palette(i, Colours_GetR(i), Colours_GetG(i), Colours_GetB(i));
		}
		palupd(0, MAX_CLR);
	}
}

static int initFail(HWND hwnd, const char *func, HRESULT hr)
{
	char txt[256];
	sprintf(txt, "DirectDraw Init FAILED: %s returned 0x%x", func, (unsigned int)hr);
	MessageBox(hwnd, txt, myname, MB_OK);
	groff();
	DestroyWindow(hwnd);
	return 1;
}

void palupd(int beg, int cnt)
{
	IDirectDrawPalette_SetEntries(lpDDPal, 0, beg, cnt, pal);
}

void palette(int ent, UBYTE r, UBYTE g, UBYTE b)
{
	if (ent >= MAX_CLR)
		return;
	pal[ent].peRed = r;
	pal[ent].peGreen = g;
	pal[ent].peBlue = b;
	pal[ent].peFlags = 0;
}

/* Platform-specific function to update the palette if it changed */
void Atari_PaletteUpdate(void)
{
	if(lpDDPal != NULL) {
		int i;
		for (i = 0; i < MAX_CLR; i++) {
			palette(i, Colours_GetR(i), Colours_GetG(i), Colours_GetB(i));
		}
		palupd(0, MAX_CLR);
	}
}

int startupdirectdraw(BOOL bltgfx, INT width)
{
	DDSURFACEDESC2 ddsd;
	DDSCAPS2 ddscaps;
	HRESULT ddrval;
	int i;

	if (width > 0) {
		scrwidth = width;
		scrheight = width * 3 / 4;
	}

	ddrval = DirectDrawCreate(NULL, (void *) &lpDD, NULL);
	if (FAILED(ddrval))
		return initFail(hWndMain, "DirectDrawCreate", ddrval);
	ddrval = IDirectDraw4_QueryInterface(lpDD, &IID_IDirectDraw4, (void *) &lpDD);
	if (FAILED(ddrval))
		return initFail(hWndMain, "QueryInterface", ddrval);
	ddrval = IDirectDraw4_SetCooperativeLevel(lpDD, hWndMain, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
	if (FAILED(ddrval))
		return initFail(hWndMain, "SetCooperativeLevel", ddrval);

	if (bltgfx) {
		memset(&ddsd, 0, sizeof(ddsd));
		ddsd.dwSize = sizeof(ddsd);
		ddrval = IDirectDraw4_GetDisplayMode(lpDD, &ddsd);
		if (FAILED(ddrval))
			return initFail(hWndMain, "GetDisplayMode", ddrval);
		ddrval = IDirectDraw4_SetDisplayMode(lpDD, ddsd.dwWidth, ddsd.dwHeight, 32, 0, 0);
	}
	else {
		ddrval = IDirectDraw4_SetDisplayMode(lpDD, scrwidth, scrheight, 8, 0, 0);
	}
	if (FAILED(ddrval)) {
		if ((ddrval == DDERR_INVALIDMODE || ddrval == DDERR_UNSUPPORTED) && !bltgfx && width != 640) {
			/* 320x240 results in DDERR_INVALIDMODE on my Win98SE / Radeon 9000 */
			/* 320x240 results in DDERR_UNSUPPORTED on my WinXP / Toshiba laptop */
			MessageBox(hWndMain,
				"DirectDraw does not support the requested display mode.\n"
				"Try running with \"-blt\" or \"-width 640\" on the command line.",
				myname, MB_OK);
			groff();
			DestroyWindow(hWndMain);
			return 1;
		}
		return initFail(hWndMain, "SetDisplayMode", ddrval);
	}

	memset(&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	ddsd.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
	ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_FLIP | DDSCAPS_COMPLEX;
	ddsd.dwBackBufferCount = 1;
	ddrval = IDirectDraw4_CreateSurface(lpDD, &ddsd, &lpDDSPrimary, NULL);
	if (FAILED(ddrval))
		return initFail(hWndMain, "CreateSurface", ddrval);
	memset(&ddscaps, 0, sizeof(ddscaps));
	ddscaps.dwCaps = DDSCAPS_BACKBUFFER;
	ddrval = IDirectDrawSurface4_GetAttachedSurface(lpDDSPrimary, &ddscaps, &lpDDSBack);
	if (FAILED(ddrval))
		return initFail(hWndMain, "GetAttachedSurface", ddrval);

	if (bltgfx) {
		ddrval = IDirectDraw4_GetDisplayMode(lpDD, &ddsd);
		if (FAILED(ddrval))
			return initFail(hWndMain, "GetDisplayMode", ddrval);

		ddsd.dwSize = sizeof(ddsd);
		ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
		ddsd.dwWidth = 336;
		ddsd.dwHeight = 252;
		ddsd.ddsCaps.dwCaps = DDSCAPS_VIDEOMEMORY;
		ddrval = IDirectDraw4_CreateSurface(lpDD, &ddsd, &lpDDSsrc, NULL);
		if (FAILED(ddrval))
			return initFail(hWndMain, "CreateSurface", ddrval);

		ddrval = IDirectDrawSurface4_Lock(lpDDSsrc, NULL, &ddsd, DDLOCK_WAIT | DDLOCK_WRITEONLY, NULL);
		if (FAILED(ddrval))
			return initFail(hWndMain, "Lock", ddrval);

		memset(ddsd.lpSurface, 0, ddsd.lPitch * ddsd.dwHeight);

		ddrval = IDirectDrawSurface4_Unlock(lpDDSsrc, NULL);
	}
	else {
		for (i = 0; i < MAX_CLR; i++)
			palette(i, Colours_GetR(i), Colours_GetG(i), Colours_GetB(i));
		IDirectDraw4_CreatePalette(lpDD, DDPCAPS_8BIT, pal, &lpDDPal, NULL);
		if (lpDDPal)
			IDirectDrawSurface4_SetPalette(lpDDSPrimary, lpDDPal);
	}
	return 0;
}

void shutdowndirectdraw(void)
{
	if (lpDD != NULL) {
		if (lpDDSsrc != NULL) {
			IDirectDrawSurface4_Release(lpDDSsrc);
			lpDDSsrc = NULL;
		}
		if (lpDDSPrimary != NULL) {
			IDirectDrawSurface4_Release(lpDDSPrimary);
			lpDDSPrimary = NULL;
		}
		if (lpDDPal != NULL) {
			IDirectDrawPalette_Release(lpDDPal);
			lpDDPal = NULL;
		}
		IDirectDraw4_Release(lpDD);
		lpDD = NULL;
	}		
}

void refreshv_directdraw(UBYTE *scr_ptr, BOOL bltgfx)
{
	DDSURFACEDESC2 desc0;
	int err;
	int x, y;
	UBYTE *srcb;
	ULONG *srcl;
	ULONG *dst;
	int h, w;
	DDBLTFX ddbltfx;

	desc0.dwSize = sizeof(desc0);
	err = IDirectDrawSurface4_Lock(bltgfx ? lpDDSsrc : lpDDSBack,
			NULL, &desc0, DDLOCK_WRITEONLY | DDLOCK_WAIT ,NULL);
	if (err == DD_OK) {
		linesize = desc0.lPitch;
		scrwidth = desc0.dwWidth;
		scrheight = desc0.dwHeight;
		scraddr = (UBYTE *) desc0.lpSurface + (bltgfx ? linesize * 6 : 0);

		if (bltgfx) {
			for (y = 0; y < Screen_HEIGHT; y++) {
				dst = (ULONG *) (scraddr + y * linesize);
				srcb = scr_ptr + y * Screen_WIDTH;
				for (x = 0; x < scrwidth; x++)
					*dst++ = Colours_table[*srcb++];
			}
		}
		else {
			w = (scrwidth - 336) / 2;
			h = (scrheight - Screen_HEIGHT) / 2;
			if (w > 0)
				scraddr += w;
			else if (w < 0)
				scr_ptr -= w;
			if (h > 0)
				scraddr += linesize * h;
			for (y = 0; y < Screen_HEIGHT; y++) {
				dst = (ULONG *) (scraddr + y * linesize);
				srcl = (ULONG *) (scr_ptr + y * Screen_WIDTH);
				for (x = (w >= 0) ? (336 >> 2) : (scrwidth >> 2); x > 0; x--)
					*dst++ = *srcl++;
			}
		}

		IDirectDrawSurface4_Unlock(bltgfx ? lpDDSsrc : lpDDSBack, NULL);
		linesize = 0;
		scrwidth = 0;
		scrheight = 0;
		scraddr = 0;
	}
	else if (err == DDERR_SURFACELOST)
		err = IDirectDrawSurface4_Restore(bltgfx ? lpDDSsrc : lpDDSBack);
	else {
		char txt[256];
		sprintf(txt, "DirectDraw error 0x%x", err);
		MessageBox(hWndMain, txt, myname, MB_OK);
		/* printf("error: %x\n", err); */
		exit(1);
	}

	if (bltgfx) {
		memset(&ddbltfx, 0, sizeof(ddbltfx));
		ddbltfx.dwSize = sizeof(ddbltfx);
		err = IDirectDrawSurface4_Blt(lpDDSBack, NULL, lpDDSsrc,
				NULL, DDBLT_WAIT, &ddbltfx);
		if (err == DDERR_SURFACELOST)
			err = IDirectDrawSurface4_Restore(lpDDSBack);
	}

#if (SHOWFRAME > 0)
	palette(0, 0x20, 0x20, 0);
	palupd(CLR_BACK, 1);
#endif
	err = IDirectDrawSurface4_Flip(lpDDSPrimary, NULL, DDFLIP_WAIT);
	/* err = IDirectDrawSurface3_Flip(lpDDSPrimary, NULL, 0); */
	if (err == DDERR_SURFACELOST)
		err = IDirectDrawSurface4_Restore(lpDDSPrimary);
#if (SHOWFRAME > 0)
	palette(0, 0x0, 0x20, 0x20);
	palupd(CLR_BACK, 1);
#endif
#if (SHOWFRAME > 0)
	palette(0, 0x0, 0x0, 0x0);
	palupd(CLR_BACK, 1);
#endif
}
