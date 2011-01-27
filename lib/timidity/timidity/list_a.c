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

    list_a.c - list MIDI programs
*/ 

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#include <stdio.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#include <stdlib.h>

#ifndef NO_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif

#include "timidity.h"
#include "common.h"
#include "instrum.h"
#include "playmidi.h"
#include "readmidi.h"
#include "output.h"
#include "controls.h"

static int open_output(void);
static void close_output(void);
static int acntl(int request, void *arg);

static int32 tonebank_start_time[128][128];
static int32 drumset_start_time[128][128];
static int tonebank_counter[128][128];
static int drumset_counter[128][128];
extern Channel channel[MAX_CHANNELS];

#define dmp list_play_mode

PlayMode dmp =
{
    DEFAULT_RATE, 0, PF_MIDI_EVENT,
    -1,
    {0,0,0,0,0},
    "List MIDI event", 'l',
    "-",
    open_output,
    close_output,
    NULL,
    acntl
};

static int open_output(void)
{
    dmp.fd = 0;
    return 0;
}

static char *time_str(int t)
{
    static char buff[32];

    t = (int)((double)t / (double)play_mode->rate + 0.5);
    sprintf(buff, "%d:%02d", t / 60, t % 60);
    return buff;
}

static void close_output(void)
{
    dmp.fd = -1;
}

static void start_list_a(void)
{
    int i, j;

    for(i = 0; i < 128; i++)
	for(j = 0; j < 128; j++)
	{
	    tonebank_start_time[i][j] = -1;
	    drumset_start_time[i][j] = -1;
	}
    memset(tonebank_counter, 0, sizeof(tonebank_counter));
    memset(drumset_counter, 0, sizeof(drumset_counter));
    memset(channel, 0, sizeof(channel));
    change_system_mode(DEFAULT_SYSTEM_MODE);
}

static void end_list_a(void)
{
    int i, j;

    ctl->cmsg(CMSG_TEXT, VERB_NORMAL,
	      "==== %s ====", current_file_info->filename);
    for(i = 0; i < 128; i++)
	for(j = 0; j < 128; j++)
	    if(tonebank_start_time[i][j] != -1)
	    {
		ctl->cmsg(CMSG_TEXT, VERB_NORMAL,
		    "Tonebank %d %d (start at %s, %d times note on)",
		    i, j,
		    time_str(tonebank_start_time[i][j]),
		    tonebank_counter[i][j]);
	    }
    for(i = 0; i < 128; i++)
	for(j = 0; j < 128; j++)
	    if(drumset_start_time[i][j] != -1)
	    {
		ctl->cmsg(CMSG_TEXT, VERB_NORMAL,
		    "Drumset %d %d (start at %s, %d times note on)",
		    i, j,
		    time_str(drumset_start_time[i][j]),
		    drumset_counter[i][j]);
	    }
}


static int do_event(MidiEvent *ev)
{
    int ch;

    ch = ev->channel;
    switch(ev->type)
    {
      case ME_NOTEON:
	if(ev->b)
	{
	    /* Note on */
	    int bank;
	    int inst;

	    bank = channel[ch].bank;
	    if(ISDRUMCHANNEL(ch))
	    {
		inst = ev->a;
		if(drumset_start_time[bank][inst] == -1)
		{
		    drumset_start_time[bank][inst] = ev->time;
		}
		drumset_counter[bank][inst]++;
	    }
	    else
	    {
		inst = channel[ch].program;
		if(tonebank_start_time[bank][inst] == -1)
		    tonebank_start_time[bank][inst] = ev->time;
		tonebank_counter[bank][inst]++;
	    }
	}
	break;
      case ME_PROGRAM:
	midi_program_change(ch, ev->a);
	break;
      case ME_TONE_BANK_LSB:
	channel[ch].bank_lsb = ev->a;
	break;
      case ME_TONE_BANK_MSB:
	channel[ch].bank_msb = ev->a;
	break;
      case ME_RESET:
	change_system_mode(ev->a);
	memset(channel, 0, sizeof(channel));
	break;
      case ME_EOT:
	return RC_TUNE_END;
    }
    return RC_NONE;
}

static int acntl(int request, void *arg)
{
    switch(request)
    {
      case PM_REQ_MIDI:
	return do_event((MidiEvent *)arg);
      case PM_REQ_DISCARD:
	return 0;
      case PM_REQ_PLAY_START:
	start_list_a();
	return 0;
      case PM_REQ_PLAY_END:
	end_list_a();
	return 0;
    }
    return -1;
}
