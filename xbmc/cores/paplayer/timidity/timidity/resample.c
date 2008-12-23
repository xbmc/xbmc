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

    resample.c
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#include <math.h>
#include <stdio.h>
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
#include "output.h"
#include "controls.h"
#include "tables.h"
#include "resample.h"
#include "recache.h"


/* for start/end of samples */
static float newt_coeffs[58][58] = {
#include "newton_table.c"
};

int sample_bounds_min, sample_bounds_max; /* min/max bounds for sample data */

/* 4-point interpolation by cubic spline curve. */

static resample_t resample_cspline(sample_t *src, splen_t ofs, resample_rec_t *rec)
{
    int32 ofsi, ofsf, v0, v1, v2, v3, temp;

    ofsi = ofs >> FRACTION_BITS;
    v1 = src[ofsi];
    v2 = src[ofsi + 1];
    if((ofs<rec->loop_start+(1L<<FRACTION_BITS))||
       ((ofs+(2L<<FRACTION_BITS))>rec->loop_end)){
	return (v1 + ((resample_t)((v2 - v1) * (ofs & FRACTION_MASK)) >> FRACTION_BITS));
    } else {
	v0 = src[ofsi - 1];
	v3 = src[ofsi + 2];
	ofsf = ofs & FRACTION_MASK;
	temp = v2;
	v2 = (6 * v2 + ((((5 * v3 - 11 * v2 + 7 * v1 - v0) >> 2) *
			 (ofsf + (1L << FRACTION_BITS)) >> FRACTION_BITS) *
			(ofsf - (1L << FRACTION_BITS)) >> FRACTION_BITS))
	    * ofsf;
	v1 = (((6 * v1+((((5 * v0 - 11 * v1 + 7 * temp - v3) >> 2) *
			 ofsf >> FRACTION_BITS) * (ofsf - (2L << FRACTION_BITS))
			>> FRACTION_BITS)) * ((1L << FRACTION_BITS) - ofsf)) + v2)
	    / (6L << FRACTION_BITS);
	return ((v1 > sample_bounds_max) ? sample_bounds_max :
		((v1 < sample_bounds_min) ? sample_bounds_min : v1));
    }
}


/* 4-point interpolation by Lagrange method.
   Lagrange is now faster than C-spline.  Both have about the same accuracy,
   so choose Lagrange over C-spline, since it is faster.  Technically, it is
   really a 3rd order Newton polynomial (whereas the old Lagrange truely was
   the Lagrange form of the polynomial).  Both Newton and Lagrange forms
   yield the same numerical results, but the Newton form is faster.  Since
   n'th order Newton interpolaiton is resample_newton(), it made sense to
   just keep this labeled as resample_lagrange(), even if it really is the
   Newton form of the polynomial. */

static resample_t resample_lagrange(sample_t *src, splen_t ofs, resample_rec_t *rec)
{
    int32 ofsi, ofsf, v0, v1, v2, v3;

    ofsi = ofs >> FRACTION_BITS;
    v1 = (int32)src[ofsi];
    v2 = (int32)src[ofsi + 1];
    if((ofs<rec->loop_start+(1L<<FRACTION_BITS))||
       ((ofs+(2L<<FRACTION_BITS))>rec->loop_end)) {
	return (v1 + ((resample_t)((v2 - v1) * (ofs & FRACTION_MASK)) >> FRACTION_BITS));
    } else {
	v0 = (int32)src[ofsi - 1];
	v3 = (int32)src[ofsi + 2];
	ofsf = (ofs & FRACTION_MASK) + (1<<FRACTION_BITS);
	v3 += -3*v2 + 3*v1 - v0;
	v3 *= (ofsf - (2<<FRACTION_BITS)) / 6;
	v3 >>= FRACTION_BITS;
	v3 += v2 - v1 - v1 + v0;
	v3 *= (ofsf - (1<<FRACTION_BITS)) >> 1;
	v3 >>= FRACTION_BITS;
	v3 += v1 - v0;
	v3 *= ofsf;
	v3 >>= FRACTION_BITS;
	v3 += v0;
	return ((v3 > sample_bounds_max) ? sample_bounds_max :
		((v3 < sample_bounds_min) ? sample_bounds_min : v3));
    }
}


/* Very fast and accurate table based interpolation.  Better speed and higher
   accuracy than Newton.  This isn't *quite* true Gauss interpolation; it's
   more a slightly modified Gauss interpolation that I accidently stumbled
   upon.  Rather than normalize all x values in the window to be in the range
   [0 to 2*PI], it simply divides them all by 2*PI instead.  I don't know why
   this works, but it does.  Gauss should only work on periodic data with the
   window spanning exactly one period, so it is no surprise that regular Gauss
   interpolation doesn't work too well on general audio data.  But dividing
   the x values by 2*PI magically does.  Any other scaling produces degraded
   results or total garbage.  If anyone can work out the theory behind why
   this works so well (at first glance, it shouldn't ??), please contact me
   (Eric A. Welsh, ewelsh@ccb.wustl.edu), as I would really like to have some
   mathematical justification for doing this.  Despite the lack of any sound
   theoretical basis, this method DOES result in highly accurate interpolation
   (or possibly approximaton, not sure yet if it truly interpolates, but it
   looks like it does).  -N 34 is as high as it can go before errors start
   appearing.  But even at -N 34, it is more accurate than Newton at -N 57.
   -N 34 has no problem running in realtime on my system, but -N 25 is the
   default, since that is the optimal compromise between speed and accuracy.
   I strongly recommend using Gauss interpolation.  It is the highest
   quality interpolation option available, and is much faster than using
   Newton polynomials. */

#define DEFAULT_GAUSS_ORDER	25
static float *gauss_table[(1<<FRACTION_BITS)] = {0};	/* don't need doubles */
static int gauss_n = DEFAULT_GAUSS_ORDER;

static resample_t resample_gauss(sample_t *src, splen_t ofs, resample_rec_t *rec)
{
    sample_t *sptr;
    int32 left, right, temp_n;

    left = (ofs>>FRACTION_BITS);
    right = (rec->data_length>>FRACTION_BITS) - left - 1;
    temp_n = (right<<1)-1;
    if (temp_n > (left<<1)+1)
	temp_n = (left<<1)+1;
    if (temp_n < gauss_n) {
	int ii, jj;
	float xd, y;
	if (temp_n <= 0)
	    temp_n = 1;
	xd = ofs & FRACTION_MASK;
	xd /= (1L<<FRACTION_BITS);
	xd += temp_n>>1;
	y = 0;
	sptr = src + (ofs>>FRACTION_BITS) - (temp_n>>1);
	for (ii = temp_n; ii;) {
	    for (jj = 0; jj <= ii; jj++)
		y += sptr[jj] * newt_coeffs[ii][jj];
	    y *= xd - --ii;
	}
	y += *sptr;
	return ((y > sample_bounds_max) ? sample_bounds_max :
		((y < sample_bounds_min) ? sample_bounds_min : y));
    } else {
	float *gptr, *gend;
	float y;
	y = 0;
	sptr = src + left - (gauss_n>>1);
	gptr = gauss_table[ofs&FRACTION_MASK];
	if (gauss_n == DEFAULT_GAUSS_ORDER) {
	    /* expanding the loop for the default case.
	     * this will allow intensive optimization when compiled
	     * with SSE2 capability.
	     */
#define do_gauss  y += *(sptr++) * *(gptr++);
	    do_gauss;
	    do_gauss;
	    do_gauss;
	    do_gauss;
	    do_gauss;
	    do_gauss;
	    do_gauss;
	    do_gauss;
	    do_gauss;
	    do_gauss;
	    do_gauss;
	    do_gauss;
	    do_gauss;
	    do_gauss;
	    do_gauss;
	    do_gauss;
	    do_gauss;
	    do_gauss;
	    do_gauss;
	    do_gauss;
	    do_gauss;
	    do_gauss;
	    do_gauss;
	    do_gauss;
	    do_gauss;
	    y += *sptr * *gptr;
#undef do_gauss
	} else {
	    gend = gptr + gauss_n;
	    do {
		y += *(sptr++) * *(gptr++);
	    } while (gptr <= gend);
	}
	return ((y > sample_bounds_max) ? sample_bounds_max :
		((y < sample_bounds_min) ? sample_bounds_min : y));
    }
}


/* (at least) n+1 point interpolation using Newton polynomials.
   n can be set with a command line option, and
   must be an odd number from 1 to 57 (57 is as high as double precision
   can go without precision errors).  Default n = 11 is good for a 1.533 MHz
   Athlon.  Larger values for n require very fast processors for real time
   playback.  Some points will be interpolated at orders > n to both increase
   accuracy and save CPU. */

static int newt_n = 11;
static int32 newt_old_trunc_x = -1;
static int newt_grow = -1;
static int newt_max = 13;
static double newt_divd[60][60];
static double newt_recip[60] = { 0, 1, 1.0/2, 1.0/3, 1.0/4, 1.0/5, 1.0/6, 1.0/7,
			1.0/8, 1.0/9, 1.0/10, 1.0/11, 1.0/12, 1.0/13, 1.0/14,
			1.0/15, 1.0/16, 1.0/17, 1.0/18, 1.0/19, 1.0/20, 1.0/21,
			1.0/22, 1.0/23, 1.0/24, 1.0/25, 1.0/26, 1.0/27, 1.0/28,
			1.0/29, 1.0/30, 1.0/31, 1.0/32, 1.0/33, 1.0/34, 1.0/35,
			1.0/36, 1.0/37, 1.0/38, 1.0/39, 1.0/40, 1.0/41, 1.0/42,
			1.0/43, 1.0/44, 1.0/45, 1.0/46, 1.0/47, 1.0/48, 1.0/49,
			1.0/50, 1.0/51, 1.0/52, 1.0/53, 1.0/54, 1.0/55, 1.0/56,
			1.0/57, 1.0/58, 1.0/59 };
static sample_t *newt_old_src = NULL;

static resample_t resample_newton(sample_t *src, splen_t ofs, resample_rec_t *rec)
{
    int n_new, n_old;
    int32 v1, v2, diff;
    sample_t *sptr;
    double y, xd;
    int32 left, right, temp_n;
    int ii, jj;

    left = (ofs>>FRACTION_BITS);
    right = (rec->data_length>>FRACTION_BITS)-(ofs>>FRACTION_BITS)-1;
    temp_n = (right<<1)-1;
    if (temp_n <= 0)
	temp_n = 1;
    if (temp_n > (left<<1)+1)
	temp_n = (left<<1)+1;
    if (temp_n < newt_n) {
	xd = ofs & FRACTION_MASK;
	xd /= (1L<<FRACTION_BITS);
	xd += temp_n>>1;
	y = 0;
	sptr = src + (ofs>>FRACTION_BITS) - (temp_n>>1);
	for (ii = temp_n; ii;) {
	    for (jj = 0; jj <= ii; jj++)
		y += sptr[jj] * newt_coeffs[ii][jj];
	    y *= xd - --ii;
	} y += *sptr;
    }else{
	if (newt_grow >= 0 && src == newt_old_src &&
	    (diff = (ofs>>FRACTION_BITS) - newt_old_trunc_x) > 0){
	    n_new = newt_n + ((newt_grow + diff)<<1);
	    if (n_new <= newt_max){
		n_old = newt_n + (newt_grow<<1);
		newt_grow += diff;
		for (v1=(ofs>>FRACTION_BITS)+(n_new>>1)+1,v2=n_new;
		     v2 > n_old; --v1, --v2){
		    newt_divd[0][v2] = src[v1];
		}for (v1 = 1; v1 <= n_new; v1++)
		    for (v2 = n_new; v2 > n_old; --v2)
			newt_divd[v1][v2] = (newt_divd[v1-1][v2] -
					     newt_divd[v1-1][v2-1]) *
			    newt_recip[v1];
	    }else newt_grow = -1;
	}
	if (newt_grow < 0 || src != newt_old_src || diff < 0){
	    newt_grow = 0;
	    for (v1=(ofs>>FRACTION_BITS)-(newt_n>>1),v2=0;
		 v2 <= newt_n; v1++, v2++){
		newt_divd[0][v2] = src[v1];
	    }for (v1 = 1; v1 <= newt_n; v1++)
		for (v2 = newt_n; v2 >= v1; --v2)
		    newt_divd[v1][v2] = (newt_divd[v1-1][v2] -
					 newt_divd[v1-1][v2-1]) *
			newt_recip[v1];
	}
	n_new = newt_n + (newt_grow<<1);
	v2 = n_new;
	y = newt_divd[v2][v2];
	xd = (double)(ofs&FRACTION_MASK) / (1L<<FRACTION_BITS) +
	    (newt_n>>1) + newt_grow;
	for (--v2; v2; --v2){
	    y *= xd - v2;
	    y += newt_divd[v2][v2];
	}y = y*xd + **newt_divd;
	newt_old_src = src;
	newt_old_trunc_x = (ofs>>FRACTION_BITS);
    }
    return ((y > sample_bounds_max) ? sample_bounds_max :
    	    ((y < sample_bounds_min) ? sample_bounds_min : y));
}


/* Simple linear interpolation */

static resample_t resample_linear(sample_t *src, splen_t ofs, resample_rec_t *rec)
{
    int32 v1, v2, ofsi;

    ofsi = ofs >> FRACTION_BITS;
    v1 = src[ofsi];
    v2 = src[ofsi + 1];
#if defined(LOOKUP_HACK) && defined(LOOKUP_INTERPOLATION)
    return (sample_t)(v1 + (iplookup[(((v2 - v1) << 5) & 0x03FE0) |
				     ((ofs & FRACTION_MASK) >> (FRACTION_BITS-5))]));
#else
    return (v1 + ((resample_t)((v2 - v1) * (ofs & FRACTION_MASK)) >> FRACTION_BITS));
#endif
}


/* No interpolation -- Earplugs recommended for maximum listening enjoyment */

static resample_t resample_none(sample_t *src, splen_t ofs, resample_rec_t *rec)
{
    return src[ofs >> FRACTION_BITS];
}


/*
 */

typedef resample_t (*resampler_t)(sample_t*, splen_t, resample_rec_t *);
static resampler_t resamplers[] = {
    resample_cspline,
    resample_lagrange,
    resample_gauss,
    resample_newton,
    resample_linear,
    resample_none
};

#ifdef FIXED_RESAMPLATION
/* don't allow to change the resamplation algorighm.
 * accessing directly to the given function.
 * hope the compiler will optimize the overhead of function calls in this case.
 */
#define cur_resample DEFAULT_RESAMPLATION
#else
static resampler_t cur_resample = DEFAULT_RESAMPLATION;
#endif

#define RESAMPLATION *dest++ = cur_resample(src, ofs, &resrc);

/* exported for recache.c */
resample_t do_resamplation(sample_t *src, splen_t ofs, resample_rec_t *rec)
{
    return cur_resample(src, ofs, rec);
}

/* return the current resampling algorithm */
int get_current_resampler(void)
{
    int i;
    for (i = 0; i < (int)(sizeof(resamplers)/sizeof(resamplers[0])); i++)
	if (resamplers[i] == cur_resample)
	    return i;
    return 0;
}

/* set the current resampling algorithm */
int set_current_resampler(int type)
{
#ifdef FIXED_RESAMPLATION
    return -1;
#else
    if (type < 0 || type > RESAMPLE_NONE)
	return -1;
    cur_resample = resamplers[type];
    return 0;
#endif
}

/* #define FINALINTERP if (ofs < le) *dest++=src[(ofs>>FRACTION_BITS)-1]/2; */
#define FINALINTERP /* Nothing to do after TiMidity++ 2.9.0 */
/* So it isn't interpolation. At least it's final. */

static resample_t resample_buffer[AUDIO_BUFFER_SIZE];
static int resample_buffer_offset;
static resample_t *vib_resample_voice(int, int32 *, int);
static resample_t *normal_resample_voice(int, int32 *, int);

#ifdef PRECALC_LOOPS
#if SAMPLE_LENGTH_BITS == 32 && TIMIDITY_HAVE_INT64
#define PRECALC_LOOP_COUNT(start, end, incr) (int32)(((int64)((end) - (start) + (incr) - 1)) / (incr))
#else
#define PRECALC_LOOP_COUNT(start, end, incr) (int32)(((splen_t)((end) - (start) + (incr) - 1)) / (incr))
#endif
#endif /* PRECALC_LOOPS */

void initialize_gauss_table(int n)
{
    int m, i, k, n_half = (n>>1);
    double ck;
    double x, x_inc, xz;
    double z[35], zsin_[34 + 35], *zsin, xzsin[35];
    float *gptr;

    for (i = 0; i <= n; i++)
    	z[i] = i / (4*M_PI);
    zsin = &zsin_[34];
    for (i = -n; i <= n; i++)
    	zsin[i] = sin(i / (4*M_PI));

    x_inc = 1.0 / (1<<FRACTION_BITS);
    gptr = safe_realloc(gauss_table[0], (n+1)*sizeof(float)*(1<<FRACTION_BITS));
    for (m = 0, x = 0.0; m < (1<<FRACTION_BITS); m++, x += x_inc)
    {
    	xz = (x + n_half) / (4*M_PI);
    	for (i = 0; i <= n; i++)
	    xzsin[i] = sin(xz - z[i]);
    	gauss_table[m] = gptr;

    	for (k = 0; k <= n; k++)
    	{
    	    ck = 1.0;

    	    for (i = 0; i <= n; i++)
    	    {
    	    	if (i == k)
    	    	    continue;
    	
    	    	ck *= xzsin[i] / zsin[k - i];
    	    }
    	    
    	    *gptr++ = ck;
    	}
    }
}

#if 0 /* NOT USED */
/* the was calculated statically in newton_table.c */
static void initialize_newton_coeffs(void)
{
    int i, j, n = 57;
    int sign;

    newt_coeffs[0][0] = 1;
    for (i = 0; i <= n; i++)
    {
    	newt_coeffs[i][0] = 1;
    	newt_coeffs[i][i] = 1;

	if (i > 1)
	{
	    newt_coeffs[i][0] = newt_coeffs[i-1][0] / i;
	    newt_coeffs[i][i] = newt_coeffs[i-1][0] / i;
	}

    	for (j = 1; j < i; j++)
    	{
    	    newt_coeffs[i][j] = newt_coeffs[i-1][j-1] + newt_coeffs[i-1][j];

	    if (i > 1)
	    	newt_coeffs[i][j] /= i;
	}
    }
    for (i = 0; i <= n; i++)
    	for (j = 0, sign = pow(-1, i); j <= i; j++, sign *= -1)
    	    newt_coeffs[i][j] *= sign;
}
#endif /* NOT USED */

/* initialize the coefficients of the current resampling algorithm */
void initialize_resampler_coeffs(void)
{
    /* initialize_newton_coeffs(); */
    initialize_gauss_table(gauss_n);
    /* we don't have to initialize newton table any more */

    /* bounds checking values for the appropriate sample types */
    /* this is as good a place as any to initialize them */
    if (play_mode->encoding & PE_24BIT)
    {
    	sample_bounds_min = -8388608;
    	sample_bounds_max = 8388607;
    }
    else /* 16-bit */
    {
    	sample_bounds_min = -32768;
    	sample_bounds_max = 32767;
    }
}

/* change the parameter for the current resampling algorithm */
int set_resampler_parm(int val)
{
    if (cur_resample == resample_gauss) {
	if (val < 1 || val > 34)
	    return -1;
	else
	    gauss_n = val;
    } else if (cur_resample == resample_newton) {
	if (val < 1 || val > 57)
	    return -1;
	else if (val % 2 == 0)
	    return -1;
	else {
	    newt_n = val;
	    /* set optimal value for newt_max */
	    newt_max = newt_n * 1.57730263158 - 1.875328947;
	    if (newt_max < newt_n)
		    newt_max = newt_n;
	    if (newt_max > 57)
		    newt_max = 57;
	}
    }
    return 0;
}

/*************** resampling with fixed increment *****************/

static resample_t *rs_plain_c(int v, int32 *countptr)
{
    Voice *vp = &voice[v];
    resample_t *dest = resample_buffer + resample_buffer_offset;
	sample_t *src = vp->sample->data;
    int32 ofs, count = *countptr, i, le;

    le = (int32)(vp->sample->loop_end >> FRACTION_BITS);
    ofs = (int32)(vp->sample_offset >> FRACTION_BITS);

    i = ofs + count;
    if(i > le)
	i = le;
    count = i - ofs;

	for (i = 0; i < count; i++) {
		dest[i] = src[i + ofs];
	}

    ofs += count;
    if(ofs == le)
    {
	vp->timeout = 1;
	*countptr = count;
    }
    vp->sample_offset = ((splen_t)ofs << FRACTION_BITS);
    return resample_buffer + resample_buffer_offset;
}

static resample_t *rs_plain(int v, int32 *countptr)
{
  /* Play sample until end, then free the voice. */
  Voice *vp = &voice[v];
  resample_t *dest = resample_buffer + resample_buffer_offset;
  sample_t *src = vp->sample->data;
  splen_t
    ofs = vp->sample_offset,
    ls = 0,
    le = vp->sample->data_length;
  resample_rec_t resrc;
  int32 count = *countptr, incr = vp->sample_increment;
#ifdef PRECALC_LOOPS
  int32 i, j;
#endif

  if(vp->cache && incr == (1 << FRACTION_BITS))
      return rs_plain_c(v, countptr);

  resrc.loop_start = ls;
  resrc.loop_end = le;
  resrc.data_length = vp->sample->data_length;
#ifdef PRECALC_LOOPS
  if (incr < 0) incr = -incr; /* In case we're coming out of a bidir loop */

  /* Precalc how many times we should go through the loop.
     NOTE: Assumes that incr > 0 and that ofs <= le */
  i = PRECALC_LOOP_COUNT(ofs, le, incr);

  if (i > count)
    {
      i = count;
      count = 0;
    }
  else count -= i;

  for(j = 0; j < i; j++)
    {
      RESAMPLATION;
      ofs += incr;
    }

  if (ofs >= le)
    {
      FINALINTERP;
      vp->timeout = 1;
      *countptr -= count;
    }
#else /* PRECALC_LOOPS */
    while (count--)
    {
      RESAMPLATION;
      ofs += incr;
      if (ofs >= le)
	{
	  FINALINTERP;
	  vp->timeout = 1;
	  *countptr -= count;
	  break;
	}
    }
#endif /* PRECALC_LOOPS */

  vp->sample_offset = ofs; /* Update offset */
  return resample_buffer + resample_buffer_offset;
}

static resample_t *rs_loop_c(Voice *vp, int32 count)
{
  int32
    ofs = (int32)(vp->sample_offset >> FRACTION_BITS),
    le = (int32)(vp->sample->loop_end >> FRACTION_BITS),
    ll = le - (int32)(vp->sample->loop_start >> FRACTION_BITS);
  resample_t *dest = resample_buffer + resample_buffer_offset;
  sample_t *src = vp->sample->data;
  int32 i, j;

  while(count)
  {
      while(ofs >= le)
	  ofs -= ll;
      /* Precalc how many times we should go through the loop */
      i = le - ofs;
      if(i > count)
	  i = count;
      count -= i;
	  for (j = 0; j < i; j++) {
		  dest[j] = src[j + ofs];
	  }
      dest += i;
      ofs += i;
  }
  vp->sample_offset = ((splen_t)ofs << FRACTION_BITS);
  return resample_buffer + resample_buffer_offset;
}

static resample_t *rs_loop(Voice *vp, int32 count)
{
  /* Play sample until end-of-loop, skip back and continue. */
  splen_t
    ofs = vp->sample_offset,
    ls, le, ll;
  resample_rec_t resrc;
  resample_t *dest = resample_buffer + resample_buffer_offset;
  sample_t *src = vp->sample->data;
#ifdef PRECALC_LOOPS
  int32 i, j;
#endif
  int32 incr = vp->sample_increment;

  if(vp->cache && incr == (1 << FRACTION_BITS))
      return rs_loop_c(vp, count);

  resrc.loop_start = ls = vp->sample->loop_start;
  resrc.loop_end = le = vp->sample->loop_end;
  ll = le - ls;
  resrc.data_length = vp->sample->data_length;

#ifdef PRECALC_LOOPS
  while (count)
    {
      while (ofs >= le)	{ofs -= ll;}
      /* Precalc how many times we should go through the loop */
      i = PRECALC_LOOP_COUNT(ofs, le, incr);
      if (i > count) {
		  i = count;
		  count = 0;
	  } else {count -= i;}
      for(j = 0; j < i; j++) {
		  RESAMPLATION;
		  ofs += incr;
	  }
    }
#else
  while (count--)
    {
      RESAMPLATION;
      ofs += incr;
      if (ofs >= le)
	ofs -= ll; /* Hopefully the loop is longer than an increment. */
    }
#endif

  vp->sample_offset = ofs; /* Update offset */
  return resample_buffer + resample_buffer_offset;
}

static resample_t *rs_bidir(Voice *vp, int32 count)
{
#if SAMPLE_LENGTH_BITS == 32
  int32
#else
  splen_t
#endif
    ofs = vp->sample_offset,
    le = vp->sample->loop_end,
    ls = vp->sample->loop_start;
  resample_t *dest = resample_buffer + resample_buffer_offset;
  sample_t *src = vp->sample->data;
  int32 incr = vp->sample_increment;
  resample_rec_t resrc;

#ifdef PRECALC_LOOPS
#if SAMPLE_LENGTH_BITS == 32
  int32
#else
  splen_t
#endif
    le2 = le << 1,
    ls2 = ls << 1;
  int32 i, j;
  /* Play normally until inside the loop region */

  resrc.loop_start = ls;
  resrc.loop_end = le;
  resrc.data_length = vp->sample->data_length;

  if (incr > 0 && ofs < ls)
    {
      /* NOTE: Assumes that incr > 0, which is NOT always the case
	 when doing bidirectional looping.  I have yet to see a case
	 where both ofs <= ls AND incr < 0, however. */
      i = PRECALC_LOOP_COUNT(ofs, ls, incr);
      if (i > count)
	{
	  i = count;
	  count = 0;
	}
      else count -= i;
      for(j = 0; j < i; j++)
	{
	  RESAMPLATION;
	  ofs += incr;
	}
    }

  /* Then do the bidirectional looping */

  while(count)
    {
      /* Precalc how many times we should go through the loop */
      i = PRECALC_LOOP_COUNT(ofs, incr > 0 ? le : ls, incr);
      if (i > count)
	{
	  i = count;
	  count = 0;
	}
      else count -= i;
      for(j = 0; j < i; j++)
	{
	  RESAMPLATION;
	  ofs += incr;
	}
      if(ofs >= 0 && ofs >= le)
	{
	  /* fold the overshoot back in */
	  ofs = le2 - ofs;
	  incr *= -1;
	}
      else if (ofs <= 0 || ofs <= ls)
	{
	  ofs = ls2 - ofs;
	  incr *= -1;
	}
    }

#else /* PRECALC_LOOPS */
  /* Play normally until inside the loop region */

  if (ofs < ls)
    {
      while (count--)
	{
	  RESAMPLATION;
	  ofs += incr;
	  if (ofs >= ls)
	    break;
	}
    }

  /* Then do the bidirectional looping */

  if (count > 0)
    while (count--)
      {
	RESAMPLATION;
	ofs += incr;
	if (ofs >= le)
	  {
	    /* fold the overshoot back in */
	    ofs = le - (ofs - le);
	    incr = -incr;
	  }
	else if (ofs <= ls)
	  {
	    ofs = ls + (ls - ofs);
	    incr = -incr;
	  }
      }
#endif /* PRECALC_LOOPS */
  vp->sample_increment = incr;
  vp->sample_offset = ofs; /* Update offset */
  return resample_buffer + resample_buffer_offset;
}

/*********************** vibrato versions ***************************/

/* We only need to compute one half of the vibrato sine cycle */
static int vib_phase_to_inc_ptr(int phase)
{
  if (phase < VIBRATO_SAMPLE_INCREMENTS / 2)
    return VIBRATO_SAMPLE_INCREMENTS / 2 - 1 - phase;
  else if (phase >= 3 * VIBRATO_SAMPLE_INCREMENTS / 2)
    return 5 * VIBRATO_SAMPLE_INCREMENTS / 2 - 1 - phase;
  else
    return phase - VIBRATO_SAMPLE_INCREMENTS / 2;
}

static int32 update_vibrato(Voice *vp, int sign)
{
  int32 depth;
  int phase, pb;
  double a;
  int ch = vp->channel;

  if(vp->vibrato_delay > 0)
  {
      vp->vibrato_delay -= vp->vibrato_control_ratio;
      if(vp->vibrato_delay > 0)
	  return vp->sample_increment;
  }

  if (vp->vibrato_phase++ >= 2 * VIBRATO_SAMPLE_INCREMENTS - 1)
    vp->vibrato_phase = 0;
  phase = vib_phase_to_inc_ptr(vp->vibrato_phase);

  if (vp->vibrato_sample_increment[phase])
    {
      if (sign)
	return -vp->vibrato_sample_increment[phase];
      else
	return vp->vibrato_sample_increment[phase];
    }

  /* Need to compute this sample increment. */

  depth = vp->vibrato_depth;
  depth <<= 7;

  if (vp->vibrato_sweep && !channel[ch].mod.val)
    {
      /* Need to update sweep */
      vp->vibrato_sweep_position += vp->vibrato_sweep;
      if (vp->vibrato_sweep_position >= (1 << SWEEP_SHIFT))
	vp->vibrato_sweep=0;
      else
	{
	  /* Adjust depth */
	  depth *= vp->vibrato_sweep_position;
	  depth >>= SWEEP_SHIFT;
	}
    }

  if(vp->sample->inst_type == INST_SF2) {
  pb = (int)((lookup_triangular(vp->vibrato_phase *
			(SINE_CYCLE_LENGTH / (2 * VIBRATO_SAMPLE_INCREMENTS)))
	    * (double)(depth) * VIBRATO_AMPLITUDE_TUNING));
  } else {
  pb = (int)((lookup_sine(vp->vibrato_phase *
			(SINE_CYCLE_LENGTH / (2 * VIBRATO_SAMPLE_INCREMENTS)))
	    * (double)(depth) * VIBRATO_AMPLITUDE_TUNING));
  }

  a = TIM_FSCALE(((double)(vp->sample->sample_rate) *
		  (double)(vp->frequency)) /
		 ((double)(vp->sample->root_freq) *
		  (double)(play_mode->rate)),
		 FRACTION_BITS);

  if(pb < 0) {
      pb = -pb;
      a /= bend_fine[(pb >> 5) & 0xFF] * bend_coarse[pb >> 13];
	  pb = -pb;
  } else {
      a *= bend_fine[(pb >> 5) & 0xFF] * bend_coarse[pb >> 13];
  }
  a += 0.5;

  /* If the sweep's over, we can store the newly computed sample_increment */
  if (!vp->vibrato_sweep || channel[ch].mod.val)
    vp->vibrato_sample_increment[phase] = (int32) a;

  if (sign)
    a = -a; /* need to preserve the loop direction */

  return (int32) a;
}

static resample_t *rs_vib_plain(int v, int32 *countptr)
{
  /* Play sample until end, then free the voice. */
  Voice *vp = &voice[v];
  resample_t *dest = resample_buffer + resample_buffer_offset;
  sample_t *src = vp->sample->data;
  splen_t
    ls = 0,
    le = vp->sample->data_length,
    ofs = vp->sample_offset;
  resample_rec_t resrc;
    
  int32 count = *countptr, incr = vp->sample_increment;
  int cc = vp->vibrato_control_counter;

  resrc.loop_start = ls;
  resrc.loop_end = le;
  resrc.data_length = vp->sample->data_length;
  /* This has never been tested */

  if (incr < 0) incr = -incr; /* In case we're coming out of a bidir loop */

  while (count--)
    {
      if (!cc--)
	{
	  cc = vp->vibrato_control_ratio;
	  incr = update_vibrato(vp, 0);
	}
      RESAMPLATION;
      ofs += incr;
      if (ofs >= le)
	{
	  FINALINTERP;
	  vp->timeout = 1;
	  *countptr -= count;
	  break;
	}
    }

  vp->vibrato_control_counter = cc;
  vp->sample_increment = incr;
  vp->sample_offset = ofs; /* Update offset */
  return resample_buffer + resample_buffer_offset;
}

static resample_t *rs_vib_loop(Voice *vp, int32 count)
{
  /* Play sample until end-of-loop, skip back and continue. */
  splen_t
    ofs = vp->sample_offset,
    ls = vp->sample->loop_start,
    le = vp->sample->loop_end,
    ll = le - vp->sample->loop_start;
  resample_t *dest = resample_buffer + resample_buffer_offset;
  sample_t *src = vp->sample->data;
  int cc = vp->vibrato_control_counter;
  int32 incr = vp->sample_increment;
  resample_rec_t resrc;
#ifdef PRECALC_LOOPS
  int32 i, j;
  int vibflag=0;
#endif

  resrc.loop_start = ls;
  resrc.loop_end = le;
  resrc.data_length =vp->sample->data_length;

#ifdef PRECALC_LOOPS
  while (count)
    {
      /* Hopefully the loop is longer than an increment */
      while(ofs >= le) {ofs -= ll;}
      /* Precalc how many times to go through the loop, taking
	 the vibrato control ratio into account this time. */
      i = PRECALC_LOOP_COUNT(ofs, le, incr);
      if(i > count) {
		  i = count;
	  }
      if(i > cc) {
		  i = cc;
		  vibflag = 1;
	  } else {cc -= i;}
      count -= i;
      if(vibflag) {
		  cc = vp->vibrato_control_ratio;
		  incr = update_vibrato(vp, 0);
		  vibflag = 0;
	  }
      for(j = 0; j < i; j++) {
		  RESAMPLATION;
		  ofs += incr;
	  }
    }
#else /* PRECALC_LOOPS */
  while (count--)
    {
      if (!cc--)
	{
	  cc=vp->vibrato_control_ratio;
	  incr=update_vibrato(vp, 0);
	}
      RESAMPLATION;
      ofs += incr;
      if (ofs >= le)
	ofs -= ll; /* Hopefully the loop is longer than an increment. */
    }
#endif /* PRECALC_LOOPS */

  vp->vibrato_control_counter = cc;
  vp->sample_increment = incr;
  vp->sample_offset = ofs; /* Update offset */
  return resample_buffer + resample_buffer_offset;
}

static resample_t *rs_vib_bidir(Voice *vp, int32 count)
{
#if SAMPLE_LENGTH_BITS == 32
  int32
#else
  splen_t
#endif
    ofs = vp->sample_offset,
    le = vp->sample->loop_end,
    ls = vp->sample->loop_start;
  resample_t *dest = resample_buffer + resample_buffer_offset;
  sample_t *src = vp->sample->data;
  int cc=vp->vibrato_control_counter;
  int32 incr = vp->sample_increment;
  resample_rec_t resrc;

#ifdef PRECALC_LOOPS
#if SAMPLE_LENGTH_BITS == 32
  int32
#else
  splen_t
#endif
    le2 = le << 1,
    ls2 = ls << 1;
  int32 i, j;
  int vibflag = 0;

  resrc.loop_start = ls;
  resrc.loop_end = le;
  resrc.data_length = vp->sample->data_length;
  /* Play normally until inside the loop region */
  while (count && incr > 0 && ofs < ls)
    {
      i = PRECALC_LOOP_COUNT(ofs, ls, incr);
      if (i > count) i = count;
      if (i > cc)
	{
	  i = cc;
	  vibflag = 1;
	}
      else cc -= i;
      count -= i;
      if (vibflag)
	{
	  cc = vp->vibrato_control_ratio;
	  incr = update_vibrato(vp, 0);
	  vibflag = 0;
	}
      for(j = 0; j < i; j++)
	{
	  RESAMPLATION;
	  ofs += incr;
	}
    }

  /* Then do the bidirectional looping */

  while (count)
    {
      /* Precalc how many times we should go through the loop */
      i = PRECALC_LOOP_COUNT(ofs, incr > 0 ? le : ls, incr);
      if(i > count) i = count;
      if(i > cc)
	{
	  i = cc;
	  vibflag = 1;
	}
      else cc -= i;
      count -= i;
      if (vibflag)
	{
	  cc = vp->vibrato_control_ratio;
	  incr = update_vibrato(vp, (incr < 0));
	  vibflag = 0;
	}
      while (i--)
	{
	  RESAMPLATION;
	  ofs += incr;
	}
      if (ofs >= 0 && ofs >= le)
	{
	  /* fold the overshoot back in */
	  ofs = le2 - ofs;
	  incr *= -1;
	}
      else if (ofs <= 0 || ofs <= ls)
	{
	  ofs = ls2 - ofs;
	  incr *= -1;
	}
    }

#else /* PRECALC_LOOPS */

  resrc.loop_start = ls;
  resrc.loop_end = le;
  resrc.data_length = vp->sample->data_length;
  /* Play normally until inside the loop region */

  if (ofs < ls)
    {
      while (count--)
	{
	  if (!cc--)
	    {
	      cc = vp->vibrato_control_ratio;
	      incr = update_vibrato(vp, 0);
	    }
	  RESAMPLATION;
	  ofs += incr;
	  if (ofs >= ls)
	    break;
	}
    }

  /* Then do the bidirectional looping */

  if (count > 0)
    while (count--)
      {
	if (!cc--)
	  {
	    cc=vp->vibrato_control_ratio;
	    incr=update_vibrato(vp, (incr < 0));
	  }
	RESAMPLATION;
	ofs += incr;
	if (ofs >= le)
	  {
	    /* fold the overshoot back in */
	    ofs = le - (ofs - le);
	    incr = -incr;
	  }
	else if (ofs <= ls)
	  {
	    ofs = ls + (ls - ofs);
	    incr = -incr;
	  }
      }
#endif /* PRECALC_LOOPS */

  /* Update changed values */
  vp->vibrato_control_counter = cc;
  vp->sample_increment = incr;
  vp->sample_offset = ofs;
  return resample_buffer + resample_buffer_offset;
}

/*********************** portamento versions ***************************/

static int rs_update_porta(int v)
{
    Voice *vp = &voice[v];
    int32 d;

    d = vp->porta_dpb;
    if(vp->porta_pb < 0)
    {
	if(d > -vp->porta_pb)
	    d = -vp->porta_pb;
    }
    else
    {
	if(d > vp->porta_pb)
	    d = -vp->porta_pb;
	else
	    d = -d;
    }

    vp->porta_pb += d;
    if(vp->porta_pb == 0)
    {
	vp->porta_control_ratio = 0;
	vp->porta_pb = 0;
    }
    recompute_freq(v);
    return vp->porta_control_ratio;
}

static resample_t *porta_resample_voice(int v, int32 *countptr, int mode)
{
    Voice *vp = &voice[v];
    int32 n = *countptr, i;
    resample_t *(* resampler)(int, int32 *, int);
    int cc = vp->porta_control_counter;
    int loop;

    if(vp->vibrato_control_ratio)
	resampler = vib_resample_voice;
    else
	resampler = normal_resample_voice;
    if(mode != 1)
	loop = 1;
    else
	loop = 0;

    vp->cache = NULL;
    resample_buffer_offset = 0;
    while(resample_buffer_offset < n)
    {
	if(cc == 0)
	{
	    if((cc = rs_update_porta(v)) == 0)
	    {
		i = n - resample_buffer_offset;
		resampler(v, &i, mode);
		resample_buffer_offset += i;
		break;
	    }
	}

	i = n - resample_buffer_offset;
	if(i > cc)
	    i = cc;
	resampler(v, &i, mode);
	resample_buffer_offset += i;

	if(!loop && (i == 0 || vp->status == VOICE_FREE))
	    break;
	cc -= i;
    }
    *countptr = resample_buffer_offset;
    resample_buffer_offset = 0;
    vp->porta_control_counter = cc;
    return resample_buffer;
}

/* interface function */
static resample_t *vib_resample_voice(int v, int32 *countptr, int mode)
{
    Voice *vp = &voice[v];

    vp->cache = NULL;
    if(mode == 0)
	return rs_vib_loop(vp, *countptr);
    if(mode == 1)
	return rs_vib_plain(v, countptr);
    return rs_vib_bidir(vp, *countptr);
}

/* interface function */
static resample_t *normal_resample_voice(int v, int32 *countptr, int mode)
{
    Voice *vp = &voice[v];
    if(mode == 0)
	return rs_loop(vp, *countptr);
    if(mode == 1)
	return rs_plain(v, countptr);
    return rs_bidir(vp, *countptr);
}

/* interface function */
resample_t *resample_voice(int v, int32 *countptr)
{
    Voice *vp = &voice[v];
    int mode;
    resample_t *result;
    resampler_t saved_resample;
	int32 i;

    if(vp->sample->sample_rate == play_mode->rate &&
       vp->sample->root_freq == get_note_freq(vp->sample, vp->sample->note_to_use) &&
       vp->frequency == vp->orig_frequency)
    {
	int32 ofs;

	/* Pre-resampled data -- just update the offset and check if
	   we're out of data. */
	ofs = (int32)(vp->sample_offset >> FRACTION_BITS); /* Kind of silly to use
						   FRACTION_BITS here... */
	if(*countptr >= (vp->sample->data_length >> FRACTION_BITS) - ofs)
	{
	    /* Note finished. Free the voice. */
	    vp->timeout = 1;

	    /* Let the caller know how much data we had left */
	    *countptr = (int32)(vp->sample->data_length >> FRACTION_BITS) - ofs;
	}
	else
	    vp->sample_offset += *countptr << FRACTION_BITS;

	for (i = 0; i < *countptr; i++) {
		resample_buffer[i] = vp->sample->data[i + ofs];
	}
	return resample_buffer;
    }

    mode = vp->sample->modes;
    if((mode & MODES_LOOPING) &&
       ((mode & MODES_ENVELOPE) ||
	(vp->status & (VOICE_ON | VOICE_SUSTAINED))))
    {
	if(mode & MODES_PINGPONG)
	{
	    vp->cache = NULL;
	    mode = 2;	/* Bidir loop */
	}
	else
	    mode = 0;	/* loop */
    }
    else
	mode = 1;	/* no loop */

    saved_resample = cur_resample;
#ifndef FIXED_RESAMPLATION
    if (reduce_quality_flag && cur_resample != resample_none)
	cur_resample = resample_linear;
#endif
    if(vp->porta_control_ratio)
	result = porta_resample_voice(v, countptr, mode);
    else if(vp->vibrato_control_ratio)
	result = vib_resample_voice(v, countptr, mode);
    else
	result = normal_resample_voice(v, countptr, mode);

#ifndef FIXED_RESAMPLATION
    cur_resample = saved_resample; /* get back */
#endif
    return result;
}

void pre_resample(Sample * sp)
{
  double a, b;
  splen_t ofs, newlen;
  sample_t *newdata, *dest, *src = (sample_t *)sp->data;
  int32 i, count, incr, f, x;
  resample_rec_t resrc;

  ctl->cmsg(CMSG_INFO, VERB_DEBUG, " * pre-resampling for note %d (%s%d)",
	    sp->note_to_use,
	    note_name[sp->note_to_use % 12], (sp->note_to_use & 0x7F) / 12);

  f = get_note_freq(sp, sp->note_to_use);
  a = b = ((double) (sp->root_freq) * play_mode->rate) /
      ((double) (sp->sample_rate) * f);
  if((int64)sp->data_length * a >= 0x7fffffffL)
  {
      /* Too large to compute */
      ctl->cmsg(CMSG_INFO, VERB_DEBUG, " *** Can't pre-resampling for note %d",
		sp->note_to_use);
      return;
  }
  newlen = (splen_t)(sp->data_length * a);
  count = (newlen >> FRACTION_BITS);
  ofs = incr = (sp->data_length - 1) / (count - 1);

  if((double)newlen + incr >= 0x7fffffffL)
  {
      /* Too large to compute */
      ctl->cmsg(CMSG_INFO, VERB_DEBUG, " *** Can't pre-resampling for note %d",
		sp->note_to_use);
      return;
  }

  dest = newdata = (sample_t *)safe_malloc((int32)(newlen >> (FRACTION_BITS - 1)) + 2);
  dest[newlen >> FRACTION_BITS] = 0;

  *dest++ = src[0];

  resrc.loop_start = 0;
  resrc.loop_end = sp->data_length;
  resrc.data_length = sp->data_length;

  /* Since we're pre-processing and this doesn't have to be done in
     real-time, we go ahead and do the higher order interpolation. */
  for(i = 1; i < count; i++)
  {
	x = cur_resample(src, ofs, &resrc);
    *dest++ = (int16)((x > 32767) ? 32767: ((x < -32768) ? -32768 : x));
    ofs += incr;
  }

  sp->data_length = newlen;
  sp->loop_start = (splen_t)(sp->loop_start * b);
  sp->loop_end = (splen_t)(sp->loop_end * b);
  free(sp->data);
  sp->data = (sample_t *) newdata;
  sp->root_freq = f;
  sp->sample_rate = play_mode->rate;
  sp->low_freq = freq_table[0];
  sp->high_freq = freq_table[127];
}


void free_gauss_table()
{
	free( gauss_table[0] );
	gauss_table[0] = 0;
}
