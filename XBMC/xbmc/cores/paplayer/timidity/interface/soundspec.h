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

#ifndef ___SOUNDSPEC_H_
#define ___SOUNDSPEC_H_

extern void open_soundspec(void);
extern void close_soundspec(void);
extern void soundspec_setinterval(double interval_sec);
extern void soundspec_update_wave(int32 *buff, int samples);
extern void soundspec_reinit(void);

extern int view_soundspec_flag;
extern int ctl_speana_flag;

#endif /* ___SOUNDSPEC_H_ */
