/*
    TiMidity++ -- MIDI to WAVE converter and player
    Copyright (C) 1999-2002 Masanao Izumo <mo@goice.co.jp>
    Copyright (C) 1995 Tuukka Toivonen <tt@cgs.fi>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/*================================================================
 * sfitem.h
 *	soundfont generator conversion table
 *================================================================*/

#ifndef SFITEM_H_DEF
#define SFITEM_H_DEF

#include "sflayer.h"
#include "sffile.h"

typedef struct _LayerItem {
	int copy;	/* copy policy */
	int type;	/* conversion type */
	int minv;	/* minimum value */
	int maxv;	/* maximum value */
	int defv;	/* default value */
} LayerItem;

/* copy policy */
enum {
	L_INHRT,	/* add to global */
	L_OVWRT,	/* overwrite on global */
	L_RANGE,	/* range */
	L_PRSET,	/* preset only */
	L_INSTR		/* instrument only */
};

/* data type */
enum {
	T_NOP,		/* nothing */
	T_NOCONV,	/* no conversion */
	T_OFFSET,	/* address offset */
	T_HI_OFF,	/* address coarse offset (32k) */
	T_RANGE,	/* range; composite values (0-127/0-127) */

	T_CUTOFF,	/* initial cutoff */
	T_FILTERQ,	/* initial resonance */
	T_TENPCT,	/* effects send */
	T_PANPOS,	/* panning position */
	T_ATTEN,	/* initial attenuation */
	T_SCALE,	/* scale tuning */

	T_TIME,		/* envelope/LFO time */
	T_TM_KEY,	/* time change per key */
	T_FREQ,		/* LFO frequency */
	T_PSHIFT,	/* env/LFO pitch shift */
	T_CSHIFT,	/* env/LFO cutoff shift */
	T_TREMOLO,	/* LFO tremolo shift */
	T_MODSUST,	/* modulation env sustain level */
	T_VOLSUST,	/* volume env sustain level */

	T_EOT		/* end of type */
};

/* sbk->sf2 convertor function */
typedef int (*SBKConv)(int gen, int amount);

/* macros for range operation */
#define RANGE(lo,hi)	(((hi)&0xff) << 8 | ((lo)&0xff))
#define LOWNUM(val)	((val) & 0xff)
#define HIGHNUM(val)	(((val) >> 8) & 0xff)

/* layer type definitions */
extern LayerItem layer_items[SF_EOF];

extern int sbk_to_sf2(int oper, int amount);

#endif
