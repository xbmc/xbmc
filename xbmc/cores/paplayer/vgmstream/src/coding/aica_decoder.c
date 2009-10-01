#include "../util.h"
#include "coding.h"

/* fixed point (.8) amount to scale the current step size by */
/* part of the same series as used in MS ADPCM "ADPCMTable" */
static const unsigned int scale_step[16] =
{
    230, 230, 230, 230, 307, 409, 512, 614,
    230, 230, 230, 230, 307, 409, 512, 614
};

/* expand an unsigned four bit delta to a wider signed range */
static const int scale_delta[16] = 
{
      1,  3,  5,  7,  9, 11, 13, 15,
     -1, -3, -5, -7, -9,-11,-13,-15
};

/* Yamaha AICA ADPCM (as seen in Dreamcast) */

void decode_aica(VGMSTREAMCHANNEL * stream, sample * outbuf, int channelspacing, int32_t first_sample, int32_t samples_to_do) {
    int i;
    int32_t sample_count;
    int32_t hist1 = stream->adpcm_history1_16;
    unsigned long step_size = stream->adpcm_step_index;

    for (i=first_sample,sample_count=0; i<first_sample+samples_to_do; i++,sample_count+=channelspacing) {
        int sample_nibble =
                (
                 (unsigned)read_8bit(stream->offset+i/2,stream->streamfile) >>
                 (i&1?4:0)
                )&0xf;

        int32_t sample_delta = (int32_t)step_size * scale_delta[sample_nibble];
        int32_t new_sample;

        new_sample = hist1 + sample_delta/8;

        outbuf[sample_count] = clamp16(new_sample);

        hist1 = outbuf[sample_count];

        step_size = (step_size * scale_step[sample_nibble])/0x100;
        if (step_size < 0x7f) step_size = 0x7f;
        if (step_size > 0x6000) step_size = 0x6000;
    }

    stream->adpcm_history1_16 = hist1;
    stream->adpcm_step_index = step_size;
}
