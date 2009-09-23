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

    wave_audio.c

    Functions to output WAVE format data to an external memory buffer.

*/

#include "timidity.h"
#include "common.h"
#include "output.h"
#include "controls.h"
#include "instrum.h"
#include "playmidi.h"
#include "readmidi.h"

static MidiSong * output_song;

static int output_data(char *buf, int32 bytes);
static int acntl(int request, void *arg);

/* export the playback mode */
PlayMode buffer_play_mode =
{
    DEFAULT_RATE,
#ifdef LITTLE_ENDIAN
    PE_16BIT|PE_SIGNED,
#else
    PE_16BIT|PE_SIGNED|PE_BYTESWAP,
#endif
    PF_PCM_STREAM,
    -1,
    {0,0,0,0,0},
    "Buffer output", 'b',
    NULL,
    NULL,
    NULL,
    output_data,
    acntl
};


void outbuf_set_data( MidiSong * song )
{
	output_song = song;
}

static int output_data(char *buf, int32 bytes)
{
    if ( !output_song )
		return -1;

	// How much could we copy? Offset is zero-based, so max offset is song->output_size - 1
	unsigned int freespace = output_song->output_size - output_song->output_offset;
	unsigned int copylength = freespace > bytes ? bytes : freespace;

	memcpy( output_song->output_buffer + output_song->output_offset, buf, copylength );
	output_song->output_offset += copylength;

	// If we didn't copy everything, allocate a special buffer, and store it there
	if ( copylength < bytes )
	{
		unsigned int stored_size = bytes - copylength;

		if ( output_song->stored_buffer )
		{
			void * newbuf = safe_realloc( output_song->stored_buffer, output_song->stored_size + stored_size );

			if ( !newbuf )
				return -1; // just stop adding

			output_song->stored_buffer = newbuf;
			memcpy( output_song->stored_buffer + output_song->stored_size, buf + copylength, stored_size );
			output_song->stored_size = output_song->stored_size + stored_size;
		}
		else
		{
			output_song->stored_buffer = safe_malloc( stored_size );
		
			if ( !output_song->stored_buffer )
				return -1; // there's little we could do

			memcpy( output_song->stored_buffer, buf + copylength, stored_size );
			output_song->stored_size = stored_size;
		}
	}

    return bytes;
}


static int acntl(int request, void *arg)
{
  switch(request)
  {

  case PM_REQ_PLAY_START:
    break;

  case PM_REQ_PLAY_END:
    break;

  case PM_REQ_DISCARD:
    return 0;
  }
  
  return -1;
}

