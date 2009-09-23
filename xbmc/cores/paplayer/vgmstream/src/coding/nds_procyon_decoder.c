#include "coding.h"
#include "../util.h"

/* ADPCM found in NDS games using Procyon Studio Digital Sound Elements */

static const int8_t proc_coef[5][2] =
{
    {0x00,0x00},
    {0x3C,0x00},
    {0x73,0xCC},
    {0x62,0xC9},
    {0x7A,0xC4},
};

void decode_nds_procyon(VGMSTREAMCHANNEL * stream, sample * outbuf, int channelspacing, int32_t first_sample, int32_t samples_to_do) {
    int i=first_sample;
    int32_t sample_count;

    int framesin = first_sample/30;

    uint8_t header = read_8bit(framesin*16+15+stream->offset,stream->streamfile) ^ 0x80;
    int scale = 12 - (header & 0xf);
    int coef_index = (header >> 4) & 0xf;
    int32_t hist1 = stream->adpcm_history1_32;
    int32_t hist2 = stream->adpcm_history2_32;
    int32_t coef1;
    int32_t coef2;

    if (coef_index > 4) coef_index = 0;
    coef1 = proc_coef[coef_index][0];
    coef2 = proc_coef[coef_index][1];
    first_sample = first_sample%30;

    for (i=first_sample,sample_count=0; i<first_sample+samples_to_do; i++,sample_count+=channelspacing) {
        int sample_byte = read_8bit(framesin*16+stream->offset+i/2,stream->streamfile) ^ 0x80;

        int32_t sample = 
                 (int32_t)
                 (i&1?
                    get_high_nibble_signed(sample_byte):
                    get_low_nibble_signed(sample_byte)
                   ) * 64 * 64;
        if (scale < 0)
        {
            sample <<= -scale;
        }
        else
            sample >>= scale;

        sample = (hist1 * coef1 + hist2 * coef2 + 32) / 64  + (sample * 64);

        hist2 = hist1;
        hist1 = sample;

        outbuf[sample_count] = clamp16((sample + 32) / 64) /  64 * 64;
    }

    stream->adpcm_history1_32 = hist1;
    stream->adpcm_history2_32 = hist2;
}
