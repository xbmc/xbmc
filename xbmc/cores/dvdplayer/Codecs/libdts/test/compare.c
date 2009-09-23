/*
 * compare.c
 * Copyright (C) 2000-2003 Michel Lespinasse <walken@zoy.org>
 * Copyright (C) 1999-2000 Aaron Holtzman <aholtzma@ess.engr.uvic.ca>
 *
 * This file is part of dtsdec, a free DTS Coherent Acoustics stream decoder.
 * See http://www.videolan.org/dtsdec.html for updates.
 *
 * dtsdec is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * dtsdec is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>
#include <math.h>

int main (int argc, char ** argv)
{
    FILE * f1;
    FILE * f2;
    float buf1[512];
    float buf2[512];
    int i, j;
    int total = 0;
    double max = 0, err = 0, square = 0;

    if (argc != 3)
	return 1;
    f1 = fopen (argv[1], "rb");
    f2 = fopen (argv[2], "rb");
    if ((f1 == NULL) || (f2 == NULL)) {
	printf ("cannot open file %s\n", (f1 == NULL) ? argv[1] : argv[2]);
	return 1;
    }
    while (1) {
	i = fread (buf1, sizeof (float), 512, f1);
	j = fread (buf2, sizeof (float), 512, f2);
	if ((i < 512) || (j < 512))
	    break;
	for (i = 0; i < 512; i++) {
	    double delta;

	    delta = buf2[i] - buf1[i];
	    err += delta;
	    square += delta * delta;
	    if (delta > max)
		max = delta;
	    if (-delta > max)
		max = -delta;
	}
	total += 512;
    }
    if (i == j) {
	err /= total;
	square = (square / total) - (err * err);
	if (square > 0)
	    square = 32768 * sqrt (square);
	err *= 32768;
	max *= 32768;
	printf ("max error %f mean error %f standard deviation %f\n",
		max, err, square);
	return ((max > 0.01) || (err > 0.001) || (square > 0.001));
    }
    if (i < j)
	printf ("%s is too short\n", argv[1]);
    else
	printf ("%s is too short\n", argv[2]);
    return 1;
}
