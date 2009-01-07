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

#ifndef ___STRTAB_H_
#define ___STRTAB_H_

#include "mblock.h"
typedef struct _StringTableNode
{
    struct _StringTableNode *next;
    char string[1]; /* variable length ('\0' terminated) */
} StringTableNode;

typedef struct _StringTable
{
    StringTableNode *head;
    StringTableNode *tail;
    uint16 nstring;
    MBlockList pool;
} StringTable;

extern void init_string_table(StringTable *stab);
extern StringTableNode *put_string_table(StringTable *stab,
					 char *str, int len);
extern void delete_string_table(StringTable *stab);
extern char **make_string_array(StringTable *stab);

#endif /* ___STRTAB_H_ */
