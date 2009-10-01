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

   filter.c: written by Vincent Pagel ( pagel@loria.fr )

   implements fir antialiasing filter : should help when setting sample
   rates as low as 8Khz.

   April 95
      - first draft

   22/5/95
      - modify "filter" so that it simulate leading and trailing 0 in the buffer
   */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#include <stdio.h>
#ifndef NO_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#include <math.h>
#include <stdlib.h>
#include "timidity.h"
#include "common.h"
#include "controls.h"
#include "instrum.h"
#include "filter.h"

/*  bessel  function   */
static FLOAT_T ino(FLOAT_T x)
{
    FLOAT_T y, de, e, sde;
    int i;

    y = x / 2;
    e = 1.0;
    de = 1.0;
    i = 1;
    do {
	de = de * y / (FLOAT_T) i;
	sde = de * de;
	e += sde;
    } while (!( (e * 1.0e-08 - sde > 0) || (i++ > 25) ));
    return(e);
}

/* Kaiser Window (symetric) */
static void kaiser(FLOAT_T *w,int n,FLOAT_T beta)
{
    FLOAT_T xind, xi;
    int i;

    xind = (2*n - 1) * (2*n - 1);
    for (i =0; i<n ; i++)
	{
	    xi = i + 0.5;
	    w[i] = ino((FLOAT_T)(beta * sqrt((double)(1. - 4 * xi * xi / xind))))
		/ ino((FLOAT_T)beta);
	}
}

/*
 * fir coef in g, cuttoff frequency in fc
 */
static void designfir(FLOAT_T *g , FLOAT_T fc)
{
    int i;
    FLOAT_T xi, omega, att, beta ;
    FLOAT_T w[ORDER2];

    for (i =0; i < ORDER2 ;i++)
	{
	    xi = (FLOAT_T) i + 0.5;
	    omega = M_PI * xi;
	    g[i] = sin( (double) omega * fc) / omega;
	}

    att = 40.; /* attenuation  in  db */
    beta = (FLOAT_T) exp(log((double)0.58417 * (att - 20.96)) * 0.4) + 0.07886
	* (att - 20.96);
    kaiser( w, ORDER2, beta);

    /* Matrix product */
    for (i =0; i < ORDER2 ; i++)
	g[i] = g[i] * w[i];
}

/*
 * FIR filtering -> apply the filter given by coef[] to the data buffer
 * Note that we simulate leading and trailing 0 at the border of the
 * data buffer
 */
static void filter(int16 *result,int16 *data, int32 length,FLOAT_T coef[])
{
    int32 sample,i,sample_window;
    int16 peak = 0;
    FLOAT_T sum;

    /* Simulate leading 0 at the begining of the buffer */
     for (sample = 0; sample < ORDER2 ; sample++ )
	{
	    sum = 0.0;
	    sample_window= sample - ORDER2;

	    for (i = 0; i < ORDER ;i++)
		sum += coef[i] *
		    ((sample_window<0)? 0.0 : data[sample_window++]) ;

	    /* Saturation ??? */
	    if (sum> 32767.) { sum=32767.; peak++; }
	    if (sum< -32768.) { sum=-32768; peak++; }
	    result[sample] = (int16) sum;
	}

    /* The core of the buffer  */
    for (sample = ORDER2; sample < length - ORDER + ORDER2 ; sample++ )
	{
	    sum = 0.0;
	    sample_window= sample - ORDER2;

	    for (i = 0; i < ORDER ;i++)
		sum += data[sample_window++] * coef[i];

	    /* Saturation ??? */
	    if (sum> 32767.) { sum=32767.; peak++; }
	    if (sum< -32768.) { sum=-32768; peak++; }
	    result[sample] = (int16) sum;
	}

    /* Simulate 0 at the end of the buffer */
    for (sample = length - ORDER + ORDER2; sample < length ; sample++ )
	{
	    sum = 0.0;
	    sample_window= sample - ORDER2;

	    for (i = 0; i < ORDER ;i++)
		sum += coef[i] *
		    ((sample_window>=length)? 0.0 : data[sample_window++]) ;

	    /* Saturation ??? */
	    if (sum> 32767.) { sum=32767.; peak++; }
	    if (sum< -32768.) { sum=-32768; peak++; }
	    result[sample] = (int16) sum;
	}

    if (peak)
	ctl->cmsg(CMSG_INFO, VERB_NOISY,
		  "Saturation %2.3f %%.", 100.0*peak/ (FLOAT_T) length);
}

/***********************************************************************/
/* Prevent aliasing by filtering any freq above the output_rate        */
/*                                                                     */
/* I don't worry about looping point -> they will remain soft if they  */
/* were already                                                        */
/***********************************************************************/
void antialiasing(int16 *data, int32 data_length,
		  int32 sample_rate, int32 output_rate)
{
    int16 *temp;
    int i;
    FLOAT_T fir_symetric[ORDER];
    FLOAT_T fir_coef[ORDER2];
    FLOAT_T freq_cut;  /* cutoff frequency [0..1.0] FREQ_CUT/SAMP_FREQ*/


    ctl->cmsg(CMSG_INFO, VERB_NOISY, "Antialiasing: Fsample=%iKHz",
	      sample_rate);

    /* No oversampling  */
    if (output_rate>=sample_rate)
	return;

    freq_cut= (FLOAT_T)output_rate / (FLOAT_T)sample_rate;
    ctl->cmsg(CMSG_INFO, VERB_NOISY, "Antialiasing: cutoff=%f%%",
	      freq_cut*100.);

    designfir(fir_coef,freq_cut);

    /* Make the filter symetric */
    for (i = 0 ; i<ORDER2 ;i++)
	fir_symetric[ORDER-1 - i] = fir_symetric[i] = fir_coef[ORDER2-1 - i];

    /* We apply the filter we have designed on a copy of the patch */
    temp = (int16 *)safe_malloc(2 * data_length);
    memcpy(temp, data, 2 * data_length);

    filter(data, temp, data_length, fir_symetric);

    free(temp);
}
