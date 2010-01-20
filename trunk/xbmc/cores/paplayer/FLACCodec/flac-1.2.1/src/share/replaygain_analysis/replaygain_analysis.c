/*
 *  ReplayGainAnalysis - analyzes input samples and give the recommended dB change
 *  Copyright (C) 2001 David Robinson and Glen Sawyer
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  concept and filter values by David Robinson (David@Robinson.org)
 *    -- blame him if you think the idea is flawed
 *  original coding by Glen Sawyer (glensawyer@hotmail.com)
 *    -- blame him if you think this runs too slowly, or the coding is otherwise flawed
 *
 *  lots of code improvements by Frank Klemm ( http://www.uni-jena.de/~pfk/mpp/ )
 *    -- credit him for all the _good_ programming ;)
 *
 *  minor cosmetic tweaks to integrate with FLAC by Josh Coalson
 *
 *
 *  For an explanation of the concepts and the basic algorithms involved, go to:
 *    http://www.replaygain.org/
 */

/*
 *  Here's the deal. Call
 *
 *    InitGainAnalysis ( long samplefreq );
 *
 *  to initialize everything. Call
 *
 *    AnalyzeSamples ( const Float_t*  left_samples,
 *                     const Float_t*  right_samples,
 *                     size_t          num_samples,
 *                     int             num_channels );
 *
 *  as many times as you want, with as many or as few samples as you want.
 *  If mono, pass the sample buffer in through left_samples, leave
 *  right_samples NULL, and make sure num_channels = 1.
 *
 *    GetTitleGain()
 *
 *  will return the recommended dB level change for all samples analyzed
 *  SINCE THE LAST TIME you called GetTitleGain() OR InitGainAnalysis().
 *
 *    GetAlbumGain()
 *
 *  will return the recommended dB level change for all samples analyzed
 *  since InitGainAnalysis() was called and finalized with GetTitleGain().
 *
 *  Pseudo-code to process an album:
 *
 *    Float_t       l_samples [4096];
 *    Float_t       r_samples [4096];
 *    size_t        num_samples;
 *    unsigned int  num_songs;
 *    unsigned int  i;
 *
 *    InitGainAnalysis ( 44100 );
 *    for ( i = 1; i <= num_songs; i++ ) {
 *        while ( ( num_samples = getSongSamples ( song[i], left_samples, right_samples ) ) > 0 )
 *            AnalyzeSamples ( left_samples, right_samples, num_samples, 2 );
 *        fprintf ("Recommended dB change for song %2d: %+6.2f dB\n", i, GetTitleGain() );
 *    }
 *    fprintf ("Recommended dB change for whole album: %+6.2f dB\n", GetAlbumGain() );
 */

/*
 *  So here's the main source of potential code confusion:
 *
 *  The filters applied to the incoming samples are IIR filters,
 *  meaning they rely on up to <filter order> number of previous samples
 *  AND up to <filter order> number of previous filtered samples.
 *
 *  I set up the AnalyzeSamples routine to minimize memory usage and interface
 *  complexity. The speed isn't compromised too much (I don't think), but the
 *  internal complexity is higher than it should be for such a relatively
 *  simple routine.
 *
 *  Optimization/clarity suggestions are welcome.
 */

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "replaygain_analysis.h"

Float_t ReplayGainReferenceLoudness = 89.0; /* in dB SPL */

typedef unsigned short  Uint16_t;
typedef signed short    Int16_t;
typedef unsigned int    Uint32_t;
typedef signed int      Int32_t;

#define YULE_ORDER         10
#define BUTTER_ORDER        2
#define RMS_PERCENTILE      0.95        /* percentile which is louder than the proposed level */
#define MAX_SAMP_FREQ   48000.          /* maximum allowed sample frequency [Hz] */
#define RMS_WINDOW_TIME     0.050       /* Time slice size [s] */
#define STEPS_per_dB      100.          /* Table entries per dB */
#define MAX_dB            120.          /* Table entries for 0...MAX_dB (normal max. values are 70...80 dB) */

#define MAX_ORDER               (BUTTER_ORDER > YULE_ORDER ? BUTTER_ORDER : YULE_ORDER)
/* [JEC] the following was originally #defined as:
 *   (size_t) (MAX_SAMP_FREQ * RMS_WINDOW_TIME)
 * but that seemed to fail to take into account the ceil() part of the
 * sampleWindow calculation in ResetSampleFrequency(), and was causing
 * buffer overflows for 48kHz analysis, hence the +1.
 */
#ifndef __sun
 #define MAX_SAMPLES_PER_WINDOW  (size_t) (MAX_SAMP_FREQ * RMS_WINDOW_TIME + 1.)   /* max. Samples per Time slice */
#else
 /* [JEC] Solaris Forte compiler doesn't like float calc in array indices */
 #define MAX_SAMPLES_PER_WINDOW  (size_t) (2401)
#endif
#define PINK_REF                64.82 /* 298640883795 */                          /* calibration value */

static Float_t          linprebuf [MAX_ORDER * 2];
static Float_t*         linpre;                                          /* left input samples, with pre-buffer */
static Float_t          lstepbuf  [MAX_SAMPLES_PER_WINDOW + MAX_ORDER];
static Float_t*         lstep;                                           /* left "first step" (i.e. post first filter) samples */
static Float_t          loutbuf   [MAX_SAMPLES_PER_WINDOW + MAX_ORDER];
static Float_t*         lout;                                            /* left "out" (i.e. post second filter) samples */
static Float_t          rinprebuf [MAX_ORDER * 2];
static Float_t*         rinpre;                                          /* right input samples ... */
static Float_t          rstepbuf  [MAX_SAMPLES_PER_WINDOW + MAX_ORDER];
static Float_t*         rstep;
static Float_t          routbuf   [MAX_SAMPLES_PER_WINDOW + MAX_ORDER];
static Float_t*         rout;
static unsigned int              sampleWindow;                           /* number of samples required to reach number of milliseconds required for RMS window */
static unsigned long    totsamp;
static double           lsum;
static double           rsum;
static int              freqindex;
#ifndef __sun
static Uint32_t  A [(size_t)(STEPS_per_dB * MAX_dB)];
static Uint32_t  B [(size_t)(STEPS_per_dB * MAX_dB)];
#else
/* [JEC] Solaris Forte compiler doesn't like float calc in array indices */
static Uint32_t  A [12000];
static Uint32_t  B [12000];
#endif

/* for each filter:
   [0] 48 kHz, [1] 44.1 kHz, [2] 32 kHz, [3] 24 kHz, [4] 22050 Hz, [5] 16 kHz, [6] 12 kHz, [7] is 11025 Hz, [8] 8 kHz */

#ifdef WIN32
#pragma warning ( disable : 4305 )
#endif

static const Float_t  AYule [9] [11] = {
    { 1., -3.84664617118067,  7.81501653005538,-11.34170355132042, 13.05504219327545,-12.28759895145294,  9.48293806319790, -5.87257861775999,  2.75465861874613, -0.86984376593551, 0.13919314567432 },
    { 1., -3.47845948550071,  6.36317777566148, -8.54751527471874,  9.47693607801280, -8.81498681370155,  6.85401540936998, -4.39470996079559,  2.19611684890774, -0.75104302451432, 0.13149317958808 },
    { 1., -2.37898834973084,  2.84868151156327, -2.64577170229825,  2.23697657451713, -1.67148153367602,  1.00595954808547, -0.45953458054983,  0.16378164858596, -0.05032077717131, 0.02347897407020 },
    { 1., -1.61273165137247,  1.07977492259970, -0.25656257754070, -0.16276719120440, -0.22638893773906,  0.39120800788284, -0.22138138954925,  0.04500235387352,  0.02005851806501, 0.00302439095741 },
    { 1., -1.49858979367799,  0.87350271418188,  0.12205022308084, -0.80774944671438,  0.47854794562326, -0.12453458140019, -0.04067510197014,  0.08333755284107, -0.04237348025746, 0.02977207319925 },
    { 1., -0.62820619233671,  0.29661783706366, -0.37256372942400,  0.00213767857124, -0.42029820170918,  0.22199650564824,  0.00613424350682,  0.06747620744683,  0.05784820375801, 0.03222754072173 },
    { 1., -1.04800335126349,  0.29156311971249, -0.26806001042947,  0.00819999645858,  0.45054734505008, -0.33032403314006,  0.06739368333110, -0.04784254229033,  0.01639907836189, 0.01807364323573 },
    { 1., -0.51035327095184, -0.31863563325245, -0.20256413484477,  0.14728154134330,  0.38952639978999, -0.23313271880868, -0.05246019024463, -0.02505961724053,  0.02442357316099, 0.01818801111503 },
    { 1., -0.25049871956020, -0.43193942311114, -0.03424681017675, -0.04678328784242,  0.26408300200955,  0.15113130533216, -0.17556493366449, -0.18823009262115,  0.05477720428674, 0.04704409688120 }
};

static const Float_t  BYule [9] [11] = {
    { 0.03857599435200, -0.02160367184185, -0.00123395316851, -0.00009291677959, -0.01655260341619,  0.02161526843274, -0.02074045215285,  0.00594298065125,  0.00306428023191,  0.00012025322027,  0.00288463683916 },
    { 0.05418656406430, -0.02911007808948, -0.00848709379851, -0.00851165645469, -0.00834990904936,  0.02245293253339, -0.02596338512915,  0.01624864962975, -0.00240879051584,  0.00674613682247, -0.00187763777362 },
    { 0.15457299681924, -0.09331049056315, -0.06247880153653,  0.02163541888798, -0.05588393329856,  0.04781476674921,  0.00222312597743,  0.03174092540049, -0.01390589421898,  0.00651420667831, -0.00881362733839 },
    { 0.30296907319327, -0.22613988682123, -0.08587323730772,  0.03282930172664, -0.00915702933434, -0.02364141202522, -0.00584456039913,  0.06276101321749, -0.00000828086748,  0.00205861885564, -0.02950134983287 },
    { 0.33642304856132, -0.25572241425570, -0.11828570177555,  0.11921148675203, -0.07834489609479, -0.00469977914380, -0.00589500224440,  0.05724228140351,  0.00832043980773, -0.01635381384540, -0.01760176568150 },
    { 0.44915256608450, -0.14351757464547, -0.22784394429749, -0.01419140100551,  0.04078262797139, -0.12398163381748,  0.04097565135648,  0.10478503600251, -0.01863887810927, -0.03193428438915,  0.00541907748707 },
    { 0.56619470757641, -0.75464456939302,  0.16242137742230,  0.16744243493672, -0.18901604199609,  0.30931782841830, -0.27562961986224,  0.00647310677246,  0.08647503780351, -0.03788984554840, -0.00588215443421 },
    { 0.58100494960553, -0.53174909058578, -0.14289799034253,  0.17520704835522,  0.02377945217615,  0.15558449135573, -0.25344790059353,  0.01628462406333,  0.06920467763959, -0.03721611395801, -0.00749618797172 },
    { 0.53648789255105, -0.42163034350696, -0.00275953611929,  0.04267842219415, -0.10214864179676,  0.14590772289388, -0.02459864859345, -0.11202315195388, -0.04060034127000,  0.04788665548180, -0.02217936801134 }
};

static const Float_t  AButter [9] [3] = {
    { 1., -1.97223372919527, 0.97261396931306 },
    { 1., -1.96977855582618, 0.97022847566350 },
    { 1., -1.95835380975398, 0.95920349965459 },
    { 1., -1.95002759149878, 0.95124613669835 },
    { 1., -1.94561023566527, 0.94705070426118 },
    { 1., -1.92783286977036, 0.93034775234268 },
    { 1., -1.91858953033784, 0.92177618768381 },
    { 1., -1.91542108074780, 0.91885558323625 },
    { 1., -1.88903307939452, 0.89487434461664 }
};

static const Float_t  BButter [9] [3] = {
    { 0.98621192462708, -1.97242384925416, 0.98621192462708 },
    { 0.98500175787242, -1.97000351574484, 0.98500175787242 },
    { 0.97938932735214, -1.95877865470428, 0.97938932735214 },
    { 0.97531843204928, -1.95063686409857, 0.97531843204928 },
    { 0.97316523498161, -1.94633046996323, 0.97316523498161 },
    { 0.96454515552826, -1.92909031105652, 0.96454515552826 },
    { 0.96009142950541, -1.92018285901082, 0.96009142950541 },
    { 0.95856916599601, -1.91713833199203, 0.95856916599601 },
    { 0.94597685600279, -1.89195371200558, 0.94597685600279 }
};

#ifdef WIN32
#pragma warning ( default : 4305 )
#endif

/* When calling this procedure, make sure that ip[-order] and op[-order] point to real data! */

static void
filter ( const Float_t* input, Float_t* output, size_t nSamples, const Float_t* a, const Float_t* b, size_t order )
{
    double  y;
    size_t  i;
    size_t  k;

    for ( i = 0; i < nSamples; i++ ) {
        y = input[i] * b[0];
        for ( k = 1; k <= order; k++ )
            y += input[i-k] * b[k] - output[i-k] * a[k];
        output[i] = (Float_t)y;
    }
}

/* returns a INIT_GAIN_ANALYSIS_OK if successful, INIT_GAIN_ANALYSIS_ERROR if not */

int
ResetSampleFrequency ( long samplefreq ) {
    int  i;

    /* zero out initial values */
    for ( i = 0; i < MAX_ORDER; i++ )
        linprebuf[i] = lstepbuf[i] = loutbuf[i] = rinprebuf[i] = rstepbuf[i] = routbuf[i] = 0.;

    switch ( (int)(samplefreq) ) {
        case 48000: freqindex = 0; break;
        case 44100: freqindex = 1; break;
        case 32000: freqindex = 2; break;
        case 24000: freqindex = 3; break;
        case 22050: freqindex = 4; break;
        case 16000: freqindex = 5; break;
        case 12000: freqindex = 6; break;
        case 11025: freqindex = 7; break;
        case  8000: freqindex = 8; break;
        default:    return INIT_GAIN_ANALYSIS_ERROR;
    }

    sampleWindow = (int) ceil (samplefreq * RMS_WINDOW_TIME);

    lsum         = 0.;
    rsum         = 0.;
    totsamp      = 0;

    memset ( A, 0, sizeof(A) );

	return INIT_GAIN_ANALYSIS_OK;
}

int
InitGainAnalysis ( long samplefreq )
{
	if (ResetSampleFrequency(samplefreq) != INIT_GAIN_ANALYSIS_OK) {
		return INIT_GAIN_ANALYSIS_ERROR;
	}

    linpre       = linprebuf + MAX_ORDER;
    rinpre       = rinprebuf + MAX_ORDER;
    lstep        = lstepbuf  + MAX_ORDER;
    rstep        = rstepbuf  + MAX_ORDER;
    lout         = loutbuf   + MAX_ORDER;
    rout         = routbuf   + MAX_ORDER;

    memset ( B, 0, sizeof(B) );

    return INIT_GAIN_ANALYSIS_OK;
}

/* returns GAIN_ANALYSIS_OK if successful, GAIN_ANALYSIS_ERROR if not */

int
AnalyzeSamples ( const Float_t* left_samples, const Float_t* right_samples, size_t num_samples, int num_channels )
{
    const Float_t*  curleft;
    const Float_t*  curright;
    long            batchsamples;
    long            cursamples;
    long            cursamplepos;
    int             i;

    if ( num_samples == 0 )
        return GAIN_ANALYSIS_OK;

    cursamplepos = 0;
    batchsamples = num_samples;

    switch ( num_channels) {
    case  1: right_samples = left_samples;
    case  2: break;
    default: return GAIN_ANALYSIS_ERROR;
    }

    if ( num_samples < MAX_ORDER ) {
        memcpy ( linprebuf + MAX_ORDER, left_samples , num_samples * sizeof(Float_t) );
        memcpy ( rinprebuf + MAX_ORDER, right_samples, num_samples * sizeof(Float_t) );
    }
    else {
        memcpy ( linprebuf + MAX_ORDER, left_samples,  MAX_ORDER   * sizeof(Float_t) );
        memcpy ( rinprebuf + MAX_ORDER, right_samples, MAX_ORDER   * sizeof(Float_t) );
    }

    while ( batchsamples > 0 ) {
        cursamples = batchsamples > (long)(sampleWindow-totsamp)  ?  (long)(sampleWindow - totsamp)  :  batchsamples;
        if ( cursamplepos < MAX_ORDER ) {
            curleft  = linpre+cursamplepos;
            curright = rinpre+cursamplepos;
            if (cursamples > MAX_ORDER - cursamplepos )
                cursamples = MAX_ORDER - cursamplepos;
        }
        else {
            curleft  = left_samples  + cursamplepos;
            curright = right_samples + cursamplepos;
        }

        filter ( curleft , lstep + totsamp, cursamples, AYule[freqindex], BYule[freqindex], YULE_ORDER );
        filter ( curright, rstep + totsamp, cursamples, AYule[freqindex], BYule[freqindex], YULE_ORDER );

        filter ( lstep + totsamp, lout + totsamp, cursamples, AButter[freqindex], BButter[freqindex], BUTTER_ORDER );
        filter ( rstep + totsamp, rout + totsamp, cursamples, AButter[freqindex], BButter[freqindex], BUTTER_ORDER );

        for ( i = 0; i < cursamples; i++ ) {             /* Get the squared values */
            lsum += lout [totsamp+i] * lout [totsamp+i];
            rsum += rout [totsamp+i] * rout [totsamp+i];
        }

        batchsamples -= cursamples;
        cursamplepos += cursamples;
        totsamp      += cursamples;
        if ( totsamp == sampleWindow ) {  /* Get the Root Mean Square (RMS) for this set of samples */
            double  val  = STEPS_per_dB * 10. * log10 ( (lsum+rsum) / totsamp * 0.5 + 1.e-37 );
            int     ival = (int) val;
            if ( ival <                     0 ) ival = 0;
            if ( ival >= (int)(sizeof(A)/sizeof(*A)) ) ival = (int)(sizeof(A)/sizeof(*A)) - 1;
            A [ival]++;
            lsum = rsum = 0.;
            memmove ( loutbuf , loutbuf  + totsamp, MAX_ORDER * sizeof(Float_t) );
            memmove ( routbuf , routbuf  + totsamp, MAX_ORDER * sizeof(Float_t) );
            memmove ( lstepbuf, lstepbuf + totsamp, MAX_ORDER * sizeof(Float_t) );
            memmove ( rstepbuf, rstepbuf + totsamp, MAX_ORDER * sizeof(Float_t) );
            totsamp = 0;
        }
        if ( totsamp > sampleWindow )   /* somehow I really screwed up: Error in programming! Contact author about totsamp > sampleWindow */
            return GAIN_ANALYSIS_ERROR;
    }
    if ( num_samples < MAX_ORDER ) {
        memmove ( linprebuf,                           linprebuf + num_samples, (MAX_ORDER-num_samples) * sizeof(Float_t) );
        memmove ( rinprebuf,                           rinprebuf + num_samples, (MAX_ORDER-num_samples) * sizeof(Float_t) );
        memcpy  ( linprebuf + MAX_ORDER - num_samples, left_samples,          num_samples             * sizeof(Float_t) );
        memcpy  ( rinprebuf + MAX_ORDER - num_samples, right_samples,         num_samples             * sizeof(Float_t) );
    }
    else {
        memcpy  ( linprebuf, left_samples  + num_samples - MAX_ORDER, MAX_ORDER * sizeof(Float_t) );
        memcpy  ( rinprebuf, right_samples + num_samples - MAX_ORDER, MAX_ORDER * sizeof(Float_t) );
    }

    return GAIN_ANALYSIS_OK;
}


static Float_t
analyzeResult ( Uint32_t* Array, size_t len )
{
    Uint32_t  elems;
    Int32_t   upper;
    size_t    i;

    elems = 0;
    for ( i = 0; i < len; i++ )
        elems += Array[i];
    if ( elems == 0 )
        return GAIN_NOT_ENOUGH_SAMPLES;

    upper = (Int32_t) ceil (elems * (1. - RMS_PERCENTILE));
    for ( i = len; i-- > 0; ) {
        if ( (upper -= Array[i]) <= 0 )
            break;
    }

    return (Float_t) ((Float_t)PINK_REF - (Float_t)i / (Float_t)STEPS_per_dB);
}


Float_t
GetTitleGain ( void )
{
    Float_t  retval;
    unsigned int    i;

    retval = analyzeResult ( A, sizeof(A)/sizeof(*A) );

    for ( i = 0; i < sizeof(A)/sizeof(*A); i++ ) {
        B[i] += A[i];
        A[i]  = 0;
    }

    for ( i = 0; i < MAX_ORDER; i++ )
        linprebuf[i] = lstepbuf[i] = loutbuf[i] = rinprebuf[i] = rstepbuf[i] = routbuf[i] = 0.f;

    totsamp = 0;
    lsum    = rsum = 0.;
    return retval;
}


Float_t
GetAlbumGain ( void )
{
    return analyzeResult ( B, sizeof(B)/sizeof(*B) );
}

/* end of replaygain_analysis.c */
