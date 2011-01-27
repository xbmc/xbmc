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

extern float pitch_freq_table[129];
extern float pitch_freq_ub_table[129];
extern float pitch_freq_lb_table[129];
extern int chord_table[4][3][3];

extern float freq_fourier(Sample *sp, int *chord);
extern int assign_pitch_to_freq(float freq);

#define CHORD_MAJOR 0
#define CHORD_MINOR 3
#define CHORD_DIM   6
#define CHORD_FIFTH 9
#define LOWEST_PITCH 0
#define HIGHEST_PITCH 127
