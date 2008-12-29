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

    mfnode.h

	Macintosh interface for TiMidity
	by T.Nogami	<t-nogami@happy.email.ne.jp>
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#ifndef MFNODE_H
#define MFNODE_H

#include "timidity.h"
#include "instrum.h"
#include "playmidi.h"
#include "readmidi.h"

#define MIDI_TITLE

/* list_mode */
typedef struct _MFnode
{
    char *file;
#ifdef MIDI_TITLE
    char *title;
#endif /* MIDI_TITLE */
    struct midi_file_info *infop;
    struct _MFnode *next;
} MFnode;

typedef struct
{
    int prog;
    int disp_cnt;
    double last_note_on;
    char *comm;
}Instr_comment;

// externs

#if ENTITY
 #define EXTERN /*entitiy*/
#else
 #define EXTERN  extern 
#endif
EXTERN Instr_comment instr_comment[];
EXTERN char *comment_indicator_buffer
 #if ENTITY
  = NULL
 #endif
 ;

MFnode	*make_new_MFnode_entry(char *file);
void	indicator_set_prog(int ch, int val, char *comm);
void	reset_indicator(void);
char	*channel_instrum_name(int ch);

#endif // MFNODE_H
