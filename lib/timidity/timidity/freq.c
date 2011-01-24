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
#include "instrum.h"
#include "freq.h"
#include "fft4g.h"

float *floatdata, *magdata, *prunemagdata;
int *ip;
float *w;
uint32 oldfftsize = 0;
float pitchmags[129];
double pitchbins[129];
double new_pitchbins[129];
int *fft1_bin_to_pitch;

/* middle C = pitch 60 = 261.6 Hz
   freq     = 13.75 * exp((pitch - 9) / 12 * log(2))
   pitch    = 9 - log(13.75 / freq) * 12 / log(2)
            = -36.37631656 + 17.31234049 * log(freq)
*/
float pitch_freq_table[129] = {
    8.17579892, 8.66195722, 9.17702400, 9.72271824, 10.3008612, 10.9133822,
    11.5623257, 12.2498574, 12.9782718, 13.7500000, 14.5676175, 15.4338532,

    16.3515978, 17.3239144, 18.3540480, 19.4454365, 20.6017223, 21.8267645,
    23.1246514, 24.4997147, 25.9565436, 27.5000000, 29.1352351, 30.8677063,

    32.7031957, 34.6478289, 36.7080960, 38.8908730, 41.2034446, 43.6535289,
    46.2493028, 48.9994295, 51.9130872, 55.0000000, 58.2704702, 61.7354127,

    65.4063913, 69.2956577, 73.4161920, 77.7817459, 82.4068892, 87.3070579,
    92.4986057, 97.9988590, 103.826174, 110.000000, 116.540940, 123.470825,

    130.812783, 138.591315, 146.832384, 155.563492, 164.813778, 174.614116,
    184.997211, 195.997718, 207.652349, 220.000000, 233.081881, 246.941651,

    261.625565, 277.182631, 293.664768, 311.126984, 329.627557, 349.228231,
    369.994423, 391.995436, 415.304698, 440.000000, 466.163762, 493.883301,

    523.251131, 554.365262, 587.329536, 622.253967, 659.255114, 698.456463,
    739.988845, 783.990872, 830.609395, 880.000000, 932.327523, 987.766603,

    1046.50226, 1108.73052, 1174.65907, 1244.50793, 1318.51023, 1396.91293,
    1479.97769, 1567.98174, 1661.21879, 1760.00000, 1864.65505, 1975.53321,

    2093.00452, 2217.46105, 2349.31814, 2489.01587, 2637.02046, 2793.82585,
    2959.95538, 3135.96349, 3322.43758, 3520.00000, 3729.31009, 3951.06641,

    4186.00904, 4434.92210, 4698.63629, 4978.03174, 5274.04091, 5587.65170,
    5919.91076, 6271.92698, 6644.87516, 7040.00000, 7458.62018, 7902.13282,

    8372.01809, 8869.84419, 9397.27257, 9956.06348, 10548.0818, 11175.3034,
    11839.8215, 12543.8540, 13289.7503
};



/* center_pitch + 0.49999 */
float pitch_freq_ub_table[129] = {
    8.41536325, 8.91576679, 9.44592587, 10.0076099, 10.6026933, 11.2331623,
    11.9011208, 12.6087983, 13.3585565, 14.1528976, 14.9944727, 15.8860904,
    16.8307265, 17.8315336, 18.8918517, 20.0152197, 21.2053866, 22.4663245,
    23.8022417, 25.2175966, 26.7171129, 28.3057952, 29.9889453, 31.7721808,
    33.6614530, 35.6630672, 37.7837035, 40.0304394, 42.4107732, 44.9326490,
    47.6044834, 50.4351932, 53.4342259, 56.6115903, 59.9778907, 63.5443616,
    67.3229060, 71.3261343, 75.5674070, 80.0608788, 84.8215464, 89.8652980,
    95.2089667, 100.870386, 106.868452, 113.223181, 119.955781, 127.088723,
    134.645812, 142.652269, 151.134814, 160.121758, 169.643093, 179.730596,
    190.417933, 201.740773, 213.736904, 226.446361, 239.911563, 254.177446,
    269.291624, 285.304537, 302.269628, 320.243515, 339.286186, 359.461192,
    380.835867, 403.481546, 427.473807, 452.892723, 479.823125, 508.354893,
    538.583248, 570.609074, 604.539256, 640.487030, 678.572371, 718.922384,
    761.671734, 806.963092, 854.947614, 905.785445, 959.646250, 1016.70979,
    1077.16650, 1141.21815, 1209.07851, 1280.97406, 1357.14474, 1437.84477,
    1523.34347, 1613.92618, 1709.89523, 1811.57089, 1919.29250, 2033.41957,
    2154.33299, 2282.43630, 2418.15702, 2561.94812, 2714.28948, 2875.68954,
    3046.68693, 3227.85237, 3419.79046, 3623.14178, 3838.58500, 4066.83914,
    4308.66598, 4564.87260, 4836.31405, 5123.89624, 5428.57897, 5751.37907,
    6093.37387, 6455.70474, 6839.58092, 7246.28356, 7677.17000, 8133.67829,
    8617.33197, 9129.74519, 9672.62809, 10247.7925, 10857.1579, 11502.7581,
    12186.7477, 12911.4095, 13679.1618
};



/* center_pitch - 0.49999 */
float pitch_freq_lb_table[129] = {
    7.94305438, 8.41537297, 8.91577709, 9.44593678, 10.0076214, 10.6027055,
    11.2331752, 11.9011346, 12.6088129, 13.3585719, 14.1529139, 14.9944900,
    15.8861088, 16.8307459, 17.8315542, 18.8918736, 20.0152428, 21.2054111,
    22.4663505, 23.8022692, 25.2176258, 26.7171438, 28.3058279, 29.9889800,
    31.7722175, 33.6614919, 35.6631084, 37.7837471, 40.0304857, 42.4108222,
    44.9327009, 47.6045384, 50.4352515, 53.4342876, 56.6116557, 59.9779599,
    63.5444350, 67.3229838, 71.3262167, 75.5674943, 80.0609713, 84.8216444,
    89.8654018, 95.2090767, 100.870503, 106.868575, 113.223311, 119.955920,
    127.088870, 134.645968, 142.652433, 151.134989, 160.121943, 169.643289,
    179.730804, 190.418153, 201.741006, 213.737151, 226.446623, 239.911840,
    254.177740, 269.291935, 285.304867, 302.269977, 320.243885, 339.286578,
    359.461607, 380.836307, 403.482012, 427.474301, 452.893246, 479.823680,
    508.355480, 538.583870, 570.609734, 604.539954, 640.487770, 678.573155,
    718.923215, 761.672614, 806.964024, 854.948602, 905.786491, 959.647359,
    1016.71096, 1077.16774, 1141.21947, 1209.07991, 1280.97554, 1357.14631,
    1437.84643, 1523.34523, 1613.92805, 1709.89720, 1811.57298, 1919.29472,
    2033.42192, 2154.33548, 2282.43893, 2418.15982, 2561.95108, 2714.29262,
    2875.69286, 3046.69045, 3227.85610, 3419.79441, 3623.14597, 3838.58944,
    4066.84384, 4308.67096, 4564.87787, 4836.31963, 5123.90216, 5428.58524,
    5751.38572, 6093.38091, 6455.71219, 6839.58882, 7246.29193, 7677.17887,
    8133.68768, 8617.34192, 9129.75574, 9672.63927, 10247.8043, 10857.1705,
    11502.7714, 12186.7618, 12911.4244
};



/* (M)ajor,		rotate back 1,	rotate back 2
   (m)inor,		rotate back 1,	rotate back 2
   (d)iminished minor,	rotate back 1,	rotate back 2
   (f)ifth,		rotate back 1,	rotate back 2
*/
int chord_table[4][3][3] = {
    0, 4, 7,     -5, 0, 4,     -8, -5, 0,
    0, 3, 7,     -5, 0, 3,     -9, -5, 0,
    0, 3, 6,     -6, 0, 3,     -9, -6, 0,
    0, 5, 7,     -5, 0, 5,     -7, -5, 0
};

/* write the chord type to *chord, returns the root note of the chord */
int assign_chord(double *pitchbins, int *chord,
		 int min_guesspitch, int max_guesspitch, int root_pitch)
{

    int type, subtype;
    int pitches[19] = {0};
    int prune_pitches[10] = {0};
    int i, j, k, n, n2;
    double val, cutoff, max;
    int start = 0;
    int root_flag;

    *chord = -1;

    if (root_pitch - 9 > min_guesspitch)
    	min_guesspitch = root_pitch - 9;

    if (min_guesspitch <= LOWEST_PITCH)
    	min_guesspitch = LOWEST_PITCH + 1;

    if (root_pitch + 9 < max_guesspitch)
    	max_guesspitch = root_pitch + 9;

    if (max_guesspitch >= HIGHEST_PITCH)
    	max_guesspitch = HIGHEST_PITCH - 1;

    /* keep only local maxima */
    for (i = min_guesspitch, n = 0; i <= max_guesspitch; i++)
    {
	val = pitchbins[i];
	if (val)
	{
	    if (pitchbins[i-1] < val && pitchbins[i+1] < val)
		pitches[n++] = i;
	}
    }

    if (n < 3)
    	return -1;

    /* find largest peak */
    max = -1;
    for (i = 0; i < n; i++)
    {
    	val = pitchbins[pitches[i]];
    	if (val > max)
    	    max = val;
    }

    /* discard any peaks below cutoff */
    cutoff = 0.2 * max;
    for (i = 0, n2 = 0, root_flag = 0; i < n; i++)
    {
    	val = pitchbins[pitches[i]];
    	if (val >= cutoff)
    	{
    	    prune_pitches[n2++] = pitches[i];
    	    if (pitches[i] == root_pitch)
    	    	root_flag = 1;
    	}
    }
    
    if (!root_flag || n2 < 3)
    	return -1;

    /* search for a chord, must contain root pitch */
    for (i = 0; i < n2; i++)
    {
    	for (subtype = 0; subtype < 3; subtype++)
    	{
    	    if (i + subtype >= n2)
    	    	continue;

	    for (type = 0; type < 4; type++)
	    {
    	    	for (j = 0, n = 0, root_flag = 0; j < 3; j++)
    	    	{
		    k = i + j;

    	    	    if (k >= n2)
    	    	    	continue;
    	    	    
    	    	    if (prune_pitches[k] == root_pitch)
    	    	    	root_flag = 1;

		    if (prune_pitches[k] - prune_pitches[i+subtype] ==
		    	chord_table[type][subtype][j])
			    n++;
    	    	}
	    	if (root_flag && n == 3)
	    	{
		    *chord = 3 * type + subtype;
		    return prune_pitches[i+subtype];
	    	}
    	    }
    	}
    }

    return -1;
}



/* initialize FFT arrays for the frequency analysis */
int freq_initialize_fft_arrays(Sample *sp)
{

    uint32 i;
    uint32 length, newlength;
    unsigned int rate;
    sample_t *origdata;

    rate = sp->sample_rate;
    length = sp->data_length >> FRACTION_BITS;
    origdata = sp->data;

    /* copy the sample to a new float array */
    floatdata = (float *) safe_malloc(length * sizeof(float));
    for (i = 0; i < length; i++)
	floatdata[i] = origdata[i];

    /* length must be a power of 2 */
    /* set it to smallest power of 2 >= 1.4*rate */
    /* at least 1.4*rate is required for decent resolution of low notes */
    newlength = pow(2, ceil(log(1.4*rate) / log(2)));
    if (length < newlength)
    {
	floatdata = safe_realloc(floatdata, newlength * sizeof(float));
	memset(floatdata + length, 0, (newlength - length) * sizeof(float));
    }
    length = newlength;

    /* allocate FFT arrays */
    /* calculate sin/cos and fft1_bin_to_pitch tables */
    if (length != oldfftsize)
    {
        float f0;
    
        if (oldfftsize > 0)
        {
            free(magdata);
            free(prunemagdata);
            free(ip);
            free(w);
            free(fft1_bin_to_pitch);
        }
        magdata = (float *) safe_malloc(length * sizeof(float));
        prunemagdata = (float *) safe_malloc(length * sizeof(float));
        ip = (int *) safe_malloc(2 + sqrt(length) * sizeof(int));
        *ip = 0;
        w = (float *) safe_malloc((length >> 1) * sizeof(float));
        fft1_bin_to_pitch = safe_malloc((length >> 1) * sizeof(float));

        for (i = 1, f0 = (float) rate / length; i < (length >> 1); i++) {
            fft1_bin_to_pitch[i] = assign_pitch_to_freq(i * f0);
        }
    }
    oldfftsize = length;

    /* zero out arrays that need it */
    memset(pitchmags, 0, 129 * sizeof(float));
    memset(pitchbins, 0, 129 * sizeof(double));
    memset(new_pitchbins, 0, 129 * sizeof(double));
    memset(prunemagdata, 0, length * sizeof(float));

    return(length);
}



/* return the frequency of the sample */
/* max of 1.4 - 2.0 seconds of audio is analyzed, depending on sample rate */
/* samples < 1.4 seconds are padded to max length for higher fft accuracy */
float freq_fourier(Sample *sp, int *chord)
{

    uint32 length, length0;
    int32 maxoffset, minoffset, minoffset1, minoffset2;
    int32 minbin, maxbin;
    int32 bin, bestbin, largest_peak;
    int32 i, j, n, total;
    unsigned int rate;
    int pitch, bestpitch, minpitch, maxpitch, maxpitch2;
    sample_t *origdata;
    float f0, mag, maxmag;
    int16 amp, oldamp, maxamp;
    int32 maxpos;
    double sum, weightsum, maxsum;
    double sum_bestfreq;
    double f0_inv;
    int num_maxsum;
    float freq, newfreq, bestfreq, freq_inc;
    float minfreq, maxfreq, minfreq2, maxfreq2;
    float min_guessfreq, max_guessfreq;

    rate = sp->sample_rate;
    length = length0 = sp->data_length >> FRACTION_BITS;
    origdata = sp->data;

    length = freq_initialize_fft_arrays(sp);

    /* base frequency of the FFT */
    f0 = (float) rate / length;
    f0_inv = 1.0 / f0;

    /* get maximum amplitude */
    maxamp = -1;
    for (i = 0; i < length0; i++)
    {
	amp = abs(origdata[i]);
	if (amp >= maxamp)
	{
	    maxamp = amp;
	    maxpos = i;
	}
    }

    /* go out 2 zero crossings in both directions, starting at maxpos */
    /* find the peaks after the 2nd crossing */
    minoffset1 = 0;
    for (n = 0, oldamp = origdata[maxpos], i = maxpos - 1; i >= 0 && n < 2; i--)
    {
	amp = origdata[i];
	if ((oldamp && amp == 0) || (oldamp > 0 && amp < 0) ||
	    (oldamp < 0 && amp > 0))
	    n++;
	oldamp = amp;
    }
    minoffset1 = i;
    maxamp = labs(origdata[i]);
    while (i >= 0)
    {
	amp = origdata[i];
	if ((oldamp && amp == 0) || (oldamp > 0 && amp < 0) ||
	    (oldamp < 0 && amp > 0))
	{
	    break;
	}
	oldamp = amp;

	amp = labs(amp);
	if (amp > maxamp)
	{
	    maxamp = amp;
	    minoffset1 = i;
	}
	i--;
    }

    minoffset2 = 0;
    for (n = 0, oldamp = origdata[maxpos], i = maxpos + 1; i < length0 && n < 2; i++)
    {
	amp = origdata[i];
	if ((oldamp && amp == 0) || (oldamp > 0 && amp < 0) ||
	    (oldamp < 0 && amp > 0))
	    n++;
	oldamp = amp;
    }
    minoffset2 = i;
    maxamp = labs(origdata[i]);
    while (i < length0)
    {
	amp = origdata[i];
	if ((oldamp && amp == 0) || (oldamp > 0 && amp < 0) ||
	    (oldamp < 0 && amp > 0))
	{
	    break;
	}
	oldamp = amp;

	amp = labs(amp);
	if (amp > maxamp)
	{
	    maxamp = amp;
	    minoffset2 = i;
	}
	i++;
    }

    /* upper bound on the detected frequency */
    /* distance between the two peaks is at most 2 periods */
    minoffset = (minoffset2 - minoffset1);
    if (minoffset < 4)
	minoffset = 4;
    max_guessfreq = (float) rate / (minoffset * 0.5);
    if (max_guessfreq >= (rate >> 1)) max_guessfreq = (rate >> 1) - 1;

    /* lower bound on the detected frequency */
    maxoffset = rate / pitch_freq_lb_table[LOWEST_PITCH] + 0.5;
    if (maxoffset > (length >> 1))
	maxoffset = (length >> 1);
    min_guessfreq = (float) rate / maxoffset;

    /* perform the in place FFT */
    rdft(length, 1, floatdata, ip, w);

    /* calc the magnitudes */
    for (i = 2; i < length; i++)
    {
	mag = floatdata[i++];
	mag *= mag;
	mag += floatdata[i] * floatdata[i];
	magdata[i >> 1] = sqrt(mag);
    }

    /* find max mag */
    maxmag = 0;
    for (i = 1; i < (length >> 1); i++)
    {
	mag = magdata[i];

	pitch = fft1_bin_to_pitch[i];
	if (pitch && mag > maxmag)
	    maxmag = mag;
    }
    
    /* Apply non-linear scaling to the magnitudes
     * I don't know why this improves the pitch detection, but it does
     * The best choice of power seems to be between 1.64 - 1.68
     */
    for (i = 1; i < (length >> 1); i++)
	magdata[i] = maxmag * pow(magdata[i] / maxmag, 1.66);

    /* bin the pitches */
    for (i = 1; i < (length >> 1); i++)
    {
	mag = magdata[i];

	pitch = fft1_bin_to_pitch[i];
	pitchbins[pitch] += mag;

	if (mag > pitchmags[pitch])
	    pitchmags[pitch] = mag;
    }

    /* zero out lowest pitch, since it contains all lower frequencies too */
    pitchbins[LOWEST_PITCH] = 0;

    /* find the largest peak */
    for (i = LOWEST_PITCH + 1, maxsum = -42; i <= HIGHEST_PITCH; i++)
    {
	sum = pitchbins[i];
	if (sum > maxsum)
	{
	    maxsum = sum;
	    largest_peak = i;
	}
    }

    minpitch = assign_pitch_to_freq(min_guessfreq);
    if (minpitch > HIGHEST_PITCH) minpitch = HIGHEST_PITCH;

    /* zero out any peak below minpitch */
    for (i = LOWEST_PITCH + 1; i < minpitch; i++)
	pitchbins[i] = 0;

    /* remove all pitches below threshold */
    for (i = minpitch; i <= HIGHEST_PITCH; i++)
    {
	if (pitchbins[i] / maxsum < 0.01 && pitchmags[i] / maxmag < 0.01)
	    pitchbins[i] = 0;
    }

    /* keep local maxima */
    for (i = LOWEST_PITCH + 1; i < HIGHEST_PITCH; i++)
    {
    	double temp;

	temp = pitchbins[i];
	    
    	/* also keep significant bands to either side */
    	if (temp && pitchbins[i-1] < temp && pitchbins[i+1] < temp)
    	{
    	    new_pitchbins[i] = temp;

    	    temp *= 0.5;
    	    if (pitchbins[i-1] >= temp)
    	    	new_pitchbins[i-1] = pitchbins[i-1];
    	    if (pitchbins[i+1] >= temp)
    	    	new_pitchbins[i+1] = pitchbins[i-1];
    	}
    }
    memcpy(pitchbins, new_pitchbins, 129 * sizeof(double));

    /* find lowest and highest pitches */
    minpitch = LOWEST_PITCH;
    while (minpitch < HIGHEST_PITCH && !pitchbins[minpitch])
    	minpitch++;
    maxpitch = HIGHEST_PITCH;
    while (maxpitch > LOWEST_PITCH && !pitchbins[maxpitch])
    	maxpitch--;

    /* uh oh, no pitches left...
     * best guess is middle C
     * return 260 Hz, since exactly 260 Hz is never returned except on error
     * this should only occur on blank/silent samples
     */
    if (maxpitch < minpitch)
    {
    	free(floatdata);
    	return 260.0;
    }

    /* pitch assignment bounds based on zero crossings and pitches kept */
    if (pitch_freq_lb_table[minpitch] > min_guessfreq)
    	min_guessfreq = pitch_freq_lb_table[minpitch];
    if (pitch_freq_ub_table[maxpitch] < max_guessfreq)
    	max_guessfreq = pitch_freq_ub_table[maxpitch];

    minfreq = pitch_freq_lb_table[minpitch];
    if (minfreq >= (rate >> 1)) minfreq = (rate >> 1) - 1;
    
    maxfreq = pitch_freq_ub_table[maxpitch];
    if (maxfreq >= (rate >> 1)) maxfreq = (rate >> 1) - 1;

    minbin = minfreq / f0;
    if (!minbin)
	minbin = 1;
    maxbin = ceil(maxfreq / f0);
    if (maxbin >= (length >> 1))
    	maxbin = (length >> 1) - 1;

    /* filter out all "noise" from magnitude array */
    for (i = minbin, n = 0; i <= maxbin; i++)
    {
	pitch = fft1_bin_to_pitch[i];
	if (pitchbins[pitch])
	{
	    prunemagdata[i] = magdata[i];
	    n++;
	}
    }

    /* whoa!, there aren't any strong peaks at all !!! bomb early
     * best guess is middle C
     * return 260 Hz, since exactly 260 Hz is never returned except on error
     * this should only occur on blank/silent samples
     */
    if (!n)
    {
    	free(floatdata);
	return 260.0;
    }

    memset(new_pitchbins, 0, 129 * sizeof(double));

    maxsum = -1;
    minpitch = assign_pitch_to_freq(min_guessfreq);
    maxpitch = assign_pitch_to_freq(max_guessfreq);
    maxpitch2 = assign_pitch_to_freq(max_guessfreq) + 9;
    if (maxpitch2 > HIGHEST_PITCH) maxpitch2 = HIGHEST_PITCH;

    /* initial guess is first local maximum */
    bestfreq = pitch_freq_table[minpitch];
    if (minpitch < HIGHEST_PITCH &&
    	pitchbins[minpitch+1] > pitchbins[minpitch])
    	    bestfreq = pitch_freq_table[minpitch+1];

    /* find best fundamental */
    for (i = minpitch; i <= maxpitch2; i++)
    {
	if (!pitchbins[i])
	    continue;

    	minfreq2 = pitch_freq_lb_table[i];
    	maxfreq2 = pitch_freq_ub_table[i];
    	freq_inc = (maxfreq2 - minfreq2) * 0.1;
    	if (minfreq2 >= (rate >> 1)) minfreq2 = (rate >> 1) - 1;
    	if (maxfreq2 >= (rate >> 1)) maxfreq2 = (rate >> 1) - 1;

	/* look for harmonics */
	for (freq = minfreq2; freq <= maxfreq2; freq += freq_inc)
	{
	    double ratio;
	
    	    n = total = 0;
    	    sum = weightsum = 0;

	    for (j = 1; j <= 32 && (newfreq = j*freq) <= maxfreq; j++)
    	    {
    	    	pitch = assign_pitch_to_freq(newfreq);

    	    	if (pitchbins[pitch])
    	    	{
    	    	    sum += pitchbins[pitch];
    	    	    n++;
    	    	    total = j;
    	  	}
    	    }

	    /* only pitches with good harmonics are assignment candidates */
	    if (n > 1)
	    {
	    	double ratio;
	    	
	    	ratio = (double) n / total;
	    	if (ratio >= 0.333333)
	    	{
	    	    weightsum = ratio * sum;
	    	    pitch = assign_pitch_to_freq(freq);

		    /* use only these pitches for chord detection */
	    	    if (pitch <= HIGHEST_PITCH && pitchbins[pitch])
	    	    	new_pitchbins[pitch] = weightsum;

		    if (pitch > maxpitch)
		    	continue;

    	    	    if (n < 2 || weightsum > maxsum)
    	    	    {
    	    	    	maxsum = weightsum;
    	    	    	bestfreq = freq;
    	    	    }
    	    	}
    	    }
    	}
    }

    bestpitch = assign_pitch_to_freq(bestfreq);

    /* assign chords */
    if ((pitch = assign_chord(new_pitchbins, chord,
    	bestpitch - 9, maxpitch2, bestpitch)) >= 0)
    	    bestpitch = pitch;

    bestfreq = pitch_freq_table[bestpitch];

    /* tune based on the fundamental and harmonics up to +5 octaves */
    sum = weightsum = 0;
    for (i = 1; i <= 32 && (freq = i*bestfreq) <= maxfreq; i++)
    {
	double tune;

	minfreq2 = pitch_freq_lb_table[bestpitch];
	maxfreq2 = pitch_freq_ub_table[bestpitch];

	minbin = minfreq2 * f0_inv;
	if (!minbin) minbin = 1;
	maxbin = ceil(maxfreq2 * f0_inv);
	if (maxbin >= (length>>1))
	    maxbin = (length>>1) - 1;

	for (bin = minbin; bin <= maxbin; bin++)
	{
	    tune = -36.37631656 + 17.31234049 * log(bin*f0) - bestpitch;
	    sum += magdata[bin];
	    weightsum += magdata[bin] * tune;
	}
    }

    bestfreq = 13.75 * exp(((bestpitch + weightsum / sum) - 9) /
    	       12 * log(2));

    /* Since we are using exactly 260 Hz as an error code, fudge the freq
     * on the extremely unlikely chance that the detected pitch is exactly
     * 260 Hz.
     */
    if (bestfreq == 260.0)
    	bestfreq += 1E-5;

    free(floatdata);

    return bestfreq;
}



int assign_pitch_to_freq(float freq)
{

    int pitch;

    /* round to nearest integer using: ceil(fraction - 0.5) */
    /* -0.5 is already added into the first constant below */
    pitch = ceil(-36.87631656f + 17.31234049f * log(freq));

    /* min and max pitches */
    if (pitch < LOWEST_PITCH) pitch = LOWEST_PITCH;
    else if (pitch > HIGHEST_PITCH) pitch = HIGHEST_PITCH;

    return pitch;
}
