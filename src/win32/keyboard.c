/* (C) 2000  Krzysztof Nikiel */
/* $Id: keyboard.c,v 1.1 2001/03/18 07:56:48 knik Exp $ */
#define DIRECTINPUT_VERSION	    0x0500

#include <windows.h>
#include <dinput.h>
#include "atari.h"
#include "main.h"
#include "keyboard.h"

#define KEYBUFSIZE 0x40

static LPDIRECTINPUTDEVICE2 dikb0 = NULL;
int pause_hit;

int kbcode;
UBYTE kbhits[KBCODES];

int kbreacquire(void)
{
  if (!dikb0)
    return 1;
  if (IDirectInputDevice_Acquire(dikb0) >= 0)
    return 0;
  else
    return 1;
}

int prockb(void)
{
  DIDEVICEOBJECTDATA que[KEYBUFSIZE];
  DWORD dwEvents;
  int i;
  HRESULT hRes;

  dwEvents = KEYBUFSIZE;
  hRes = IDirectInputDevice_GetDeviceData(dikb0,
					  sizeof(DIDEVICEOBJECTDATA),
					  que, &dwEvents, 0);
  if (hRes != DI_OK)
    {
      if ((hRes == DIERR_INPUTLOST))
	kbreacquire();
      return 1;
    }

  for (i = 0; i < dwEvents; i++)
    {
#if 0
      printf("%02x(%02x)\n", que[i].dwOfs, que[i].dwData);
#endif
      if (que[i].dwOfs >= KBCODES)
	continue;
      if (que[i].dwOfs == DIK_PAUSE)
	{
	  if (que[i].dwData)
	    pause_hit = 1;
	  continue;
	}
      if (que[i].dwData)
	kbhits[kbcode = que[i].dwOfs] = 1;
      else
	{
	  kbhits[kbcode = que[i].dwOfs] = 0;
	  kbcode |= 0x100;
	}
    }
  return 0;
}

void uninitinput(void)
{
  if (dikb0)
    {
      IDirectInputDevice_Unacquire(dikb0);
      IDirectInputDevice_Release(dikb0);
      dikb0 = NULL;
    }
}

HRESULT
SetDIDwordProperty(LPDIRECTINPUTDEVICE pdev, REFGUID guidProperty,
		   DWORD dwObject, DWORD dwHow, DWORD dwValue)
{
  DIPROPDWORD dipdw;

  dipdw.diph.dwSize = sizeof(dipdw);
  dipdw.diph.dwHeaderSize = sizeof(dipdw.diph);
  dipdw.diph.dwObj = dwObject;
  dipdw.diph.dwHow = dwHow;
  dipdw.dwData = dwValue;

  return pdev->lpVtbl->SetProperty(pdev, guidProperty, &dipdw.diph);
}

static int initkb(LPDIRECTINPUT pdi)
{
  LPDIRECTINPUTDEVICE pdev;
  HRESULT hRes;

  if (IDirectInput_CreateDevice(pdi,
				&GUID_SysKeyboard, &pdev, NULL) != DI_OK)
    {
      return 1;
    }
  if (IDirectInputDevice_SetDataFormat(pdev, &c_dfDIKeyboard) != DI_OK)
    {
      IDirectInputDevice_Release(pdev);
      return 1;
    }
  if (IDirectInputDevice_SetCooperativeLevel(pdev, hWndMain,
					     DISCL_NONEXCLUSIVE |
      DISCL_FOREGROUND) != DI_OK)
    {
      IDirectInputDevice_Release(pdev);
      return 1;
    }
  if (SetDIDwordProperty(pdev, DIPROP_BUFFERSIZE, 0, DIPH_DEVICE,
      KEYBUFSIZE) != DI_OK)
    {
      IDirectInputDevice_Release(pdev);
      return 1;
    }

  hRes = pdev->lpVtbl->QueryInterface(pdev, &IID_IDirectInputDevice2,
				      (LPVOID *) & dikb0);
  if (hRes < 0)
    return 1;
  IDirectInputDevice_Release(pdev);
  kbreacquire();
  return 0;
}

int initinput(void)
{
  int i;
  LPDIRECTINPUT pdi;

  if (DirectInputCreate(myInstance, 0x0300, &pdi, NULL) != DI_OK)
    {
      return 1;
    }
  i = initkb(pdi);
  IDirectInput_Release(pdi);
  if (i)
    return i;
  return 0;
}

void clearkb(void)
{
  int i;
  for (i = 0; i < KBCODES; i++)
    kbhits[i] = 0;
  pause_hit = 0;
  kbcode = 0;
}

/*
$Log: keyboard.c,v $
Revision 1.1  2001/03/18 07:56:48  knik
win32 port

*/
