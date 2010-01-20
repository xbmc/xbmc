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

#ifndef ___VT100_H_
#define ___VT100_H_

#define VT100_COLS 80
#define VT100_ROWS 24

#define VT100_ATTR_UNDERLINE	000000400000
#define VT100_ATTR_REVERSE	000001000000
#define VT100_ATTR_BOLD		000010000000

/* vt100 utility functions */
extern void vt100_init_screen(void);
extern void vt100_refresh(void);
extern void vt100_clrtoeol(void);
extern void vt100_move(int row, int col);
extern void vt100_set_attr(int attr);
extern void vt100_reset_attr(void);

#endif /* ___VT100_H_ */
