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
 * sfitem.c
 *	soundfont generator table definition
 *================================================================*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#include <stdio.h>
#include "timidity.h"
#include "common.h"
#include "sflayer.h"
#include "sfitem.h"

/* layer type definitions */
LayerItem layer_items[SF_EOF] = {
	{L_INHRT, T_OFFSET, 0, 0, 0}, /* startAddrs */
	{L_INHRT, T_OFFSET, 0, 0, 0}, /* endAddrs */
	{L_INHRT, T_OFFSET, 0, 0, 0}, /* startloopAddrs */
	{L_INHRT, T_OFFSET, 0, 0, 0}, /* endloopAddrs */
	{L_INHRT, T_HI_OFF, 0, 0, 0}, /* startAddrsHi */
	{L_INHRT, T_PSHIFT, -12000, 12000, 0}, /* lfo1ToPitch */
	{L_INHRT, T_PSHIFT, -12000, 12000, 0}, /* lfo2ToPitch */
	{L_INHRT, T_PSHIFT, -12000, 12000, 0}, /* env1ToPitch */
	{L_INHRT, T_CUTOFF, 1500, 13500, 13500}, /* initialFilterFc */
	{L_INHRT, T_FILTERQ, 0, 960, 0}, /* initialFilterQ */
	{L_INHRT, T_CSHIFT, -12000, 12000, 0}, /* lfo1ToFilterFc */
	{L_INHRT, T_CSHIFT, -12000, 12000, 0}, /* env1ToFilterFc */
	{L_INHRT, T_HI_OFF, 0, 0, 0}, /* endAddrsHi */
	{L_INHRT, T_TREMOLO, -960, 960, 0}, /* lfo1ToVolume */
	{L_INHRT, T_NOP, 0, 0, 0}, /* env2ToVolume / unused1 */
	{L_INHRT, T_TENPCT, 0, 1000, 0}, /* chorusEffectsSend */
	{L_INHRT, T_TENPCT, 0, 1000, 0}, /* reverbEffectsSend */
	{L_INHRT, T_PANPOS, 0, 1000, 0}, /* panEffectsSend */
	{L_INHRT, T_NOP, 0, 0, 0}, /* unused */
	{L_INHRT, T_NOP, 0, 0, 0}, /* sampleVolume / unused */
	{L_INHRT, T_NOP, 0, 0, 0}, /* unused3 */
	{L_INHRT, T_TIME, -12000, 5000, -12000}, /* delayLfo1 */
	{L_INHRT, T_FREQ, -16000, 4500, 0}, /* freqLfo1 */
	{L_INHRT, T_TIME, -12000, 5000, -12000}, /* delayLfo2 */
	{L_INHRT, T_FREQ, -16000, 4500, 0}, /* freqLfo2 */
	{L_INHRT, T_TIME, -12000, 5000, -12000}, /* delayEnv1 */
	{L_INHRT, T_TIME, -12000, 5000, -12000}, /* attackEnv1 */
	{L_INHRT, T_TIME, -12000, 5000, -12000}, /* holdEnv1 */
	{L_INHRT, T_TIME, -12000, 5000, -12000}, /* decayEnv1 */
	{L_INHRT, T_MODSUST, 0, 1000, 0}, /* sustainEnv1 */
	{L_INHRT, T_TIME, -12000, 5000, -12000}, /* releaseEnv1 */
	{L_INHRT, T_TM_KEY, -1200, 1200, 0}, /* autoHoldEnv1 */
	{L_INHRT, T_TM_KEY, -1200, 1200, 0}, /* autoDecayEnv1 */
	{L_INHRT, T_TIME, -12000, 5000, -12000}, /* delayEnv2 */
	{L_INHRT, T_TIME, -12000, 5000, -12000}, /* attackEnv2 */
	{L_INHRT, T_TIME, -12000, 5000, -12000}, /* holdEnv2 */
	{L_INHRT, T_TIME, -12000, 5000, -12000}, /* decayEnv2 */
	{L_INHRT, T_VOLSUST, 0, 1440, 0}, /* sustainEnv2 */
	{L_INHRT, T_TIME, -12000, 5000, -12000}, /* releaseEnv2 */
	{L_INHRT, T_TM_KEY, -1200, 1200, 0}, /* autoHoldEnv2 */
	{L_INHRT, T_TM_KEY, -1200, 1200, 0}, /* autoDecayEnv2 */
	{L_PRSET, T_NOCONV, 0, 0, 0}, /* instrument */
	{L_INHRT, T_NOP, 0, 0, 0}, /* nop */
	{L_RANGE, T_RANGE, 0, 0, RANGE(0,127)}, /* keyRange */
	{L_RANGE, T_RANGE, 0, 0, RANGE(0,127)}, /* velRange */
	{L_INHRT, T_HI_OFF, 0, 0, 0}, /* startloopAddrsHi */
	{L_OVWRT, T_NOCONV, 0, 127, -1}, /* keynum */
	{L_OVWRT, T_NOCONV, 0, 127, -1}, /* velocity */
	{L_INHRT, T_ATTEN, 0, 1440, 0}, /* initialAttenuation */
	{L_INHRT, T_NOP, 0, 0, 0}, /* keyTuning */
	{L_INHRT, T_HI_OFF, 0, 0, 0}, /* endloopAddrsHi */
	{L_INHRT, T_NOCONV, -120, 120, 0}, /* coarseTune */
	{L_INHRT, T_NOCONV, -99, 99, 0}, /* fineTune */
	{L_INSTR, T_NOCONV, 0, 0, 0}, /* sampleId */
	{L_OVWRT, T_NOCONV, 0, 3, 0}, /* sampleFlags */
	{L_OVWRT, T_NOCONV, 0, 0, 0}, /* samplePitch (only in SBK) */
	{L_INHRT, T_SCALE, 0, 1200, 100}, /* scaleTuning */
	{L_OVWRT, T_NOCONV, 0, 127, 0}, /* keyExclusiveClass */
	{L_OVWRT, T_NOCONV, 0, 127, -1}, /* rootKey */
};

