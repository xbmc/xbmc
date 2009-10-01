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

#ifndef ___MBLOCK_H_
#define ___MBLOCK_H_


/* Memory block for decreasing malloc
 *
 * +------+    +------+             +-------+
 * |BLOCK1|--->|BLOCK2|---> ... --->|BLOCK N|---> NULL
 * +------+    +------+             +-------+
 *
 *
 * BLOCK:
 * +-----------------------+
 * | memory 1              |
 * |                       |
 * +-----------------------+
 * | memory 2              |
 * +-----------------------+
 * | memory 3              |
 * |                       |
 * |                       |
 * +-----------------------+
 * | unused ...            |
 * +-----------------------+
 */


#define MIN_MBLOCK_SIZE 8192

typedef struct _MBlockNode
{
    size_t block_size;
    size_t offset;
    struct _MBlockNode *next;
#ifndef MBLOCK_NOPAD
    void *pad;
#endif /* MBLOCK_NOPAD */
    char buffer[1];
} MBlockNode;

typedef struct _MBlockList
{
    MBlockNode *first;
    size_t allocated;
} MBlockList;

extern void init_mblock(MBlockList *mblock);
extern void *new_segment(MBlockList *mblock, size_t nbytes);
extern void reuse_mblock(MBlockList *mblock);
extern char *strdup_mblock(MBlockList *mblock, const char *str);
extern int free_global_mblock(void);
extern void free_global(void);

#endif /* ___MBLOCK_H_ */
