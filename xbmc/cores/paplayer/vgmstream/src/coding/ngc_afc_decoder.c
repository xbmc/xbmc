#include "coding.h"
#include "../util.h"

const short afc_coef[16][2] =
{{0,0},
{0x0800,0},
{0,0x0800},
{0x0400,0x0400},
{0x1000,0xf800},
{0x0e00,0xfa00},
{0x0c00,0xfc00},
{0x1200,0xf600},
{0x1068,0xf738},
{0x12c0,0xf704},
{0x1400,0xf400},
{0x0800,0xf800},
{0x0400,0xfc00},
{0xfc00,0x0400},
{0xfc00,0},
{0xf800,0}};

void decode_ngc_afc(VGMSTREAMCHANNEL * stream, sample * outbuf, int channelspacing, int32_t first_sample, int32_t samples_to_do) {
    int i=first_sample;
    int32_t sample_count;

    int framesin = first_sample/16;

    int8_t header = read_8bit(framesin*9+stream->offset,stream->streamfile);
    int32_t scale = 1 << ((header>>4) & 0xf);
    int coef_index = (header & 0xf);
    int32_t hist1 = stream->adpcm_history1_16;
    int32_t hist2 = stream->adpcm_history2_16;
    int coef1 = afc_coef[coef_index][0];
    int coef2 = afc_coef[coef_index][1];
    /*printf("offset: %x\nscale: %d\nindex: %d (%lf,%lf)\nhist: %d %d\n",
            (unsigned)stream->offset,scale,coef_index,coef1/2048.0,coef2/2048.0,hist1,hist2);*/

    first_sample = first_sample%16;

    for (i=first_sample,sample_count=0; i<first_sample+samples_to_do; i++,sample_count+=channelspacing) {
        int sample_byte = read_8bit(framesin*9+stream->offset+1+i/2,stream->streamfile);

        outbuf[sample_count] = clamp16((
                 (((i&1?
                    get_low_nibble_signed(sample_byte):
                    get_high_nibble_signed(sample_byte)
                   ) * scale)<<11) +
                 (coef1 * hist1 + coef2 * hist2))>>11
                );
        /*printf("%hd\n",outbuf[sample_count]);*/

        hist2 = hist1;
        hist1 = outbuf[sample_count];
    }

    stream->adpcm_history1_16 = hist1;
    stream->adpcm_history2_16 = hist2;
}
