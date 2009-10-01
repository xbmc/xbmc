/*
** Copyright (C) 1999-2008 Erik de Castro Lopo <erikd@zip.com.au>
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

typedef	void	AUDIO_OUT ;

typedef int (*get_audio_callback_t) (void *callback_data, float *samples, int frames) ;

/* A general audio ooutput function (Linux/OSS, Win32, MacOSX, Solaris)
** which retrieves data using the callback function in the above struct.
**
** audio_open - opens the device and returns an anonymous pointer to its
**              own private data.
*/



AUDIO_OUT *audio_open (int channels, int samplerate) ;

void audio_play (get_audio_callback_t callback, AUDIO_OUT *audio_out, void *callback_data) ;

void audio_close (AUDIO_OUT *audio_data) ;

