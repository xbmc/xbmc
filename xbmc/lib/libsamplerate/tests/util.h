/*
** Copyright (C) 2002-2008 Erik de Castro Lopo <erikd@mega-nerd.com>
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
** Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
*/

#define	ABS(a)			(((a) < 0) ? - (a) : (a))
#define MIN(a,b)		(((a) < (b)) ? (a) : (b))
#define	MAX(a,b)		(((a) >= (b)) ? (a) : (b))

#define	ARRAY_LEN(x)	((int) (sizeof (x) / sizeof ((x) [0])))

void gen_windowed_sines (int freq_count, const double *freqs, double max, float *output, int output_len) ;

void save_oct_float (char *filename, float *input, int in_len, float *output, int out_len) ;
void save_oct_double (char *filename, double *input, int in_len, double *output, int out_len) ;

void interleave_data (const float *in, float *out, int frames, int channels) ;

void deinterleave_data (const float *in, float *out, int frames, int channels) ;

void reverse_data (float *data, int datalen) ;

double calculate_snr (float *data, int len, int expected_peaks) ;

void print_cpu_name (void) ;

#if OS_IS_WIN32
/*
**	Extra Win32 hacks.
**
**	Despite Microsoft claim of windows being POSIX compatibile it has '_sleep'
**	instead of 'sleep'.
*/

#define sleep _sleep
#endif

