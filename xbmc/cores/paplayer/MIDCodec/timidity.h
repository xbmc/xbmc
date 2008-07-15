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

extern int Timidity_Init(int rate, int format, int channels, int samples);
extern char *Timidity_Error(void);
extern void Timidity_SetVolume(int volume);
extern int Timidity_PlaySome(void *stream, int samples);
extern MidiSong *Timidity_LoadSong(char *midifile);
extern void Timidity_Start(MidiSong *song);
extern int Timidity_Active(void);
extern void Timidity_Stop(void);
extern void Timidity_FreeSong(MidiSong *song);

