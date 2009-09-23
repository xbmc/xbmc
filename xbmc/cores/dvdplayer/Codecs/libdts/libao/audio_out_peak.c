#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <inttypes.h>

#include "dts.h"
#include "audio_out.h"
#include "audio_out_internal.h"

typedef struct peak_instance_s {
    ao_instance_t ao;
    int flags;
    float peak;
} peak_instance_t;

static int peak_setup (ao_instance_t * _instance, int sample_rate, int * flags,
		       level_t * level, sample_t * bias)
{
    peak_instance_t * instance = (peak_instance_t *) _instance;

    *flags = instance->flags;
    *level = CONVERT_LEVEL;
    *bias = 0;

    return 0;
}

static int peak_play (ao_instance_t * _instance, int flags, sample_t * samples)
{
    peak_instance_t * instance = (peak_instance_t *) _instance;
    int i;

    for (i = 0; i < 256 * 2; i++) {
#ifdef LIBDTS_FIXED
	float f = fabs (samples[i] * (1.0 / (1 << 30)));
#else
	float f = fabs (samples[i]);
#endif
	if (instance->peak < f)
	    instance->peak = f;
    }

    return 0;
}

static void peak_close (ao_instance_t * _instance)
{
    peak_instance_t * instance = (peak_instance_t *) _instance;

    printf ("peak level = %.4f (%+.2f dB)\n",
	    instance->peak, 6 * log (instance->peak) / log (2));
}

static ao_instance_t * peak_open (int flags)
{
    peak_instance_t * instance;

    instance = (peak_instance_t *) malloc (sizeof (peak_instance_t));
    if (instance == NULL)
	return NULL;

    instance->ao.setup = peak_setup;
    instance->ao.play = peak_play;
    instance->ao.close = peak_close;
    instance->flags = flags;
    instance->peak = 0;

    return (ao_instance_t *) instance;
}

ao_instance_t * ao_peak_open (void)
{
    return peak_open (DTS_STEREO);
}

ao_instance_t * ao_peakdolby_open (void)
{
    return peak_open (DTS_DOLBY);
}
