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

#ifndef ___SFLAYER_H_
#define ___SFLAYER_H_

/*================================================================
 * sflayer.h
 *	SoundFont layer structure
 *================================================================*/

enum {
	SF_startAddrs,		/* 0 sample start address -4 (0to*0xffffff)*/
        SF_endAddrs,		/* 1 */
        SF_startloopAddrs,	/* 2 loop start address -4 (0 to * 0xffffff) */
        SF_endloopAddrs,	/* 3 loop end address -3 (0 to * 0xffffff) */
        SF_startAddrsHi,	/* 4 high word of startAddrs */
        SF_lfo1ToPitch,		/* 5 main fm: lfo1-> pitch */
        SF_lfo2ToPitch,		/* 6 aux fm:  lfo2-> pitch */
        SF_env1ToPitch,		/* 7 pitch env: env1(aux)-> pitch */
        SF_initialFilterFc,	/* 8 initial filter cutoff */
        SF_initialFilterQ,	/* 9 filter Q */
        SF_lfo1ToFilterFc,	/* 10 filter modulation: lfo1->filter*cutoff */
        SF_env1ToFilterFc,	/* 11 filter env: env1(aux)->filter * cutoff */
        SF_endAddrsHi,		/* 12 high word of endAddrs */
        SF_lfo1ToVolume,	/* 13 tremolo: lfo1-> volume */
        SF_env2ToVolume,	/* 14 Env2Depth: env2-> volume */
        SF_chorusEffectsSend,	/* 15 chorus */
        SF_reverbEffectsSend,	/* 16 reverb */
        SF_panEffectsSend,	/* 17 pan */
        SF_auxEffectsSend,	/* 18 pan auxdata (internal) */
        SF_sampleVolume,	/* 19 used internally */
        SF_unused3,		/* 20 */
        SF_delayLfo1,		/* 21 delay 0x8000-n*(725us) */
        SF_freqLfo1,		/* 22 frequency */
        SF_delayLfo2,		/* 23 delay 0x8000-n*(725us) */
        SF_freqLfo2,		/* 24 frequency */
        SF_delayEnv1,		/* 25 delay 0x8000 - n(725us) */
        SF_attackEnv1,		/* 26 attack */
        SF_holdEnv1,		/* 27 hold */
        SF_decayEnv1,		/* 28 decay */
        SF_sustainEnv1,		/* 29 sustain */
        SF_releaseEnv1,		/* 30 release */
        SF_autoHoldEnv1,	/* 31 */
        SF_autoDecayEnv1,	/* 32 */
        SF_delayEnv2,		/* 33 delay 0x8000 - n(725us) */
        SF_attackEnv2,		/* 34 attack */
        SF_holdEnv2,		/* 35 hold */
        SF_decayEnv2,		/* 36 decay */
        SF_sustainEnv2,		/* 37 sustain */
        SF_releaseEnv2,		/* 38 release */
        SF_autoHoldEnv2,	/* 39 */
        SF_autoDecayEnv2,	/* 40 */
        SF_instrument,		/* 41 */
        SF_nop,			/* 42 */
        SF_keyRange,		/* 43 */
        SF_velRange,		/* 44 */
        SF_startloopAddrsHi,	/* 45 high word of startloopAddrs */
        SF_keynum,		/* 46 */
        SF_velocity,		/* 47 */
        SF_initAtten,		/* 48 */
        SF_keyTuning,		/* 49 */
        SF_endloopAddrsHi,	/* 50 high word of endloopAddrs */
        SF_coarseTune,		/* 51 */
        SF_fineTune,		/* 52 */
        SF_sampleId,		/* 53 */
        SF_sampleFlags,		/* 54 */
        SF_samplePitch,		/* 55 SF1 only */
        SF_scaleTuning,		/* 56 */
        SF_keyExclusiveClass,	/* 57 */
        SF_rootKey,		/* 58 */
	SF_EOF			/* 59 */
};


/* name strings */
extern char *sf_gen_text[SF_EOF];


/*----------------------------------------------------------------
 * layer value table
 *----------------------------------------------------------------*/

typedef struct _LayerTable {
	short val[SF_EOF];
	char set[SF_EOF];
} LayerTable;


#endif /* ___SFLAYER_H_ */
