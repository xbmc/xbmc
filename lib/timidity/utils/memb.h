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

#ifndef ___MEMB_H_
#define ___MEMB_H_

#include "mblock.h"
#include "url.h"
typedef struct _MemBufferNode
{
    struct _MemBufferNode *next; /* link to next buffer node */
    int size;			/* size of base */
    int pos;			/* current read position */
    char base[1];
} MemBufferNode;
#define MEMBASESIZE (MIN_MBLOCK_SIZE - sizeof(MemBufferNode))

typedef struct _MemBuffer
{
    MemBufferNode *head;	/* start buffer node pointer */
    MemBufferNode *tail;	/* last buffer node pointer */
    MemBufferNode *cur;		/* current buffer node pointer */
    long total_size;
    MBlockList pool;
} MemBuffer;

extern void init_memb(MemBuffer *b);
extern void push_memb(MemBuffer *b, char *buff, long buff_size);
extern long read_memb(MemBuffer *b, char *buff, long buff_size);
extern long skip_read_memb(MemBuffer *b, long size);
extern void rewind_memb(MemBuffer *b);
extern void delete_memb(MemBuffer *b);
extern URL memb_open_stream(MemBuffer *b, int autodelete);

#endif /* ___MEMB_H_ */
