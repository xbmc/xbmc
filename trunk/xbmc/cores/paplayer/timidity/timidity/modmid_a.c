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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#include "timidity.h"
#include "output.h"

static int open_output(void);
static void close_output(void);
static int output_data(char *buf, int32 bytes);
static int acntl(int request, void *arg);

PlayMode modmidi_play_mode = {
    DEFAULT_RATE,
    PE_16BIT|PE_SIGNED,
    PF_PCM_STREAM,
    -1,
    {0,0,0,0,0},
    "MOD -> MIDI file conversion", 'M',
    NULL,
    open_output,
    close_output,
    output_data,
    acntl
};

static int open_output(void)
{
    modmidi_play_mode.fd = 0;
    return 0;
}

static void close_output(void)
{
    modmidi_play_mode.fd = -1;
}

static int output_data(char *buf, int32 bytes)
{
    return bytes;
}

static int acntl(int request, void *arg)
{
    switch(request)
    {
      case PM_REQ_DISCARD:
	return 0;
    }
    return -1;
}
