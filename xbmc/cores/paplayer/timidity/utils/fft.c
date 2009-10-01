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
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "timidity.h"
#include "common.h"
#include "fft.h"

#ifdef DEBUG
#ifdef sun
int fprintf(FILE *, const char *, ...);
#endif
#endif


#define swap(x, j, k)	\
{			\
    double tmp;		\
    tmp = x[j];		\
    x[j] = x[k];	\
    x[k] = tmp;		\
}

#define swap2(x, j, k)	\
{			\
    double tmp;		\
    tmp = x[j];		\
    x[j] = -x[k];	\
    x[k] = tmp;		\
}

static void make_table(double *trig, int *b, int n)
{
    unsigned long i, j, k, bitmask;

    /* check n */
    for(i = n; !(i & 1); i >>= 1)
	;
    if(i != 1)
    {
	fprintf(stderr, "Invalid fft data size: %d\n", n);
	exit(1);
    }

    /* make bitrev table */
    for(i = 0; i < n; i++)
	b[i] = 0;
    bitmask = n / 2;
    for(i = 1; i < n; i <<= 1, bitmask >>= 1)
	for(j = 0; j < n; j += i * 2)
	    for(k = j + i; k < j + i * 2; k++)
		b[k] = (int)((unsigned long)b[k] | bitmask);

    /* make trig table */
    for(i = 0; i < n; i++)
    {
	j = i * 2;
	trig[j  ] = cos((2 * M_PI) * i / (double)n);
	trig[j+1] = sin((2 * M_PI) * i / (double)n);
    }

    for(i = 0; i < n; i++)
	if(i < b[i])
	{
	    j = b[i] * 2;
	    swap(trig, i*2, j);
	    swap(trig, i*2+1, j+1);
	}

}

void realfft(double *x, int n_arg)
{
    int n = n_arg;
    static double *trig_table = NULL;
    static int *bitrev_table;
    int n1;

    if(n == 0)
    {
	if(trig_table == NULL)
	    return;
	free(trig_table);
	free(bitrev_table);
	trig_table = NULL;
	return;
    }

    if(trig_table == NULL)
    {
	trig_table = (double *)safe_malloc((n * 2) * sizeof(double));
	bitrev_table = (int *)safe_malloc(n * sizeof(int));

	if(!(trig_table && bitrev_table))
	{
	    fprintf(stderr, "fft: Can't allocate memroy.\n");
	    exit(1);
	}

	make_table(trig_table, bitrev_table, n);

	if(x == NULL)		/* initialize only */
	    return;
    }
    /* end of initialize tables */


    /* first step: butterfly w^0 */
    /* n1 = n/2, n/4, n/8, ... 1 */
    for(n1 = n >> 1; n1 > 0; n1 >>= 1)
    {
	int i;

	for(i = 0; i < n1; i++)
	{
	    double x1, x2;

	    x[i]    = (x1 = x[i]) + (x2 = x[i+n1]);
	    x[i+n1] = x1 - x2;
	}
    }

    /* main loop */
    /* n1 = n/8, n/16, n/32, ... 1 */
    for(n1 = n >> 3; n1 > 0; n1 >>= 1)
    {
	int i;
	int wi0;

	wi0 = 8;
	for(i = n1 << 2; i < n; i <<= 1, wi0 <<= 1)
	{
	    double *imp;	/* pointer to start of imaginary part */
	    double *rep;	/* pointer to start of real part  */
	    double *imp2;	/* pointer to start of imaginary part */
	    double *rep2;	/* pointer to start of real part  */
	    int wi, j0;
	    int in;

	    in   = i >> 1;
	    wi   = wi0;
	    rep  = x + i;
	    imp  = rep + in;
	    rep2 = rep + n1;
	    imp2 = imp + n1;

	    for(j0 = 0; j0 < in; j0 += (n1 << 1), wi += 4)
	    {
		int j, jn;
		double c, s;

		c  = trig_table[wi];
		s  = trig_table[wi+1];
		jn = j0 + n1;

		/* butterfly */
		for(j = j0; j < jn; j++)
		{
		    double xi1, xr1, xi2, xr2, ti, tr;

		    rep[j]  = (xr1 = rep[j]) + (tr = c * (xr2 = rep2[j])
					     - s * (xi2 = imp2[j]));
		    imp[j]  = (xi1 = imp[j]) + (ti = s * xr2 + c * xi2);
		    rep2[j] = xr1 - tr;
		    imp2[j] = xi1 - ti;
		}
	    }
	}
    }

    /* move data */
    {
	int ii, i, j, i2, i4;

	for(i = 4; i < n; i = ii)
	{
	    i2 = i >> 1;
	    i4 = i2 >> 1;
	    ii = i << 1;
	    for(j = 0; j < i4; j++)
		swap(x, j + i + i2, ii - j - 1);
	    for(j = 1; j < i2; j += 2)
		swap2(x, j + i, ii - j - 1);
	}

	for(i = 0; i < n; i++)
	    if(i < bitrev_table[i])
		swap(x, i, bitrev_table[i]);
	for(i = n/2+1; i < n; i++)
	    x[i] = -x[i];
    }
}
