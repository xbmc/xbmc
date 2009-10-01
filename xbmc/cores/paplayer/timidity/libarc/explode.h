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

#ifndef ___EXPLODE_H_
#define ___EXPLODE_H_

typedef struct _ExplodeHandler *ExplodeHandler;

enum explode_method_t
{
    EXPLODE_LIT8,
    EXPLODE_LIT4,
    EXPLODE_NOLIT8,
    EXPLODE_NOLIT4
};

extern ExplodeHandler open_explode_handler(
	long (* read_func)(char *buf, long size, void *user_val),
	int method,
	long compsize, long origsize,
	void *user_val);

extern long explode(ExplodeHandler decoder,
		    char *decode_buff,
		    long decode_buff_size);

extern void close_explode_handler(ExplodeHandler decoder);


#endif /* ___EXPLODE_H_ */
