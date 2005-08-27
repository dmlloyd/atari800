/*
 * memory.c - RAM memory emulation
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
#include <stdlib.h>
#include <string.h>

#include "atari.h"
#include "antic.h"
#include "cpu.h"
#include "cartridge.h"
#include "gtia.h"
#include "log.h"
#include "memory.h"
#include "pbi.h"
#include "pia.h"
#include "pokey.h"
#include "rt-config.h"
#ifndef BASIC
#include "statesav.h"
#endif

UBYTE memory[65536];
#ifndef PAGED_ATTRIB
UBYTE attrib[65536];
#else
rdfunc readmap[256];
wrfunc writemap[256];

typedef struct map_save {
	int     code;
	rdfunc  rdptr;
	wrfunc  wrptr;
} map_save;

void ROM_PutByte(UWORD addr, UBYTE value)
{
}

map_save save_map[2] = {
	{0, NULL, NULL},          /* RAM */
	{1, NULL, ROM_PutByte}    /* ROM */
};
#endif

static UBYTE under_atarixl_os[16384];
static UBYTE under_atari_basic[8192];
static UBYTE *atarixe_memory = NULL;
static ULONG atarixe_memory_size = 0;

int have_basic = FALSE; /* Atari BASIC image has been successfully read (Atari 800 only) */

extern const UBYTE *antic_xe_ptr;	/* Separate ANTIC access to extended memory */

static void AllocXEMemory(void)
{
	if (ram_size > 64) {
		/* don't count 64 KB of base memory */
		/* count number of 16 KB banks, add 1 for saving base memory 0x4000-0x7fff */
		ULONG size = (1 + (ram_size - 64) / 16) * 16384;
		if (size != atarixe_memory_size) {
			if (atarixe_memory != NULL)
				free(atarixe_memory);
			atarixe_memory = (UBYTE *) malloc(size);
			if (atarixe_memory == NULL) {
				Aprint("MEMORY_InitialiseMachine: Out of memory! Switching to 64 KB mode");
				atarixe_memory_size = 0;
				ram_size = 64;
			}
			else {
				atarixe_memory_size = size;
				memset(atarixe_memory, 0, size);
			}
		}
	}
	/* atarixe_memory not needed, free it */
	else if (atarixe_memory != NULL) {
		free(atarixe_memory);
		atarixe_memory = NULL;
		atarixe_memory_size = 0;
	}
}

void MEMORY_InitialiseMachine(void)
{
	antic_xe_ptr = NULL;
	switch (machine_type) {
	case MACHINE_OSA:
	case MACHINE_OSB:
		memcpy(memory + 0xd800, atari_os, 0x2800);
		Atari800_PatchOS();
		dFillMem(0x0000, 0x00, ram_size * 1024 - 1);
		SetRAM(0x0000, ram_size * 1024 - 1);
		if (ram_size < 52) {
			dFillMem(ram_size * 1024, 0xff, 0xd000 - ram_size * 1024);
			SetROM(ram_size * 1024, 0xcfff);
		}
#ifndef PAGED_ATTRIB
		SetHARDWARE(0xd000, 0xd7ff);
#else
		readmap[0xd0] = GTIA_GetByte;
		readmap[0xd1] = PBI_GetByte;
		readmap[0xd2] = POKEY_GetByte;
		readmap[0xd3] = PIA_GetByte;
		readmap[0xd4] = ANTIC_GetByte;
		readmap[0xd5] = CART_GetByte;
		readmap[0xd6] = PBIM1_GetByte;
		readmap[0xd7] = PBIM2_GetByte;
		writemap[0xd0] = GTIA_PutByte;
		writemap[0xd1] = PBI_PutByte;
		writemap[0xd2] = POKEY_PutByte;
		writemap[0xd3] = PIA_PutByte;
		writemap[0xd4] = ANTIC_PutByte;
		writemap[0xd5] = CART_PutByte;
		writemap[0xd6] = PBIM1_PutByte;
		writemap[0xd7] = PBIM2_PutByte;
#endif
		SetROM(0xd800, 0xffff);
		break;
	case MACHINE_XLXE:
		memcpy(memory + 0xc000, atari_os, 0x4000);
		Atari800_PatchOS();
		if (ram_size == 16) {
			dFillMem(0x0000, 0x00, 0x4000);
			SetRAM(0x0000, 0x3fff);
			dFillMem(0x4000, 0xff, 0x8000);
			SetROM(0x4000, 0xcfff);
		} else {
			dFillMem(0x0000, 0x00, 0xc000);
			SetRAM(0x0000, 0xbfff);
			SetROM(0xc000, 0xcfff);
		}
#ifndef PAGED_ATTRIB
		SetHARDWARE(0xd000, 0xd7ff);
#else
		readmap[0xd0] = GTIA_GetByte;
		readmap[0xd1] = PBI_GetByte;
		readmap[0xd2] = POKEY_GetByte;
		readmap[0xd3] = PIA_GetByte;
		readmap[0xd4] = ANTIC_GetByte;
		readmap[0xd5] = CART_GetByte;
		readmap[0xd6] = PBIM1_GetByte;
		readmap[0xd7] = PBIM2_GetByte;
		writemap[0xd0] = GTIA_PutByte;
		writemap[0xd1] = PBI_PutByte;
		writemap[0xd2] = POKEY_PutByte;
		writemap[0xd3] = PIA_PutByte;
		writemap[0xd4] = ANTIC_PutByte;
		writemap[0xd5] = CART_PutByte;
		writemap[0xd6] = PBIM1_PutByte;
		writemap[0xd7] = PBIM2_PutByte;
#endif
		SetROM(0xd800, 0xffff);
		break;
	case MACHINE_5200:
		memcpy(memory + 0xf800, atari_os, 0x800);
		dFillMem(0x0000, 0x00, 0xf800);
		SetRAM(0x0000, 0x3fff);
		SetROM(0x4000, 0xffff);
#ifndef PAGED_ATTRIB
		SetHARDWARE(0xc000, 0xc0ff);	/* 5200 GTIA Chip */
		SetHARDWARE(0xd400, 0xd4ff);	/* 5200 ANTIC Chip */
		SetHARDWARE(0xe800, 0xe8ff);	/* 5200 POKEY Chip */
		SetHARDWARE(0xeb00, 0xebff);	/* 5200 POKEY Chip */
#else
		readmap[0xc0] = GTIA_GetByte;
		readmap[0xd4] = ANTIC_GetByte;
		readmap[0xe8] = POKEY_GetByte;
		readmap[0xeb] = POKEY_GetByte;
		writemap[0xc0] = GTIA_PutByte;
		writemap[0xd4] = ANTIC_PutByte;
		writemap[0xe8] = POKEY_PutByte;
		writemap[0xeb] = POKEY_PutByte;
#endif
		break;
	}
	AllocXEMemory();
	Coldstart();
}

#ifndef BASIC

void MemStateSave(UBYTE SaveVerbose)
{
	SaveUBYTE(&memory[0], 65536);
#ifndef PAGED_ATTRIB
	SaveUBYTE(&attrib[0], 65536);
#else
#warning state save not working yet
#endif

	if (machine_type == MACHINE_XLXE) {
		if (SaveVerbose != 0)
			SaveUBYTE(&atari_basic[0], 8192);
		SaveUBYTE(&under_atari_basic[0], 8192);

		if (SaveVerbose != 0)
			SaveUBYTE(&atari_os[0], 16384);
		SaveUBYTE(&under_atarixl_os[0], 16384);
	}

	if (ram_size > 64) {
		SaveUBYTE(&atarixe_memory[0], atarixe_memory_size);
		/* a hack that makes state files compatible with previous versions:
           for 130 XE there's written 192 KB of unused data */
		if (ram_size == 128) {
			UBYTE buffer[256];
			int i;
			memset(buffer, 0, 256);
			for (i = 0; i < 192 * 4; i++)
				SaveUBYTE(&buffer[0], 256);
		}
	}

}

void MemStateRead(UBYTE SaveVerbose)
{
	ReadUBYTE(&memory[0], 65536);
#ifndef PAGED_ATTRIB
	ReadUBYTE(&attrib[0], 65536);
#else
#warning state save not working yet
#endif

	if (machine_type == MACHINE_XLXE) {
		if (SaveVerbose != 0)
			ReadUBYTE(&atari_basic[0], 8192);
		ReadUBYTE(&under_atari_basic[0], 8192);

		if (SaveVerbose != 0)
			ReadUBYTE(&atari_os[0], 16384);
		ReadUBYTE(&under_atarixl_os[0], 16384);
	}

	antic_xe_ptr = NULL;
	AllocXEMemory();
	if (ram_size > 64) {
		ReadUBYTE(&atarixe_memory[0], atarixe_memory_size);
		/* a hack that makes state files compatible with previous versions:
           for 130 XE there's written 192 KB of unused data */
		if (ram_size == 128) {
			UBYTE buffer[256];
			int i;
			for (i = 0; i < 192 * 4; i++)
				ReadUBYTE(&buffer[0], 256);
		}
	}

}

#endif /* BASIC */

void CopyFromMem(UWORD from, UBYTE *to, int size)
{
	while (--size >= 0) {
		*to++ = GetByte(from);
		from++;
	}
}

void CopyToMem(const UBYTE *from, UWORD to, int size)
{
	while (--size >= 0) {
		PutByte(to, *from);
		from++;
		to++;
	}
}

/*
 * Returns non-zero, if Atari BASIC is disabled by given PORTB output.
 * Normally BASIC is disabled by setting bit 1, but it's also disabled
 * when using 576K and 1088K memory expansions, where bit 1 is used
 * for selecting extended memory bank number.
 */
static int basic_disabled(UBYTE portb)
{
	return (portb & 0x02) != 0
	 || ((portb & 0x10) == 0 && (ram_size == 576 || ram_size == 1088));
}

/* Note: this function is only for XL/XE! */
void MEMORY_HandlePORTB(UBYTE byte, UBYTE oldval)
{
	/* Switch XE memory bank in 0x4000-0x7fff */
	if (ram_size > 64) {
		int bank = 0;
		/* bank = 0 : base RAM */
		/* bank = 1..64 : extended RAM */
		if ((byte & 0x10) == 0)
			switch (ram_size) {
			case 128:
				bank = ((byte & 0x0c) >> 2) + 1;
				break;
			case RAM_320_RAMBO:
				bank = (((byte & 0x0c) + ((byte & 0x60) >> 1)) >> 2) + 1;
				break;
			case RAM_320_COMPY_SHOP:
				bank = (((byte & 0x0c) + ((byte & 0xc0) >> 2)) >> 2) + 1;
				break;
			case 576:
				bank = (((byte & 0x0e) + ((byte & 0x60) >> 1)) >> 1) + 1;
				break;
			case 1088:
				bank = (((byte & 0x0e) + ((byte & 0xe0) >> 1)) >> 1) + 1;
				break;
			}
		/* Note: in Compy Shop bit 5 (ANTIC access) disables Self Test */
		if (selftest_enabled && (bank != xe_bank || (ram_size == RAM_320_COMPY_SHOP && (byte & 0x20) == 0))) {
			/* Disable Self Test ROM */
			memcpy(memory + 0x5000, under_atarixl_os + 0x1000, 0x800);
			SetRAM(0x5000, 0x57ff);
			selftest_enabled = FALSE;
		}
		if (bank != xe_bank) {
			memcpy(atarixe_memory + (xe_bank << 14), memory + 0x4000, 16384);
			memcpy(memory + 0x4000, atarixe_memory + (bank << 14), 16384);
			xe_bank = bank;
		}
		if (ram_size == 128 || ram_size == RAM_320_COMPY_SHOP)
			switch (byte & 0x30) {
			case 0x20:	/* ANTIC: base, CPU: extended */
				antic_xe_ptr = atarixe_memory;
				break;
			case 0x10:	/* ANTIC: extended, CPU: base */
				if (ram_size == 128)
					antic_xe_ptr = atarixe_memory + ((((byte & 0x0c) >> 2) + 1) << 14);
				else	/* 320 Compy Shop */
					antic_xe_ptr = atarixe_memory + (((((byte & 0x0c) + ((byte & 0xc0) >> 2)) >> 2) + 1) << 14);
				break;
			default:	/* ANTIC same as CPU */
				antic_xe_ptr = NULL;
				break;
			}
	}

	/* Enable/disable OS ROM in 0xc000-0xcfff and 0xd800-0xffff */
	if ((oldval ^ byte) & 0x01) {
		if (byte & 0x01) {
			/* Enable OS ROM */
			if (ram_size > 48) {
				memcpy(under_atarixl_os, memory + 0xc000, 0x1000);
				memcpy(under_atarixl_os + 0x1800, memory + 0xd800, 0x2800);
				SetROM(0xc000, 0xcfff);
				SetROM(0xd800, 0xffff);
			}
			memcpy(memory + 0xc000, atari_os, 0x1000);
			memcpy(memory + 0xd800, atari_os + 0x1800, 0x2800);
			Atari800_PatchOS();
		}
		else {
			/* Disable OS ROM */
			if (ram_size > 48) {
				memcpy(memory + 0xc000, under_atarixl_os, 0x1000);
				memcpy(memory + 0xd800, under_atarixl_os + 0x1800, 0x2800);
				SetRAM(0xc000, 0xcfff);
				SetRAM(0xd800, 0xffff);
			} else {
				dFillMem(0xc000, 0xff, 0x1000);
				dFillMem(0xd800, 0xff, 0x2800);
			}
			/* When OS ROM is disabled we also have to disable Self Test - Jindroush */
			if (selftest_enabled) {
				if (ram_size > 20) {
					memcpy(memory + 0x5000, under_atarixl_os + 0x1000, 0x800);
					SetRAM(0x5000, 0x57ff);
				}
				else
					dFillMem(0x5000, 0xff, 0x800);
				selftest_enabled = FALSE;
			}
		}
	}

	/* Enable/disable BASIC ROM in 0xa000-0xbfff */
	if (!cartA0BF_enabled) {
		/* BASIC is disabled if bit 1 set or accessing extended 576K or 1088K memory */
		int now_disabled = basic_disabled(byte);
		if (basic_disabled(oldval) != now_disabled) {
			if (now_disabled) {
				/* Disable BASIC ROM */
				if (ram_size > 40) {
					memcpy(memory + 0xa000, under_atari_basic, 0x2000);
					SetRAM(0xa000, 0xbfff);
				}
				else
					dFillMem(0xa000, 0xff, 0x2000);
			}
			else {
				/* Enable BASIC ROM */
				if (ram_size > 40) {
					memcpy(under_atari_basic, memory + 0xa000, 0x2000);
					SetROM(0xa000, 0xbfff);
				}
				memcpy(memory + 0xa000, atari_basic, 0x2000);
			}
		}
	}

	/* Enable/disable Self Test ROM in 0x5000-0x57ff */
	if (byte & 0x80) {
		if (selftest_enabled) {
			/* Disable Self Test ROM */
			if (ram_size > 20) {
				memcpy(memory + 0x5000, under_atarixl_os + 0x1000, 0x800);
				SetRAM(0x5000, 0x57ff);
			}
			else
				dFillMem(0x5000, 0xff, 0x800);
			selftest_enabled = FALSE;
		}
	}
	else {
		/* We can enable Self Test only if the OS ROM is enabled */
		/* and we're not accessing extended 320K Compy Shop or 1088K memory */
		/* Note: in Compy Shop bit 5 (ANTIC access) disables Self Test */
		if (!selftest_enabled && (byte & 0x01)
		&& !((byte & 0x30) != 0x30 && ram_size == RAM_320_COMPY_SHOP)
		&& !((byte & 0x10) == 0 && ram_size == 1088)) {
			/* Enable Self Test ROM */
			if (ram_size > 20) {
				memcpy(under_atarixl_os + 0x1000, memory + 0x5000, 0x800);
				SetROM(0x5000, 0x57ff);
			}
			memcpy(memory + 0x5000, atari_os + 0x1000, 0x800);
			selftest_enabled = TRUE;
		}
	}
}

static int cart809F_enabled = FALSE;
int cartA0BF_enabled = FALSE;
static UBYTE under_cart809F[8192];
static UBYTE under_cartA0BF[8192];

void Cart809F_Disable(void)
{
	if (cart809F_enabled) {
		if (ram_size > 32) {
			memcpy(memory + 0x8000, under_cart809F, 0x2000);
			SetRAM(0x8000, 0x9fff);
		}
		else
			dFillMem(0x8000, 0xff, 0x2000);
		cart809F_enabled = FALSE;
	}
}

void Cart809F_Enable(void)
{
	if (!cart809F_enabled) {
		if (ram_size > 32) {
			memcpy(under_cart809F, memory + 0x8000, 0x2000);
			SetROM(0x8000, 0x9fff);
		}
		cart809F_enabled = TRUE;
	}
}

void CartA0BF_Disable(void)
{
	if (cartA0BF_enabled) {
		/* No BASIC if not XL/XE or bit 1 of PORTB set */
		/* or accessing extended 576K or 1088K memory */
		if ((machine_type != MACHINE_XLXE) || basic_disabled((UBYTE) (PORTB | PORTB_mask))) {
			if (ram_size > 40) {
				memcpy(memory + 0xa000, under_cartA0BF, 0x2000);
				SetRAM(0xa000, 0xbfff);
			}
			else
				dFillMem(0xa000, 0xff, 0x2000);
		}
		else
			memcpy(memory + 0xa000, atari_basic, 0x2000);
		cartA0BF_enabled = FALSE;
		if (machine_type == MACHINE_XLXE) {
			TRIG[3] = 0;
			if (GRACTL & 4)
				TRIG_latch[3] = 0;
		}
	}
}

void CartA0BF_Enable(void)
{
	if (!cartA0BF_enabled) {
		/* No BASIC if not XL/XE or bit 1 of PORTB set */
		/* or accessing extended 576K or 1088K memory */
		if (ram_size > 40 && ((machine_type != MACHINE_XLXE) || (PORTB & 0x02)
		|| ((PORTB & 0x10) == 0 && (ram_size == 576 || ram_size == 1088)))) {
			/* Back-up 0xa000-0xbfff RAM */
			memcpy(under_cartA0BF, memory + 0xa000, 0x2000);
			SetROM(0xa000, 0xbfff);
		}
		cartA0BF_enabled = TRUE;
		if (machine_type == MACHINE_XLXE)
			TRIG[3] = 1;
	}
}

void get_charset(UBYTE *cs)
{
	switch (machine_type) {
	case MACHINE_OSA:
	case MACHINE_OSB:
		memcpy(cs, memory + 0xe000, 1024);
		break;
	case MACHINE_XLXE:
		memcpy(cs, atari_os + 0x2000, 1024);
		break;
	case MACHINE_5200:
		memcpy(cs, memory + 0xf800, 1024);
		break;
	}
}

/*
$Log: memory.c,v $
Revision 1.13  2005/08/27 10:39:58  pfusik
cast the result of malloc()

Revision 1.12  2005/08/22 20:54:49  pfusik
initialize have_basic just in case

Revision 1.11  2005/08/21 15:44:34  pfusik
CopyFromMem and CopyToMem (both used by SIO) now work with hardware registers

Revision 1.10  2005/08/18 23:31:30  pfusik
minor clean up

Revision 1.9  2005/08/17 22:33:19  pfusik
removed PILL

Revision 1.8  2005/08/16 23:06:41  pfusik
#include "config.h" before system headers

Revision 1.7  2005/08/15 17:21:53  pfusik
SetROM/SetRAM macros for PAGED_ATTRIB

Revision 1.6  2005/08/14 08:41:34  pfusik
#include "config.h" for BASIC and PAGED_ATTRIB definitions

Revision 1.5  2005/08/10 19:52:03  pfusik
no state files in BASIC version

Revision 1.4  2003/03/07 11:22:38  pfusik
PORTB_handler() -> MEMORY_HandlePORTB()

Revision 1.3  2003/02/24 09:33:02  joy
header cleanup

Revision 1.2  2003/01/27 13:14:53  joy
Jason's changes: either PAGED_ATTRIB support (mostly), or just clean up.

Revision 1.18  2002/07/14 13:32:01  pfusik
separate ANTIC access to extended memory for 130 XE and 320 Compy Shop

Revision 1.17  2002/07/14 13:25:07  pfusik
emulation of 576K and 1088K RAM machines

Revision 1.16  2002/07/04 12:40:57  pfusik
emulation of 16K RAM machines: 400 and 600XL

Revision 1.15  2001/10/26 05:43:00  fox
made 130 XE state files compatible with previous versions

Revision 1.14  2001/10/08 11:40:48  joy
neccessary include for compiling with DEBUG defined (see line 200)

Revision 1.13  2001/10/03 16:42:50  fox
rewritten escape codes handling

Revision 1.12  2001/10/01 17:13:26  fox
Poke -> dPutByte

Revision 1.11  2001/09/17 18:19:50  fox
malloc/free atarixe_memory, enable_c000_ram -> ram_size = 52

Revision 1.10  2001/09/17 18:12:08  fox
machine, mach_xlxe, Ram256, os, default_system -> machine_type, ram_size

Revision 1.9  2001/09/17 07:33:07  fox
Initialise_Atari... functions moved to atari.c

Revision 1.8  2001/09/09 08:39:01  fox
read Atari BASIC for Atari 800

Revision 1.7  2001/08/16 23:28:57  fox
deleted CART_Remove() in Initialise_Atari*, so auto-switching to 5200 mode
when inserting a 5200 cartridge works

Revision 1.6  2001/07/20 20:15:35  fox
rewritten to support the new cartridge module

Revision 1.3  2001/03/25 06:57:35  knik
open() replaced by fopen()

Revision 1.2  2001/03/18 06:34:58  knik
WIN32 conditionals removed

*/
