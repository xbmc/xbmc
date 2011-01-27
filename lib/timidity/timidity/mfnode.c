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

    mfnode.c

	Macintosh interface for TiMidity
	by T.Nogami	<t-nogami@happy.email.ne.jp>
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#ifndef NO_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif

#include "timidity.h"
#include "common.h"
#include "controls.h"

#define ENTITY 1    /*EXTERN*/
#include "mfnode.h"

Instr_comment instr_comment[MAX_CHANNELS];

MFnode *make_new_MFnode_entry(char *file)
{
    struct midi_file_info *infop;
#ifdef MIDI_TITLE
    char *title = NULL;
#endif

    if(!strcmp(file, "-"))
	infop = get_midi_file_info("-", 1);
    else
    {
#ifdef MIDI_TITLE
	title = get_midi_title(file);
#else
	if(check_midi_file(file) < 0)
	    return NULL;
#endif /* MIDI_TITLE */
	infop = get_midi_file_info(file, 0);
    }

    if(!strcmp(file, "-") || (infop && infop->format >= 0))
    {
	MFnode *mfp;
	mfp = (MFnode *)safe_malloc(sizeof(MFnode));
	memset(mfp, 0, sizeof(MFnode));
#ifdef MIDI_TITLE
	mfp->title = title;
#endif /* MIDI_TITLE */
	mfp->file = safe_strdup(url_unexpand_home_dir(file));
	mfp->infop = infop;
	return mfp;
    }

    ctl->cmsg(CMSG_WARNING, VERB_NORMAL, "%s: Not a midi file (Ignored)",
	url_unexpand_home_dir(file));
    return NULL;
}

void indicator_set_prog(int ch, int val, char *comm)
{
    instr_comment[ch].comm = comm;
    instr_comment[ch].prog = val;
    instr_comment[ch].last_note_on = 0.0;
}

/*char *channel_instrum_name(int ch)
{
    char *comm;
    int bank;

    if(ISDRUMCHANNEL(ch))
		return "dram";
    if(channel[ch].program == SPECIAL_PROGRAM)
		return "Special Program";

    if(IS_CURRENT_MOD_FILE)
    {
		int pr;
		pr = channel[ch].special_sample;
		if(pr > 0 &&
			special_patch[pr] != NULL &&
			special_patch[pr]->name != NULL)
				return special_patch[pr]->name;
		return "MOD";
    }

    bank = channel[ch].bank;
    if(tonebank[bank] == NULL)
		bank = 0;
    comm = tonebank[bank]->tone[channel[ch].program].comment;
    if(comm == NULL)
		comm = tonebank[0]->tone[channel[ch].program].comment;
    return comm;
}
*/
