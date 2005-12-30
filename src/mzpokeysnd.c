/*
 * mzpokeysnd.c - POKEY sound chip emulation, v1.6
 *
 * Copyright (C) 2002 Michael Borisov
 * Copyright (C) 2002-2005 Atari800 development team (see DOC/CREDITS)
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
#include <stdlib.h>
#include <math.h>

#ifdef ASAP /* external project, see http://asap.sf.net */
#include "asap_internal.h"
#else
#include "atari.h"
#endif
#include "mzpokeysnd.h"
#include "pokeysnd.h"
#include "remez.h"

#define SND_FILTER_SIZE  2048

#define NPOKEYS 2

/* Volume only emulations declarations */
#ifdef VOL_ONLY_SOUND

#define	SAMPBUF_MAX	2000
extern int	sampbuf_val[SAMPBUF_MAX];	/* volume values */
extern int	sampbuf_cnt[SAMPBUF_MAX];	/* relative start time */
extern int	sampbuf_ptr;                    /* pointer to sampbuf */
extern int	sampbuf_rptr;                   /* pointer to read from sampbuf */
extern int	sampbuf_last;                   /* last absolute time */
extern int	sampbuf_AUDV[4 * MAXPOKEYS];	/* prev. channel volume */
extern int	sampbuf_lastval;		/* last volume */
extern int	sampout;			/* last out volume */
extern uint16	samp_freq;
extern int	samp_consol_val;		/* actual value of console sound */
#endif  /* VOL_ONLY_SOUND */

/* M_PI was not defined in MSVC headers */
#ifndef M_PI
# define M_PI 3.141592653589793
#endif

static unsigned int num_cur_pokeys = 0;

/* Filter */
static unsigned sample_rate; /* Hz */
static unsigned pokey_frq; /* Hz - for easier resampling */
static int filter_size;
static double filter_data[SND_FILTER_SIZE];
static unsigned audible_frq;

static const unsigned long pokey_frq_ideal =  1789790; /* Hz - True */
#if 0
static const int filter_size_44 = 1274;
static const int filter_size_44_8 = 884;
static const int filter_size_22 = 1239;
static const int filter_size_22_8 = 893;
static const int filter_size_11 = 1305;
static const int filter_size_11_8 = 937;
static const int filter_size_48 = 898;
static const int filter_size_48_8 = 626;
static const int filter_size_8  = 1322;
static const int filter_size_8_8 = 1214;
#endif

/* Flags and quality */
static int snd_flags = 0;
static int snd_quality = 0;

/* Poly tables */
static unsigned char poly4tbl[15];
static unsigned char poly5tbl[31];
static unsigned char poly17tbl[131071];
static unsigned char poly9tbl[511];


struct stPokeyState;

typedef unsigned char (*readout_t)(struct stPokeyState* ps);
typedef void (*event_t)(struct stPokeyState* ps, char p5v, char p4v, char p917v);



/* State variables for single Pokey Chip */
typedef struct stPokeyState
{
    /* Poly positions */
    unsigned int poly4pos;
    unsigned int poly5pos;
    unsigned int poly17pos;
    unsigned int poly9pos;

    /* Change queue */
    unsigned char ovola;
    int qet[1322]; /* maximal length of filter */
    unsigned char qev[1322];
    int qebeg;
    int qeend;

    /* Main divider (64khz/15khz) */
    unsigned char mdivk;    /* 28 for 64khz, 114 for 15khz */

    /* Main switches */
    unsigned char selpoly9;
    unsigned char c0_hf;
    unsigned char c1_f0;
    unsigned char c2_hf;
    unsigned char c3_f2;

    /* Main output state */
    unsigned char outvol_all;
    unsigned char forcero; /* Force readout */

    /* channel 0 state */

    readout_t readout_0;
    event_t event_0;

    unsigned long c0divpos;
    unsigned long c0divstart;   /* AUDF0 recalculated */
    unsigned long c0divstart_p; /* start value when c1_f0 */
    unsigned short c0diva;      /* AUDF0 register */
    unsigned char c0ctl;        /* AUDC0 register */

    unsigned char c0t1;         /* D - 5bit, Q goes to sw3 */
    unsigned char c0t2;         /* D - out sw2, Q goes to sw4 and t3 */
    unsigned char c0t3;         /* D - out t2, q goes to xor */

    unsigned char c0sw1;        /* in1 - 4bit, in2 - 17bit, out goes to sw2 */
    unsigned char c0sw2;        /* in1 - /Q t2, in2 - out sw1, out goes to t2 */
    unsigned char c0sw3;        /* in1 - +5, in2 - Q t1, out goes to C t2 */
    unsigned char c0sw4;        /* hi-pass sw */
    unsigned char c0vo;         /* volume only */

    unsigned char c0stop;       /* channel counter stopped */

    unsigned char vol0;

    unsigned char outvol_0;

    /* channel 1 state */

    readout_t readout_1;
    event_t event_1;

    unsigned long c1divpos;
    unsigned long c1divstart;
    unsigned short c1diva;
    unsigned char c1ctl;

    unsigned char c1t1;
    unsigned char c1t2;
    unsigned char c1t3;

    unsigned char c1sw1;
    unsigned char c1sw2;
    unsigned char c1sw3;
    unsigned char c1sw4;
    unsigned char c1vo;

    unsigned char c1stop;      /* channel counter stopped */

    unsigned char vol1;

    unsigned char outvol_1;

    /* channel 2 state */

    readout_t readout_2;
    event_t event_2;

    unsigned long c2divpos;
    unsigned long c2divstart;
    unsigned long c2divstart_p;     /* start value when c1_f0 */
    unsigned short c2diva;
    unsigned char c2ctl;

    unsigned char c2t1;
    unsigned char c2t2;

    unsigned char c2sw1;
    unsigned char c2sw2;
    unsigned char c2sw3;
    unsigned char c2vo;

    unsigned char c2stop;          /* channel counter stopped */

    unsigned char vol2;

    unsigned char outvol_2;

    /* channel 3 state */

    readout_t readout_3;
    event_t event_3;

    unsigned long c3divpos;
    unsigned long c3divstart;
    unsigned short c3diva;
    unsigned char c3ctl;

    unsigned char c3t1;
    unsigned char c3t2;

    unsigned char c3sw1;
    unsigned char c3sw2;
    unsigned char c3sw3;
    unsigned char c3vo;

    unsigned char c3stop;          /* channel counter stopped */

    unsigned char vol3;

    unsigned char outvol_3;

} PokeyState;

PokeyState pokey_states[NPOKEYS];

/* Forward declarations for ResetPokeyState */

static unsigned char readout0_normal(PokeyState* ps);
static void event0_pure(PokeyState* ps, char p5v, char p4v, char p917v);

static unsigned char readout1_normal(PokeyState* ps);
static void event1_pure(PokeyState* ps, char p5v, char p4v, char p917v);

static unsigned char readout2_normal(PokeyState* ps);
static void event2_pure(PokeyState* ps, char p5v, char p4v, char p917v);

static unsigned char readout3_normal(PokeyState* ps);
static void event3_pure(PokeyState* ps, char p5v, char p4v, char p917v);




void ResetPokeyState(PokeyState* ps)
{
    /* Poly positions */
    ps->poly4pos = 0;
    ps->poly5pos = 0;
    ps->poly9pos = 0;
    ps->poly17pos = 0;

    /* Change queue */
    ps->ovola = 0;
    ps->qebeg = 0;
    ps->qeend = 0;

    /* Global Pokey controls */
    ps->mdivk = 28;

    ps->selpoly9 = 0;
    ps->c0_hf = 0;
    ps->c1_f0 = 0;
    ps->c2_hf = 0;
    ps->c3_f2 = 0;

    ps->outvol_all = 0;
    ps->forcero = 0;

    /* Channel 0 state */
    ps->readout_0 = readout0_normal;
    ps->event_0 = event0_pure;

    ps->c0divpos = 1000;
    ps->c0divstart = 1000;
    ps->c0divstart_p = 1000;
    ps->c0diva = 255;
    ps->c0ctl = 0;

    ps->c0t1 = 0;
    ps->c0t2 = 0;
    ps->c0t3 = 0;

    ps->c0sw1 = 0;
    ps->c0sw2 = 0;
    ps->c0sw3 = 0;
    ps->c0sw4 = 0;
    ps->c0vo = 1;

    ps->c0stop = 1;

    ps->vol0 = 0;

    ps->outvol_0 = 0;


    /* Channel 1 state */
    ps->readout_1 = readout1_normal;
    ps->event_1 = event1_pure;

    ps->c1divpos = 1000;
    ps->c1divstart = 1000;
    ps->c1diva = 255;
    ps->c1ctl = 0;

    ps->c1t1 = 0;
    ps->c1t2 = 0;
    ps->c1t3 = 0;

    ps->c1sw1 = 0;
    ps->c1sw2 = 0;
    ps->c1sw3 = 0;
    ps->c1sw4 = 0;
    ps->c1vo = 1;

    ps->c1stop = 1;

    ps->vol1 = 0;

    ps->outvol_1 = 0;

    /* Channel 2 state */

    ps->readout_2 = readout2_normal;
    ps->event_2 = event2_pure;

    ps->c2divpos = 1000;
    ps->c2divstart = 1000;
    ps->c2divstart_p = 1000;
    ps->c2diva = 255;
    ps->c2ctl = 0;

    ps->c2t1 = 0;
    ps->c2t2 = 0;

    ps->c2sw1 = 0;
    ps->c2sw2 = 0;
    ps->c2sw3 = 0;

    ps->c2vo = 0;

    ps->c2stop = 0;

    ps->vol2 = 0;
    ps->outvol_2 = 0;

    /* Channel 3 state */
    ps->readout_3 = readout3_normal;
    ps->event_3 = event3_pure;

    ps->c3divpos = 1000;
    ps->c3divstart = 1000;
    ps->c3diva = 255;
    ps->c3ctl = 0;

    ps->c3t1 = 0;
    ps->c3t2 = 0;

    ps->c3sw1 = 0;
    ps->c3sw2 = 0;
    ps->c3sw3 = 0;

    ps->c3stop = 1;

    ps->vol3 = 0;

    ps->outvol_3 = 0;
}


double read_resam_all(PokeyState* ps)
{
    int i = ps->qebeg;
    unsigned char avol,bvol;
    double sum;

    if(ps->qebeg == ps->qeend)
    {
        return ps->ovola * filter_data[0]; /* if no events in the queue */
    }

    avol = ps->ovola;
    sum = 0;

    /* Separate two loop cases, for wrap-around and without */
    if(ps->qeend < ps->qebeg) /* With wrap */
    {
        while(i<filter_size)
        {
            bvol = ps->qev[i];
            sum += (avol-bvol)*filter_data[ps->qet[i]];
            avol = bvol;
            ++i;
        }
        i=0;
    }

    /* without wrap */
    while(i<ps->qeend)
    {
        bvol = ps->qev[i];
        sum += (avol-bvol)*filter_data[ps->qet[i]];
        avol = bvol;
        ++i;
    }

    sum += avol*filter_data[0];
    return sum;
}

static void add_change(PokeyState* ps, unsigned char a)
{
    ps->qev[ps->qeend] = a;
    ps->qet[ps->qeend] = 0;
    ++ps->qeend;
    if(ps->qeend >= filter_size)
        ps->qeend = 0;
}

static void bump_qe_subticks(PokeyState* ps, int subticks)
{
    /* Remove too old events from the queue while bumping */
    int i = ps->qebeg;
    if(ps->qeend < ps->qebeg) /* Loop with wrap */
    {
        while(i<filter_size)
        {
            ps->qet[i] += subticks;
            if(ps->qet[i] >= filter_size - 1)
            {
                ps->ovola = ps->qev[i];
                ++ps->qebeg;
                if(ps->qebeg >= filter_size)
                    ps->qebeg = 0;
            }
            ++i;
        }
        i=0;
    }
    /* loop without wrap */
    while(i<ps->qeend)
    {
        ps->qet[i] += subticks;
        if(ps->qet[i] >= filter_size - 1)
        {
            ps->ovola = ps->qev[i];
            ++ps->qebeg;
            if(ps->qebeg >= filter_size)
                ps->qebeg = 0;
        }
        ++i;
    }
}



static void build_poly4(void)
{
    unsigned char c;
    unsigned char i;
    unsigned char poly4=1;

    for(i=0; i<15; i++)
    {
        poly4tbl[i] = ~poly4;
        c = ((poly4>>2)&1) ^ ((poly4>>3)&1);
        poly4 = ((poly4<<1)&15) + c;
    }
}

static void build_poly5(void)
{
	unsigned char c;
	unsigned char i;
	unsigned char poly5 = 1;

	for(i = 0; i < 31; i++) {
		poly5tbl[i] = ~poly5; /* Inversion! Attention! */
		c = ((poly5 >> 2) ^ (poly5 >> 4)) & 1;
		poly5 = ((poly5 << 1) & 31) + c;
	}
}

static void build_poly17(void)
{
	unsigned int c;
	unsigned int i;
	unsigned int poly17 = 1;

	for(i = 0; i < 131071; i++) {
		poly17tbl[i] = (unsigned char) poly17;
		c = ((poly17 >> 11) ^ (poly17 >> 16)) & 1;
		poly17 = ((poly17 << 1) & 131071) + c;
	}
}

static void build_poly9(void)
{
	unsigned int c;
	unsigned int i;
	unsigned int poly9 = 1;

	for(i = 0; i < 511; i++) {
		poly9tbl[i] = (unsigned char) poly9;
		c = ((poly9 >> 3) ^ (poly9 >> 8)) & 1;
		poly9 = ((poly9 << 1) & 511) + c;
	}
}

static void advance_polies(PokeyState* ps, unsigned long tacts)
{
    ps->poly4pos = (tacts + ps->poly4pos) % 15;
    ps->poly5pos = (tacts + ps->poly5pos) % 31;
    ps->poly17pos = (tacts + ps->poly17pos) % 131071;
    ps->poly9pos = (tacts + ps->poly9pos) % 511;
}

/***********************************

   READ OUTPUT 0

  ************************************/

static unsigned char readout0_vo(PokeyState* ps)
{
    return ps->vol0;
}

static unsigned char readout0_hipass(PokeyState* ps)
{
    if(ps->c0t2 ^ ps->c0t3)
        return ps->vol0;
    else return 0;
}

static unsigned char readout0_normal(PokeyState* ps)
{
    if(ps->c0t2)
        return ps->vol0;
    else return 0;
}

/***********************************

   READ OUTPUT 1

  ************************************/

static unsigned char readout1_vo(PokeyState* ps)
{
    return ps->vol1;
}

static unsigned char readout1_hipass(PokeyState* ps)
{
    if(ps->c1t2 ^ ps->c1t3)
        return ps->vol1;
    else return 0;
}

static unsigned char readout1_normal(PokeyState* ps)
{
    if(ps->c1t2)
        return ps->vol1;
    else return 0;
}

/***********************************

   READ OUTPUT 2

  ************************************/

static unsigned char readout2_vo(PokeyState* ps)
{
    return ps->vol2;
}

static unsigned char readout2_normal(PokeyState* ps)
{
    if(ps->c2t2)
        return ps->vol2;
    else return 0;
}

/***********************************

   READ OUTPUT 3

  ************************************/

static unsigned char readout3_vo(PokeyState* ps)
{
    return ps->vol3;
}

static unsigned char readout3_normal(PokeyState* ps)
{
    if(ps->c3t2)
        return ps->vol3;
    else return 0;
}


/***********************************

   EVENT CHANNEL 0

  ************************************/

static void event0_pure(PokeyState* ps, char p5v, char p4v, char p917v)
{
    ps->c0t2 = !ps->c0t2;
    ps->c0t1 = p5v;
}

static void event0_p5(PokeyState* ps, char p5v, char p4v, char p917v)
{
    if(ps->c0t1)
        ps->c0t2 = !ps->c0t2;
    ps->c0t1 = p5v;
}

static void event0_p4(PokeyState* ps, char p5v, char p4v, char p917v)
{
    ps->c0t2 = p4v;
    ps->c0t1 = p5v;
}

static void event0_p917(PokeyState* ps, char p5v, char p4v, char p917v)
{
    ps->c0t2 = p917v;
    ps->c0t1 = p5v;
}

static void event0_p4_p5(PokeyState* ps, char p5v, char p4v, char p917v)
{
    if(ps->c0t1)
        ps->c0t2 = p4v;
    ps->c0t1 = p5v;
}

static void event0_p917_p5(PokeyState* ps, char p5v, char p4v, char p917v)
{
    if(ps->c0t1)
        ps->c0t2 = p917v;
    ps->c0t1 = p5v;
}

/***********************************

   EVENT CHANNEL 1

  ************************************/

static void event1_pure(PokeyState* ps, char p5v, char p4v, char p917v)
{
    ps->c1t2 = !ps->c1t2;
    ps->c1t1 = p5v;
}

static void event1_p5(PokeyState* ps, char p5v, char p4v, char p917v)
{
    if(ps->c1t1)
        ps->c1t2 = !ps->c1t2;
    ps->c1t1 = p5v;
}

static void event1_p4(PokeyState* ps, char p5v, char p4v, char p917v)
{
    ps->c1t2 = p4v;
    ps->c1t1 = p5v;
}

static void event1_p917(PokeyState* ps, char p5v, char p4v, char p917v)
{
    ps->c1t2 = p917v;
    ps->c1t1 = p5v;
}

static void event1_p4_p5(PokeyState* ps, char p5v, char p4v, char p917v)
{
    if(ps->c1t1)
        ps->c1t2 = p4v;
    ps->c1t1 = p5v;
}

static void event1_p917_p5(PokeyState* ps, char p5v, char p4v, char p917v)
{
    if(ps->c1t1)
        ps->c1t2 = p917v;
    ps->c1t1 = p5v;
}

/***********************************

   EVENT CHANNEL 2

  ************************************/

static void event2_pure(PokeyState* ps, char p5v, char p4v, char p917v)
{
    ps->c2t2 = !ps->c2t2;
    ps->c2t1 = p5v;
    /* high-pass clock for channel 0 */
    ps->c0t3 = ps->c0t2;
}

static void event2_p5(PokeyState* ps, char p5v, char p4v, char p917v)
{
    if(ps->c2t1)
        ps->c2t2 = !ps->c2t2;
    ps->c2t1 = p5v;
    /* high-pass clock for channel 0 */
    ps->c0t3 = ps->c0t2;
}

static void event2_p4(PokeyState* ps, char p5v, char p4v, char p917v)
{
    ps->c2t2 = p4v;
    ps->c2t1 = p5v;
    /* high-pass clock for channel 0 */
    ps->c0t3 = ps->c0t2;
}

static void event2_p917(PokeyState* ps, char p5v, char p4v, char p917v)
{
    ps->c2t2 = p917v;
    ps->c2t1 = p5v;
    /* high-pass clock for channel 0 */
    ps->c0t3 = ps->c0t2;
}

static void event2_p4_p5(PokeyState* ps, char p5v, char p4v, char p917v)
{
    if(ps->c2t1)
        ps->c2t2 = p4v;
    ps->c2t1 = p5v;
    /* high-pass clock for channel 0 */
    ps->c0t3 = ps->c0t2;
}

static void event2_p917_p5(PokeyState* ps, char p5v, char p4v, char p917v)
{
    if(ps->c2t1)
        ps->c2t2 = p917v;
    ps->c2t1 = p5v;
    /* high-pass clock for channel 0 */
    ps->c0t3 = ps->c0t2;
}

/***********************************

   EVENT CHANNEL 3

  ************************************/

static void event3_pure(PokeyState* ps, char p5v, char p4v, char p917v)
{
    ps->c3t2 = !ps->c3t2;
    ps->c3t1 = p5v;
    /* high-pass closk for channel 1 */
    ps->c1t3 = ps->c1t2;
}

static void event3_p5(PokeyState* ps, char p5v, char p4v, char p917v)
{
    if(ps->c3t1)
        ps->c3t2 = !ps->c3t2;
    ps->c3t1 = p5v;
    /* high-pass closk for channel 1 */
    ps->c1t3 = ps->c1t2;
}

static void event3_p4(PokeyState* ps, char p5v, char p4v, char p917v)
{
    ps->c3t2 = p4v;
    ps->c3t1 = p5v;
    /* high-pass closk for channel 1 */
    ps->c1t3 = ps->c1t2;
}

static void event3_p917(PokeyState* ps, char p5v, char p4v, char p917v)
{
    ps->c3t2 = p917v;
    ps->c3t1 = p5v;
    /* high-pass closk for channel 1 */
    ps->c1t3 = ps->c1t2;
}

static void event3_p4_p5(PokeyState* ps, char p5v, char p4v, char p917v)
{
    if(ps->c3t1)
        ps->c3t2 = p4v;
    ps->c3t1 = p5v;
    /* high-pass closk for channel 1 */
    ps->c1t3 = ps->c1t2;
}

static void event3_p917_p5(PokeyState* ps, char p5v, char p4v, char p917v)
{
    if(ps->c3t1)
        ps->c3t2 = p917v;
    ps->c3t1 = p5v;
    /* high-pass closk for channel 1 */
    ps->c1t3 = ps->c1t2;
}

static void advance_ticks(PokeyState* ps, unsigned long ticks)
{
    unsigned long ta,tbe, tbe0, tbe1, tbe2, tbe3;
    char p5v,p4v,p917v;

    unsigned char outvol_new;
    unsigned char need0=0;
    unsigned char need1=0;
    unsigned char need2=0;
    unsigned char need3=0;

    unsigned char need=0;

    if(ps->forcero)
    {
        ps->forcero = 0;
        outvol_new = ps->outvol_0 + ps->outvol_1 + ps->outvol_2 + ps->outvol_3;
        if(outvol_new != ps->outvol_all)
        {
            ps->outvol_all = outvol_new;
            add_change(ps, outvol_new);
        }
    }

    while(ticks>0)
    {
        tbe0 = ps->c0divpos;
        tbe1 = ps->c1divpos;
        tbe2 = ps->c2divpos;
        tbe3 = ps->c3divpos;

        tbe = ticks+1;

        if(!ps->c0stop && tbe0 < tbe)
            tbe = tbe0;
        if(!ps->c1stop && tbe1 < tbe)
            tbe = tbe1;
        if(!ps->c2stop && tbe2 < tbe)
            tbe = tbe2;
        if(!ps->c3stop && tbe3 < tbe)
            tbe = tbe3;

        if(tbe>ticks)
            ta = ticks;
        else
        {
            ta = tbe;
            need = 1;
        }

        ticks -= ta;

        if(!ps->c0stop) ps->c0divpos -= ta;
        if(!ps->c1stop) ps->c1divpos -= ta;
        if(!ps->c2stop) ps->c2divpos -= ta;
        if(!ps->c3stop) ps->c3divpos -= ta;

        advance_polies(ps,ta);
        bump_qe_subticks(ps,ta);

        if(need)
        {
            p5v = poly5tbl[ps->poly5pos] & 1;
            p4v = poly4tbl[ps->poly4pos] & 1;
            if(ps->selpoly9)
                p917v = poly9tbl[ps->poly9pos] & 1;
            else
                p917v = poly17tbl[ps->poly17pos] & 1;

            if(!ps->c0stop && ta == tbe0)
            {
                ps->event_0(ps,p5v,p4v,p917v);
                ps->c0divpos = ps->c0divstart;
                need0 = 1;
            }
            if(!ps->c1stop && ta == tbe1)
            {
                ps->event_1(ps,p5v,p4v,p917v);
                ps->c1divpos = ps->c1divstart;
                if(ps->c1_f0)
                    ps->c0divpos = ps->c0divstart_p;
                need1 = 1;
            }
            if(!ps->c2stop && ta == tbe2)
            {
                ps->event_2(ps,p5v,p4v,p917v);
                ps->c2divpos = ps->c2divstart;
                need2 = 1;
                if(ps->c0sw4)
                    need0 = 1;
            }
            if(!ps->c3stop && ta == tbe3)
            {
                ps->event_3(ps,p5v,p4v,p917v);
                ps->c3divpos = ps->c3divstart;
                if(ps->c3_f2)
                    ps->c2divpos = ps->c2divstart_p;
                need3 = 1;
                if(ps->c1sw4)
                    need1 = 1;
            }

            if(need0)
            {
                ps->outvol_0 = 2*ps->readout_0(ps);
            }
            if(need1)
            {
                ps->outvol_1 = 2*ps->readout_1(ps);
            }
            if(need2)
            {
                ps->outvol_2 = 2*ps->readout_2(ps);
            }
            if(need3)
            {
                ps->outvol_3 = 2*ps->readout_3(ps);
            }

            outvol_new = ps->outvol_0 + ps->outvol_1 + ps->outvol_2 + ps->outvol_3;
            if(outvol_new != ps->outvol_all)
            {
                ps->outvol_all = outvol_new;
                add_change(ps, outvol_new);
            }
        }
    }
}

static double generate_sample(PokeyState* ps)
{
    /*unsigned long ta = (subticks+pokey_frq)/sample_rate;
    subticks = (subticks+pokey_frq)%sample_rate;*/

    advance_ticks(ps, pokey_frq/sample_rate);
    return read_resam_all(ps);
}

/******************************************
 filter table generator by Krzysztof Nikiel
 ******************************************/

static int remez_filter_table(double resamp_rate, /* output_rate/input_rate */
                              double *cutoff, int quality)
{
  int i;
  static const int orders[] = {600, 800, 1000, 1200};
  static const struct {
    int stop;		/* stopband ripple */
    double weight;	/* stopband weight */
    double twidth[sizeof(orders)/sizeof(orders[0])];
  } paramtab[] =
  {
    {70, 90, {4.9e-3, 3.45e-3, 2.65e-3, 2.2e-3}},
    {55, 25, {3.4e-3, 2.7e-3, 2.05e-3, 1.7e-3}},
    {40, 6.0, {2.6e-3, 1.8e-3, 1.5e-3, 1.2e-3}},
    {-1, 0, {0, 0, 0, 0}}
  };
  static const double passtab[] = {0.5, 0.6, 0.7};
  int ripple = 0, order = 0;
  int size;
  double weights[2], desired[2], bands[4];
  static const int interlevel = 5;
  double step = 1.0 / interlevel;

  *cutoff = 0.95 * 0.5 * resamp_rate;

  if (quality >= (int) (sizeof(passtab) / sizeof(passtab[0])))
    quality = (int) (sizeof(passtab) / sizeof(passtab[0])) - 1;

  for (ripple = 0; paramtab[ripple].stop > 0; ripple++)
  {
    for (order = 0; order < (int) (sizeof(orders)/sizeof(orders[0])); order++)
    {
      if ((*cutoff - paramtab[ripple].twidth[order])
	  > passtab[quality] * 0.5 * resamp_rate)
	/* transition width OK */
	goto found;
    }
  }

  /* not found -- use shortest trasition */
  ripple--;
  order--;

found:

#if 0
  printf("order: %d, cutoff: %g\tstopband:%d\ttranswidth:%f\n",
         orders[order],
	 1789790 * *cutoff,
	 paramtab[ripple].stop,
	 1789790 * paramtab[ripple].twidth[order]);
  exit(1);
#endif

  size = orders[order] + 1;

  if (size > SND_FILTER_SIZE) /* static table too short */
    return 0;

  desired[0] = 1;
  desired[1] = 0;

  weights[0] = 1;
  weights[1] = paramtab[ripple].weight;

  bands[0] = 0;
  bands[2] = *cutoff;
  bands[1] = bands[2] - paramtab[ripple].twidth[order];
  bands[3] = 0.5;

  bands[1] *= (double)interlevel;
  bands[2] *= (double)interlevel;
  remez(filter_data, (size / interlevel) + 1, 2, bands, desired, weights, BANDPASS);
  for (i = size - interlevel; i >= 0; i -= interlevel)
  {
    int s;
    double h1 = filter_data[i/interlevel];
    double h2 = filter_data[i/interlevel+1];

    for (s = 0; s < interlevel; s++)
    {
      double d = (double)s * step;
      filter_data[i+s] = (h1*(1.0 - d) + h2 * d) * step;
    }
  }

  /* compute reversed cumulative sum table */
  for (i = size - 2; i >= 0; i--)
    filter_data[i] += filter_data[i + 1];

#if 0
  for (i = 0; i < size; i++)
    printf("%.15f,\n", filter_data[i]);
  fflush(stdout);
  exit(1);
#endif

  return size;
}




/*****************************************************************************/
/* Module:  Pokey_sound_init()                                               */
/* Purpose: to handle the power-up initialization functions                  */
/*          these functions should only be executed on a cold-restart        */
/*                                                                           */
/* Authors: Michael Borisov, Krzystof Nikiel                                 */
/*                                                                           */
/* Inputs:  freq17 - the value for the '1.79MHz' Pokey audio clock           */
/*          playback_freq - the playback frequency in samples per second     */
/*          num_pokeys - specifies the number of pokey chips to be emulated  */
/*                                                                           */
/* Outputs: Adjusts local globals - no return value                          */
/*                                                                           */
/*****************************************************************************/

static void Pokey_process_8(void* sndbuffer, unsigned sndn);
static void Pokey_process_16(void* sndbuffer, unsigned sndn);
static void Update_pokey_sound_mz(uint16 addr, uint8 val, uint8 chip, uint8 gain);
#ifdef SERIO_SOUND
static void Update_serio_sound_mz(int out, UBYTE data);
#endif
#ifdef CONSOLE_SOUND
static void Update_consol_sound_mz( int set );
#endif
#ifdef VOL_ONLY_SOUND
static void Update_vol_only_sound_mz( void );
#endif

int Pokey_sound_init_mz(uint32 freq17, uint16 playback_freq, uint8 num_pokeys,
                        int flags, int quality
#ifdef __PLUS
                        , int clear_regs
#endif
                       )
{
    double cutoff;

    sample_rate = playback_freq;
    snd_flags = flags;
    snd_quality = quality;

    Update_pokey_sound = Update_pokey_sound_mz;
#ifdef SERIO_SOUND
    Update_serio_sound = Update_serio_sound_mz;
#endif
#ifdef CONSOLE_SOUND
    Update_consol_sound = Update_consol_sound_mz;
#endif
#ifdef VOL_ONLY_SOUND
    Update_vol_only_sound = Update_vol_only_sound_mz;
#endif

#ifdef VOL_ONLY_SOUND
	samp_freq=playback_freq;
#endif  /* VOL_ONLY_SOUND */

	Pokey_process_ptr = (flags & SND_BIT16) ? Pokey_process_16 : Pokey_process_8;

    switch(playback_freq)
    {
#if 0
    case 44100:
        if(flags & SND_BIT16)
        {
            filter_data = filter_44;
            filter_size = filter_size_44;
        }
        else
        {
            filter_data = filter_44_8;
            filter_size = filter_size_44_8;
        }
        pokey_frq = 1808100; /* 1.02% off ideal */
        audible_frq = 20000; /* ultrasound */
        break;
    case 22050:
        if(flags & SND_BIT16)
        {
            filter_data = filter_22;
            filter_size = filter_size_22;
        }
        else
        {
            filter_data = filter_22_8;
            filter_size = filter_size_22_8;
        }
        pokey_frq = 1786050; /* 0.2% off ideal */
        audible_frq = 10000; /* 30db filter attenuation */
        break;
    case 11025:
        if(flags & SND_BIT16)
        {
            filter_data = filter_11;
            filter_size = filter_size_11;
        }
        else
        {
            filter_data = filter_11_8;
            filter_size = filter_size_11_8;
        }
        pokey_frq = 1786050; /* 0.2% off ideal */
        audible_frq = 4500; /* 30db filter attenuation */
        break;
    case 48000:
        if(flags & SND_BIT16)
        {
            filter_data = filter_48;
            filter_size = filter_size_48;
        }
        else
        {
            filter_data = filter_48_8;
            filter_size = filter_size_48_8;
        }
        pokey_frq = 1776000; /* 0.7% off ideal */
        audible_frq = 20000; /* ultrasound */
        break;
    case 8000:
        if(flags & SND_BIT16)
        {
            filter_data = filter_8;
            filter_size = filter_size_8;
        }
        else
        {
            filter_data = filter_8_8;
            filter_size = filter_size_8_8;
        }
        pokey_frq = 1792000; /* 0.1% off ideal */
        audible_frq = 4000; /* Nyquist, also 30db attn, should be 50 */
        break;
#endif
    default:
        pokey_frq = (int)(((double)pokey_frq_ideal/sample_rate) + 0.5)
          * sample_rate;
	filter_size = remez_filter_table((double)sample_rate/pokey_frq,
					 &cutoff, quality);
	audible_frq = (unsigned) (cutoff * pokey_frq);
    }

    build_poly4();
    build_poly5();
    build_poly9();
    build_poly17();

#ifdef __PLUS
	if (clear_regs)
#endif
	{
		ResetPokeyState(pokey_states);
		ResetPokeyState(pokey_states + 1);
	}
	num_cur_pokeys = num_pokeys;

	return 0; /* OK */
}

/*****************************************************************************/
/* Function: Update_pokey_sound()                                            */
/*                                                                           */
/* Inputs:  addr - the address of the parameter to be changed                */
/*          val - the new value to be placed in the specified address        */
/*          gain - specified as an 8-bit fixed point number - use 1 for no   */
/*                 amplification (output is multiplied by gain)              */
/*                                                                           */
/* Outputs: Adjusts local globals - no return value                          */
/*                                                                           */
/*****************************************************************************/

static void Update_readout_0(PokeyState* ps)
{
    if(ps->c0vo)
        ps->readout_0 = readout0_vo;
    else if(ps->c0sw4)
        ps->readout_0 = readout0_hipass;
    else
        ps->readout_0 = readout0_normal;
}

static void Update_readout_1(PokeyState* ps)
{
    if(ps->c1vo)
        ps->readout_1 = readout1_vo;
    else if(ps->c1sw4)
        ps->readout_1 = readout1_hipass;
    else
        ps->readout_1 = readout1_normal;
}

static void Update_readout_2(PokeyState* ps)
{
    if(ps->c2vo)
        ps->readout_2 = readout2_vo;
    else
        ps->readout_2 = readout2_normal;
}

static void Update_readout_3(PokeyState* ps)
{
    if(ps->c3vo)
        ps->readout_3 = readout3_vo;
    else
        ps->readout_3 = readout3_normal;
}

static void Update_event0(PokeyState* ps)
{
    if(ps->c0sw3)
    {
        if(ps->c0sw2)
            ps->event_0 = event0_pure;
        else
        {
            if(ps->c0sw1)
                ps->event_0 = event0_p4;
            else
                ps->event_0 = event0_p917;
        }
    }
    else
    {
        if(ps->c0sw2)
            ps->event_0 = event0_p5;
        else
        {
            if(ps->c0sw1)
                ps->event_0 = event0_p4_p5;
            else
                ps->event_0 = event0_p917_p5;
        }
    }
}

static void Update_event1(PokeyState* ps)
{
    if(ps->c1sw3)
    {
        if(ps->c1sw2)
            ps->event_1 = event1_pure;
        else
        {
            if(ps->c1sw1)
                ps->event_1 = event1_p4;
            else
                ps->event_1 = event1_p917;
        }
    }
    else
    {
        if(ps->c1sw2)
            ps->event_1 = event1_p5;
        else
        {
            if(ps->c1sw1)
                ps->event_1 = event1_p4_p5;
            else
                ps->event_1 = event1_p917_p5;
        }
    }
}

static void Update_event2(PokeyState* ps)
{
    if(ps->c2sw3)
    {
        if(ps->c2sw2)
            ps->event_2 = event2_pure;
        else
        {
            if(ps->c2sw1)
                ps->event_2 = event2_p4;
            else
                ps->event_2 = event2_p917;
        }
    }
    else
    {
        if(ps->c2sw2)
            ps->event_2 = event2_p5;
        else
        {
            if(ps->c2sw1)
                ps->event_2 = event2_p4_p5;
            else
                ps->event_2 = event2_p917_p5;
        }
    }
}

static void Update_event3(PokeyState* ps)
{
    if(ps->c3sw3)
    {
        if(ps->c3sw2)
            ps->event_3 = event3_pure;
        else
        {
            if(ps->c3sw1)
                ps->event_3 = event3_p4;
            else
                ps->event_3 = event3_p917;
        }
    }
    else
    {
        if(ps->c3sw2)
            ps->event_3 = event3_p5;
        else
        {
            if(ps->c3sw1)
                ps->event_3 = event3_p4_p5;
            else
                ps->event_3 = event3_p917_p5;
        }
    }
}

static void Update_c0divstart(PokeyState* ps)
{
    if(ps->c1_f0)
    {
        if(ps->c0_hf)
        {
            ps->c0divstart = 256;
            ps->c0divstart_p = ps->c0diva + 7;
        }
        else
        {
            ps->c0divstart = 256 * ps->mdivk;
            ps->c0divstart_p = (ps->c0diva+1)*ps->mdivk;
        }
    }
    else
    {
        if(ps->c0_hf)
            ps->c0divstart = ps->c0diva + 4;
        else
            ps->c0divstart = (ps->c0diva+1) * ps->mdivk;
    }
}

static void Update_c1divstart(PokeyState* ps)
{
    if(ps->c1_f0)
    {
        if(ps->c0_hf)
            ps->c1divstart = ps->c0diva + 256*ps->c1diva + 7;
        else
            ps->c1divstart = (ps->c0diva + 256*ps->c1diva + 1) * ps->mdivk;
    }
    else
        ps->c1divstart = (ps->c1diva + 1) * ps->mdivk;
}

static void Update_c2divstart(PokeyState* ps)
{
    if(ps->c3_f2)
    {
        if(ps->c2_hf)
        {
            ps->c2divstart = 256;
            ps->c2divstart_p = ps->c2diva + 7;
        }
        else
        {
            ps->c2divstart = 256 * ps->mdivk;
            ps->c2divstart_p = (ps->c2diva+1)*ps->mdivk;
        }
    }
    else
    {
        if(ps->c2_hf)
            ps->c2divstart = ps->c2diva + 4;
        else
            ps->c2divstart = (ps->c2diva+1) * ps->mdivk;
    }
}

static void Update_c3divstart(PokeyState* ps)
{
    if(ps->c3_f2)
    {
        if(ps->c2_hf)
            ps->c3divstart = ps->c2diva + 256*ps->c3diva + 7;
        else
            ps->c3divstart = (ps->c2diva + 256*ps->c3diva + 1) * ps->mdivk;
    }
    else
        ps->c3divstart = (ps->c3diva + 1) * ps->mdivk;
}

static void Update_audctl(PokeyState* ps, unsigned char val)
{
    unsigned char nc0_hf,nc2_hf,nc1_f0,nc3_f2,nc0sw4,nc1sw4,new_divk;
    unsigned char recalc0=0;
    unsigned char recalc1=0;
    unsigned char recalc2=0;
    unsigned char recalc3=0;

    unsigned int cnt0 = 0;
    unsigned int cnt1 = 0;
    unsigned int cnt2 = 0;
    unsigned int cnt3 = 0;

    nc0_hf = (val & 0x40) != 0;
    nc2_hf = (val & 0x20) != 0;
    nc1_f0 = (val & 0x10) != 0;
    nc3_f2 = (val & 0x08) != 0;
    nc0sw4 = (val & 0x04) != 0;
    nc1sw4 = (val & 0x02) != 0;
    if(val & 0x01)
        new_divk = 114;
    else
        new_divk = 28;

    if(new_divk != ps->mdivk)
    {
        recalc0 = recalc1 = recalc2 = recalc3 = 1;
    }
    if(nc1_f0 != ps->c1_f0)
    {
        recalc0 = recalc1 = 1;
    }
    if(nc3_f2 != ps->c3_f2)
    {
        recalc2 = recalc3 = 1;
    }
    if(nc0_hf != ps->c0_hf)
    {
        recalc0 = 1;
        if(nc1_f0)
            recalc1 = 1;
    }
    if(nc2_hf != ps->c2_hf)
    {
        recalc2 = 1;
        if(nc3_f2)
            recalc3 = 1;
    }

    if(recalc0)
    {
        if(ps->c0_hf)
            cnt0 = ps->c0divpos;
        else
            cnt0 = ps->c0divpos/ps->mdivk;
    }
    if(recalc1)
    {
        if(ps->c1_f0)
        {
            if(ps->c0_hf)
                cnt1 = ps->c1divpos/256;
            else
                cnt1 = ps->c1divpos/256/ps->mdivk;
        }
        else
        {
            cnt1 = ps->c1divpos/ps->mdivk;
        }
    }
    if(recalc2)
    {
        if(ps->c2_hf)
            cnt2 = ps->c2divpos;
        else
            cnt2 = ps->c2divpos/ps->mdivk;
    }
    if(recalc3)
    {
        if(ps->c3_f2)
        {
            if(ps->c2_hf)
                cnt3 = ps->c3divpos/256;
            else
                cnt3 = ps->c3divpos/256/ps->mdivk;
        }
    }

    if(recalc0)
    {
        if(nc0_hf)
            ps->c0divpos = cnt0;
        else
            ps->c0divpos = cnt0*new_divk;
    }
    if(recalc1)
    {
        if(nc1_f0)
        {
            if(nc0_hf)
                ps->c1divpos = cnt1*256+cnt0;
            else
                ps->c1divpos = (cnt1*256+cnt0)*new_divk;
        }
        else
        {
            ps->c1divpos = cnt1*new_divk;
        }
    }

    if(recalc2)
    {
        if(nc2_hf)
            ps->c2divpos = cnt2;
        else
            ps->c2divpos = cnt2*new_divk;
    }
    if(recalc3)
    {
        if(nc3_f2)
        {
            if(nc2_hf)
                ps->c3divpos = cnt3*256+cnt2;
            else
                ps->c3divpos = (cnt3*256+cnt2)*new_divk;
        }
    }

    ps->c0_hf = nc0_hf;
    ps->c2_hf = nc2_hf;
    ps->c1_f0 = nc1_f0;
    ps->c3_f2 = nc3_f2;
    ps->c0sw4 = nc0sw4;
    ps->c1sw4 = nc1sw4;
    ps->mdivk = new_divk;
}

static void Update_c0stop(PokeyState* ps)
{
    unsigned long lim = pokey_frq/2/audible_frq;

    unsigned char hfa = 0;
    ps->c0stop = 0;

    if(ps->c0vo || ps->vol0 == 0)
        ps->c0stop = 1;
    else if(!ps->c0sw4 && ps->c0sw3 && ps->c0sw2) /* If channel 0 is a pure tone... */
    {
        if(ps->c1_f0)
        {
            if(ps->c1divstart <= lim)
            {
                ps->c0stop = 1;
                hfa = 1;
            }
        }
        else
        {
            if(ps->c0divstart <= lim)
            {
                ps->c0stop = 1;
                hfa = 1;
            }
        }
    }
    else if(!ps->c0sw4 && ps->c0sw3 && !ps->c0sw2 && ps->c0sw1) /* if channel 0 is poly4... */
    {
        /* period for poly4 signal is 15 cycles */
        if(ps->c1_f0)
        {
            if(ps->c1divstart <= lim*2/15) /* all poly4 signal is above Nyquist */
            {
                ps->c0stop = 1;
                hfa = 1;
            }
        }
        else
        {
            if(ps->c0divstart <= lim*2/15)
            {
                ps->c0stop = 1;
                hfa = 1;
            }
        }
    }

    ps->outvol_0 = 2*ps->readout_0(ps);
    if(hfa)
        ps->outvol_0 = ps->vol0;
}

static void Update_c1stop(PokeyState* ps)
{
    unsigned long lim = pokey_frq/2/audible_frq;

    unsigned char hfa = 0;
    ps->c1stop = 0;

    if(!ps->c1_f0 && (ps->c1vo || ps->vol1 == 0))
        ps->c1stop = 1;
    else if(!ps->c1sw4 && ps->c1sw3 && ps->c1sw2 && ps->c1divstart <= lim) /* If channel 1 is a pure tone */
    {
        ps->c1stop = 1;
        hfa = 1;
    }
    else if(!ps->c1sw4 && ps->c1sw3 && !ps->c1sw2 && ps->c1sw1 && ps->c1divstart <= lim*2/15)  /* all poly4 signal is above Nyquist */
    {
        ps->c1stop = 1;
        hfa = 1;
    }

    ps->outvol_1 = 2*ps->readout_1(ps);
    if(hfa)
        ps->outvol_1 = ps->vol1;
}

static void Update_c2stop(PokeyState* ps)
{
    unsigned long lim = pokey_frq/2/audible_frq;

    unsigned char hfa = 0;
    ps->c2stop = 0;

    if(!ps->c0sw4 && (ps->c2vo || ps->vol2 == 0))
        ps->c2stop = 1;
    /* If channel 2 is a pure tone and no filter for c0... */
    else if(ps->c2sw3 && ps->c2sw2 && !ps->c0sw4)
    {
        if(ps->c3_f2)
        {
            if(ps->c3divstart <= lim)
            {
                ps->c2stop = 1;
                hfa = 1;
            }
        }
        else
        {
            if(ps->c2divstart <= lim)
            {
                ps->c2stop = 1;
                hfa = 1;
            }
        }
    }
    else if(ps->c2sw3 && !ps->c2sw2 && ps->c2sw1 && !ps->c0sw4) /* if channel 2 is poly4 and no filter for c0... */
    {
        /* period for poly4 signal is 15 cycles */
        if(ps->c3_f2)
        {
            if(ps->c3divstart <= lim*2/15) /* all poly4 signal is above Nyquist */
            {
                ps->c2stop = 1;
                hfa = 1;
            }
        }
        else
        {
            if(ps->c2divstart <= lim*2/15)
            {
                ps->c2stop = 1;
                hfa = 1;
            }
        }
    }

    ps->outvol_2 = 2*ps->readout_2(ps);
    if(hfa)
        ps->outvol_2 = ps->vol2;
}

static void Update_c3stop(PokeyState* ps)
{
    unsigned long lim = pokey_frq/2/audible_frq;
    unsigned char hfa = 0;
    ps->c3stop = 0;

    if(!ps->c1sw4 && !ps->c3_f2 && (ps->c3vo || ps->vol3 == 0))
        ps->c3stop = 1;
    /* If channel 3 is a pure tone */
    else if(ps->c3sw3 && ps->c3sw2 && !ps->c1sw4 && ps->c3divstart <= lim)
    {
        ps->c3stop = 1;
        hfa = 1;
    }
    else if(ps->c3sw3 && !ps->c3sw2 && ps->c3sw1 && !ps->c1sw4 && ps->c3divstart <= lim*2/15)  /* all poly4 signal is above Nyquist */
    {
        ps->c3stop = 1;
        hfa = 1;
    }

    ps->outvol_3 = 2*ps->readout_3(ps);
    if(hfa)
        ps->outvol_3 = ps->vol3;
}

static void Update_pokey_sound_mz(uint16 addr, uint8 val, uint8 chip, uint8 gain)
{
    PokeyState* ps = pokey_states+chip;

    switch(addr & 0x0f)
    {
    case _AUDF1:
        ps->c0diva = val;
        Update_c0divstart(ps);
        if(ps->c1_f0)
        {
            Update_c1divstart(ps);
            Update_c1stop(ps);
        }
        Update_c0stop(ps);
        ps->forcero = 1;
        break;
    case _AUDC1:
        ps->c0sw1 = (val & 0x40) != 0;
        ps->c0sw2 = (val & 0x20) != 0;
        ps->c0sw3 = (val & 0x80) != 0;
        ps->vol0 = val & 0xF;
        ps->c0vo = (val & 0x10) != 0;
        Update_readout_0(ps);
        Update_event0(ps);
        Update_c0stop(ps);
        ps->forcero = 1;
        break;
    case _AUDF2:
        ps->c1diva = val;
        Update_c1divstart(ps);
        if(ps->c1_f0)
        {
            Update_c0divstart(ps);
            Update_c0stop(ps);
        }
        Update_c1stop(ps);
        ps->forcero = 1;
        break;
    case _AUDC2:
        ps->c1sw1 = (val & 0x40) != 0;
        ps->c1sw2 = (val & 0x20) != 0;
        ps->c1sw3 = (val & 0x80) != 0;
        ps->vol1 = val & 0xF;
        ps->c1vo = (val & 0x10) != 0;
        Update_readout_1(ps);
        Update_event1(ps);
        Update_c1stop(ps);
        ps->forcero = 1;
        break;
    case _AUDF3:
        ps->c2diva = val;
        Update_c2divstart(ps);
        if(ps->c3_f2)
        {
            Update_c3divstart(ps);
            Update_c3stop(ps);
        }
        Update_c2stop(ps);
        ps->forcero = 1;
        break;
    case _AUDC3:
        ps->c2sw1 = (val & 0x40) != 0;
        ps->c2sw2 = (val & 0x20) != 0;
        ps->c2sw3 = (val & 0x80) != 0;
        ps->vol2 = val & 0xF;
        ps->c2vo = (val & 0x10) != 0;
        Update_readout_2(ps);
        Update_event2(ps);
        Update_c2stop(ps);
        ps->forcero = 1;
        break;
    case _AUDF4:
        ps->c3diva = val;
        Update_c3divstart(ps);
        if(ps->c3_f2)
        {
            Update_c2divstart(ps);
            Update_c2stop(ps);
        }
        Update_c3stop(ps);
        ps->forcero = 1;
        break;
    case _AUDC4:
        ps->c3sw1 = (val & 0x40) != 0;
        ps->c3sw2 = (val & 0x20) != 0;
        ps->c3sw3 = (val & 0x80) != 0;
        ps->vol3 = val & 0xF;
        ps->c3vo = (val & 0x10) != 0;
        Update_readout_3(ps);
        Update_event3(ps);
        Update_c3stop(ps);
        ps->forcero = 1;
        break;
    case _AUDCTL:
        ps->selpoly9 = (val & 0x80) != 0;
        Update_audctl(ps,val);
        Update_readout_0(ps);
        Update_readout_1(ps);
        Update_readout_2(ps);
        Update_readout_3(ps);
        Update_c0divstart(ps);
        Update_c1divstart(ps);
        Update_c2divstart(ps);
        Update_c3divstart(ps);
        Update_c0stop(ps);
        Update_c1stop(ps);
        Update_c2stop(ps);
        Update_c3stop(ps);
        ps->forcero = 1;
        break;
    case _STIMER:
        if(ps->c1_f0)
            ps->c0divpos = ps->c0divstart_p;
        else
            ps->c0divpos = ps->c0divstart;
        ps->c1divpos = ps->c1divstart;
        if(ps->c3_f2)
            ps->c2divpos = ps->c2divstart_p;
        else
            ps->c2divpos = ps->c2divstart;

        ps->c3divpos = ps->c3divstart;
        ps->c0t2 = 1;
        ps->c1t2 = 1;
        ps->c2t2 = 0;
        ps->c3t2 = 0;
        break;
    }
}

void Pokey_debugreset(uint8 chip)
{
    PokeyState* ps = pokey_states+chip;

    if(ps->c1_f0)
        ps->c0divpos = ps->c0divstart_p;
    else
        ps->c0divpos = ps->c0divstart;
    ps->c1divpos = ps->c1divstart;
    if(ps->c3_f2)
        ps->c2divpos = ps->c2divstart_p;
    else
        ps->c2divpos = ps->c2divstart;
    ps->c3divpos = ps->c3divstart;

    ps->c0t2 = 1;
    ps->c1t2 = 1;
    ps->c2t2 = 1;
    ps->c3t2 = 1;
}

/**************************************************************

           Master gain and DC offset calculation
                 by Michael Borisov

 In order to use the available 8-bit or 16-bit dynamic range
 to full extent, reducing the influence of quantization
 noise while simultaneously avoiding overflows, gain
 and DC offset should be set to appropriate value.

 All Pokey-generated sounds have maximal amplitude of 15.
 When all four channels sound simultaneously and in the
 same phase, amplidudes would add up to 60.

 If Pokey is generating a 'pure tone', it always has a DC
 offset of half its amplitude. For other signals (produced
 by poly generators) DC offset varies, but it is always near
 to half amplitude and never exceeds this value.

 In case that pure tone base frequency is outside of audible
 range (ultrasound frequency for high sample rates and above-
 Nyquist frequency for low sample rates), to speed up the engine,
 the generator is stopped while having only DC offset on the
 output (half of corresponding AUDV value). In order that this
 DC offset can be always represented as integer, AUDV values
 are multiplied by 2 when the generator works.

 Therefore maximum integer value before resampling filters
 would be 60*2 = 120 while having maximum DC offset of 60.
 Resampling does not change the DC offset, therefore we may
 subtract it from the signal either before or after resampling.
 In mzpokeysnd, DC offset is subtracted after resampling, however
 for better understanding in further measurements I assume
 subtracting DC before. So, input range for the resampler
 becomes [-60 .. 60].

 Resampling filter removes some harmonics from the signal as if
 the rectangular wave was Fourier transformed forth-and-back,
 while zeroing all harmonics above cutoff frequency. In case
 of high-frequency pure tone (above samplerate/8), only first
 harmonic of the Fourier transofm will remain. As it
 is known, Fourier-transform of the rectangular function of
 amplitude 1 has first oscillation component of amplitude 4/M_PI.
 Therefore, maximum sample values for filtered rectangular
 signal may exceed the amplitude  of rectangular signal
 by up to 4/M_PI times.

 Since our range before resampler is -60 .. 60, taking into
 account mentioned effect with band limiting, range of values
 on the resampler output appears to be in the following bounds:
 [-60*4/M_PI .. 60*4/M_PI]

 In order to map this into signed 8-bit range [-128 .. 127], we
 should multiply the resampler output by 127/60/4*M_PI.

 As it is common for sound hardware to have 8-bit sound unsigned,
 additional DC offset of 128 must be added.

 For 16-bit case the output range is [-32768 .. 32767], and
 we should multiply the resampler output by 32767/60/4*M_PI

 To make some room for numerical errors, filter ripples and
 quantization noise, so that they do not cause overflows in
 quantization, dynamic range is reduced in mzpokeysnd by
 multiplying the output amplitude with 0.95, reserving 5%
 of the total range for such effects, which is about 0.51db.

 Mentioned gain and DC values were tested with 17kHz audio
 playing synchronously on 4 channels, which showed to be
 utilizing 95% of the sample values range.

 Since any other gain value will be not optimal, I removed
 user gain setting and hard-coded the gain into mzpokeysnd

 ---

 A note from Piotr Fusik:
 I've added support for the key click sound generated by GTIA. Its
 volume seems to be pretty much like 8 on single POKEY's channel.
 So, the volumes now can sum up to 136 (4 channels * 15 * 2
 + 8 * 2 for GTIA), not 120.

 A note from Mark Grebe:
 I've added back in the console and sio sounds from the old
 pokey version.  So, now the volumes can sum up to 152
 (4 channesl * 15 * 2 + 8 * 4 for old sound), not 120 or 136.

 ******************************************************************/


/******************************************************************

          Quantization effects and dithering
              by Michael Borisov

 Quantization error in the signal has an expectation value of half
 the LSB, when the rounding is performed properly. Sometimes they
 express quantization error as a random function with even
 distribution over the range [-0.5 to 0.5]. Spectrum of this function
 is flat, because it's a white noise.

 Power of a discrete signal (including noise) is calculated as
 mean square of its samples. For the mentioned above noise
 this is approximately 0.33. Therefore, in decibels for 8-bit case,
 our noise will have power of 10*log10(0.33/256/256) = -53dB

 Because noise is white, this power of -53dB will be evenly
 distributed over the whole signal band upto Nyquist frequency.
 The larger the band is (higher sampling frequency), less
 is the quantisation noise floor. For 8000Hz noise floor is
 10*log10(0.33/256/256/4000) = -89dB/Hz, and for 44100Hz noise
 floor is 10*log10(0.33/256/256/22050) = -96.4dB/Hz.
 This shows that higher sampling rates are better in sense of
 quantization noise. Moreover, as large part of quantization noise
 in case of 44100Hz will fall into ultrasound and hi-frequency
 area 10-20kHz where human ear is less sensitive, this will
 show up as great improvement in quantization noise performance
 compared to 8000Hz.

 I was plotting spectral analysis for sounds produced by mzpokeysnd
 to check these measures. And it showed up that in 8-bit case
 there is no expected flat noise floor of -89db/Hz for 8000Hz,
 but some distortion spectral peaks had higher amplitude than
 the aliasing peaks in 16-bit case. This was a proof to another
 statement which says that quantization noise tends to become
 correlated with the signal. Correlation is especially strong
 for simple signals generated by Pokey. Correlation means that
 the noise power of -53db is no longer evenly distributed
 across the whole frequency range, but concentrates in larger
 peaks at locations which depend on the Pokey signal.

 To decorrelate quantization distortion and make it again
 white noise, which would improve the sound spectrum, since
 the maximum distortion peaks will have less amplitude,
 dithering is used. Another white noise is added to the signal
 before quantization. Since this added noise is not correlated
 with the signal, it shows itself as a flat noise floor.
 Quantization noise now tries to correlate with the dithering
 noise, but this does not lead to appearance of sharp
 spectral peaks any more :)

 Another thing is that for listening, white noise is better than
 distortion. This is because human hearing has some 'noise
 reduction' system which makes it easier to percept sounds
 on the white noise background.

 From the other point of view, if a signal has high and low
 spectral peaks, it is desirable that there is no distortion
 component with peaks of amplitude comparable to those of
 the true signal. Otherwise, perception of background low-
 amplitude signals will be disrupted. That's why they say
 that dithering extends dynamic range.

 Dithering does not eliminate correlation of quantization noise
 completely. Degree of reduction of this effect depends on
 the dithering noise power. The higher is dithering noise,
 the more quantization noise is decorrelated. But this also
 leads to increase of noise percepted by the listener. So, an
 optimum value should be selected. My experiments show that
 unbiased rand() noise of amplitude 0.25 LSB is doing well.

 Test spectral pictures for 8-bit sound, 8kHz sampling rate,
 dithered, show a noise floor of approx. -87dB/Hz.

 ******************************************************************/

#define MAX_SAMPLE 152
extern int atari_speaker;

static void Pokey_process_8(void* sndbuffer, unsigned sndn)
{
    unsigned short i;
    int nsam = sndn;
    uint8 *buffer = (uint8 *) sndbuffer;

    if(num_cur_pokeys<1)
        return; /* module was not initialized */

    /* if there are two pokeys, then the signal is stereo
       we assume even sndn */
    while(nsam >= (int) num_cur_pokeys)
    {
#ifdef VOL_ONLY_SOUND
        if( sampbuf_rptr!=sampbuf_ptr )
            { int l;
            if( sampbuf_cnt[sampbuf_rptr]>0 )
                sampbuf_cnt[sampbuf_rptr]-=1280;
            while(  (l=sampbuf_cnt[sampbuf_rptr])<=0 )
                {	sampout=sampbuf_val[sampbuf_rptr];
                        sampbuf_rptr++;
                        if( sampbuf_rptr>=SAMPBUF_MAX )
                                sampbuf_rptr=0;
                        if( sampbuf_rptr!=sampbuf_ptr )
                            {
                            sampbuf_cnt[sampbuf_rptr]+=l;
                            }
                        else	break;
                }
            }
#endif

#ifdef VOL_ONLY_SOUND
        buffer[0] = (uint8)floor((generate_sample(pokey_states) + sampout - MAX_SAMPLE / 2.0)
         * (255.0 / MAX_SAMPLE / 4 * M_PI * 0.95) + 128 + 0.5 + 0.5 * rand() / RAND_MAX - 0.25);
#else
        buffer[0] = (uint8)floor((generate_sample(pokey_states) - MAX_SAMPLE / 2.0)
         * (255.0 / MAX_SAMPLE / 4 * M_PI * 0.95) + 128 + 0.5 + 0.5 * rand() / RAND_MAX - 0.25);
#endif
        for(i=1; i<num_cur_pokeys; i++)
        {
            buffer[i] = (uint8)floor((generate_sample(pokey_states + i) - MAX_SAMPLE / 2.0)
             * (255.0 / MAX_SAMPLE / 4 * M_PI * 0.95) + 128 + 0.5 + 0.5 * rand() / RAND_MAX - 0.25);
        }
        buffer += num_cur_pokeys;
        nsam -= num_cur_pokeys;
    }
}

static void Pokey_process_16(void* sndbuffer, unsigned sndn)
{
    unsigned short i;
    int nsam = sndn;
    int16 *buffer = (int16 *) sndbuffer;

    if(num_cur_pokeys<1)
        return; /* module was not initialized */

    /* if there are two pokeys, then the signal is stereo
       we assume even sndn */
    while(nsam >= (int) num_cur_pokeys)
    {
#ifdef VOL_ONLY_SOUND
        if( sampbuf_rptr!=sampbuf_ptr )
            { int l;
            if( sampbuf_cnt[sampbuf_rptr]>0 )
                sampbuf_cnt[sampbuf_rptr]-=1280;
            while(  (l=sampbuf_cnt[sampbuf_rptr])<=0 )
                {	sampout=sampbuf_val[sampbuf_rptr];
                        sampbuf_rptr++;
                        if( sampbuf_rptr>=SAMPBUF_MAX )
                                sampbuf_rptr=0;
                        if( sampbuf_rptr!=sampbuf_ptr )
                            {
                            sampbuf_cnt[sampbuf_rptr]+=l;
                            }
                        else	break;
                }
            }
#endif
#ifdef VOL_ONLY_SOUND
        buffer[0] = (int16)floor((generate_sample(pokey_states) + sampout - MAX_SAMPLE / 2.0)
         * (65535.0 / MAX_SAMPLE / 4 * M_PI * 0.95) + 0.5 + 0.5 * rand() / RAND_MAX - 0.25);
#else
        buffer[0] = (int16)floor((generate_sample(pokey_states) - MAX_SAMPLE / 2.0)
         * (65535.0 / MAX_SAMPLE / 4 * M_PI * 0.95) + 0.5 + 0.5 * rand() / RAND_MAX - 0.25);
#endif
        for(i=1; i<num_cur_pokeys; i++)
        {
            buffer[i] = (int16)floor((generate_sample(pokey_states + i) - MAX_SAMPLE / 2.0)
             * (65535.0 / MAX_SAMPLE / 4 * M_PI * 0.95) + 0.5 + 0.5 * rand() / RAND_MAX - 0.25);
        }
        buffer += num_cur_pokeys;
        nsam -= num_cur_pokeys;
    }
}

#ifdef SERIO_SOUND
static void Update_serio_sound_mz( int out, UBYTE data )
{
#ifdef VOL_ONLY_SOUND
   int bits,pv,future;
        if (!serio_sound_enabled) return;

	pv=0;
	future=0;
	bits= (data<<1) | 0x200;
	while( bits )
	{
		sampbuf_lastval-=pv;
		pv=(bits&0x01)*pokey_states[0].vol3;
		sampbuf_lastval+=pv;

	sampbuf_val[sampbuf_ptr]=sampbuf_lastval;
	sampbuf_cnt[sampbuf_ptr]=
		(cpu_clock+future-sampbuf_last)*128*samp_freq/178979;
	sampbuf_last=cpu_clock+future;
	sampbuf_ptr++;
	if( sampbuf_ptr>=SAMPBUF_MAX )
		sampbuf_ptr=0;
	if( sampbuf_ptr==sampbuf_rptr )
	{	sampbuf_rptr++;
		if( sampbuf_rptr>=SAMPBUF_MAX )
			sampbuf_rptr=0;
	}
			/* 1789790/19200 = 93 */
		future+=93;	/* ~ 19200 bit/s - FIXME!!! set speed form AUDF [2] ??? */
		bits>>=1;
	}
	sampbuf_lastval-=pv;
#endif  /* VOL_ONLY_SOUND */
}
#endif /* SERIO_SOUND */

#ifdef CONSOLE_SOUND
static void Update_consol_sound_mz( int set )
{
#ifdef VOL_ONLY_SOUND
  static int prev_atari_speaker=0;
  static unsigned int prev_cpu_clock=0;
  int d;
        if (!console_sound_enabled) return;

	if( !set && samp_consol_val==0 )	return;
	sampbuf_lastval-=samp_consol_val;
	if( prev_atari_speaker!=atari_speaker )
	{	samp_consol_val=atari_speaker*8*4;	/* gain */
		prev_cpu_clock=cpu_clock;
	}
	else if( !set )
	{	d=cpu_clock - prev_cpu_clock;
		if( d<114 )
		{	sampbuf_lastval+=samp_consol_val;   return;	}
		while( d>=114 /* CPUL */ )
		{	samp_consol_val=samp_consol_val*99/100;
			d-=114;
		}
		prev_cpu_clock=cpu_clock-d;
	}
	sampbuf_lastval+=samp_consol_val;
	prev_atari_speaker=atari_speaker;

	sampbuf_val[sampbuf_ptr]=sampbuf_lastval;
	sampbuf_cnt[sampbuf_ptr]=
		(cpu_clock-sampbuf_last)*128*samp_freq/178979;
	sampbuf_last=cpu_clock;
	sampbuf_ptr++;
	if( sampbuf_ptr>=SAMPBUF_MAX )
		sampbuf_ptr=0;
	if( sampbuf_ptr==sampbuf_rptr )
	{	sampbuf_rptr++;
		if( sampbuf_rptr>=SAMPBUF_MAX )
			sampbuf_rptr=0;
	}
#endif  /* VOL_ONLY_SOUND */
}
#endif

#ifdef VOL_ONLY_SOUND
static void Update_vol_only_sound_mz( void )
{
#ifdef CONSOLE_SOUND
	Update_consol_sound(0);	/* mmm */
#endif /* CONSOLE_SOUND */
}
#endif
