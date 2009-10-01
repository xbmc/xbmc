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
 * SBK --> SF2 Conversion
 *
 * Copyright (C) 1996,1997 Takashi Iwai
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *================================================================*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#include <stdio.h>
#include <math.h>
#include "timidity.h"
#include "common.h"
#include "sffile.h"
#include "sfitem.h"


/*----------------------------------------------------------------
 * prototypes
 *----------------------------------------------------------------*/

static int sbk_cutoff(int gen, int val);
static int sbk_filterQ(int gen, int val);
static int sbk_tenpct(int gen, int val);
static int sbk_panpos(int gen, int val);
static int sbk_atten(int gen, int val);
static int sbk_scale(int gen, int val);
static int sbk_time(int gen, int val);
static int sbk_tm_key(int gen, int val);
static int sbk_freq(int gen, int val);
static int sbk_pshift(int gen, int val);
static int sbk_cshift(int gen, int val);
static int sbk_tremolo(int gen, int val);
static int sbk_volsust(int gen, int val);
static int sbk_modsust(int gen, int val);

/*----------------------------------------------------------------
 * convertor function table
 *----------------------------------------------------------------*/

static SBKConv sbk_convertors[T_EOT] = {
	NULL, NULL, NULL, NULL, NULL,

	sbk_cutoff, sbk_filterQ, sbk_tenpct, sbk_panpos, sbk_atten, sbk_scale,

	sbk_time, sbk_tm_key, sbk_freq, sbk_pshift, sbk_cshift,
	sbk_tremolo, sbk_modsust, sbk_volsust,
};


/*----------------------------------------------------------------
 * sbk --> sf2 conversion
 *----------------------------------------------------------------*/

int sbk_to_sf2(int oper, int amount)
{
	LayerItem *item = &layer_items[oper];
	if (item->type < 0 || item->type >= T_EOT) {
		fprintf(stderr, "illegal gen item type %d\n", item->type);
		return amount;
	}
	if (sbk_convertors[item->type])
		return sbk_convertors[item->type](oper, amount);
	return amount;
}

/*----------------------------------------------------------------
 * conversion rules for each type
 *----------------------------------------------------------------*/

/* initial cutoff */
static int sbk_cutoff(int gen, int val)
{
	if (val == 127)
		return 14400;
	else
		return 59 * val + 4366;
	/*return 50 * val + 4721;*/
}

/* initial resonance */
static int sbk_filterQ(int gen, int val)
{
	return val * 3 / 2;
}

/* chorus/reverb */
static int sbk_tenpct(int gen, int val)
{
	return val * 1000 / 256;
}

/* pan position */
static int sbk_panpos(int gen, int val)
{
	return val * 1000 / 127 - 500;
}

/* initial attenuation */
static int sbk_atten(int gen, int val)
{
	if (val == 0)
		return 1000;
	return (int)(-200.0 * log10((double)val / 127.0) * 10);
}

/* scale tuning */
static int sbk_scale(int gen, int val)
{
	return (val ? 50 : 100);
}

/* env/lfo time parameter */
static int sbk_time(int gen, int val)
{
	if (val <= 0) val = 1;
	return (int)(log((double)val / 1000.0) / log(2.0) * 1200.0);
}

/* time change per key */
static int sbk_tm_key(int gen, int val)
{
	return (int)(val * 5.55);
}

/* lfo frequency */
static int sbk_freq(int gen, int val)
{
	if (val == 0) {
		if (gen == SF_freqLfo1) return -725;
		else /* SF_freqLfo2*/ return -15600;
	}
	/*return (int)(3986.0 * log10((double)val) - 7925.0);*/
	return (int)(1200 * log10((double)val) / log10(2.0) - 7925.0);

}

/* lfo/env pitch shift */
static int sbk_pshift(int gen, int val)
{
	return (1200 * val / 64 + 1) / 2;
}

/* lfo/env cutoff freq shift */
static int sbk_cshift(int gen, int val)
{
	if (gen == SF_lfo1ToFilterFc)
		return (1200 * 3 * val) / 64;
	else
		return (1200 * 6 * val) / 64;
}

/* lfo volume shift */
static int sbk_tremolo(int gen, int val)
{
	return (120 * val) / 64;
}

/* mod env sustain */
static int sbk_modsust(int gen, int val)
{
	if (val < 96)
		return 1000 * (96 - val) / 96;
	else
		return 0;
}

/* vol env sustain */
static int sbk_volsust(int gen, int val)
{
	if (val < 96)
		return (2000 - 21 * val) / 2;
	else
		return 0;
}
