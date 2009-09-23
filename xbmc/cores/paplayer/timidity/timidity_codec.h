/*

    TiMidity -- Experimental MIDI to WAVE converter
    Copyright (C) 1995 Tuukka Toivonen <toivonen@clinet.fi>

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
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

typedef struct _MidiSong MidiSong;

int Timidity_Init(int rate, int bits_per_sample, int channels, const char * soundfont_file ); // soundfont_file may be 0
void Timidity_Cleanup();
int Timidity_GetLength( MidiSong *song );
MidiSong *Timidity_LoadSong(char *fn);
void Timidity_FreeSong(MidiSong *song);
int Timidity_FillBuffer( MidiSong* song, void *buf, unsigned int size );
unsigned long Timidity_Seek( MidiSong *song, unsigned long iTimePos );
char *Timidity_ErrorMsg();

