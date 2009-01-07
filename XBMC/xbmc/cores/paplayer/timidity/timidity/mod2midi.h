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

    mod2midi.h

    Header file for sample info -> MIDI event conversion

 */

#include "unimod.h"

void Voice_SetVolume (UBYTE, UWORD);
void Voice_SetFrequency (UBYTE, ULONG);
void Voice_SetPanning (UBYTE, ULONG);
void Voice_Play (UBYTE, SAMPLE *, ULONG);
void Voice_Stop (UBYTE);
BOOL Voice_Stopped (UBYTE);
void Voice_NewTempo (UWORD, UWORD);
void Voice_TickDone ();
void Voice_StartPlaying ();
void Voice_EndPlaying ();
void load_module_samples (SAMPLE *, int, int);
void Voice_SetPeriod (UBYTE v, ULONG period);
