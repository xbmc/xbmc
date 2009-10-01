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

   filter.h : written by Vincent Pagel ( pagel@loria.fr )

   implements fir antialiasing filter : should help when setting sample
   rates as low as 8Khz.

   */

#ifndef ___FILTER_H_
#define ___FILTER_H_

/* Order of the FIR filter = 20 should be enough ! */
#define ORDER 20
#define ORDER2 ORDER/2

void antialiasing(int16 *data, int32 data_length,
		  int32 sample_rate, int32 output_rate);

#endif /* ___FILTER_H_ */
