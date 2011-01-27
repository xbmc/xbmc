#include "coding.h"
#include "../util.h"

void decode_ngc_dtk(VGMSTREAMCHANNEL * stream, sample * outbuf, int channelspacing, int32_t first_sample, int32_t samples_to_do, int channel) {
    int i=first_sample;
    int32_t sample_count;

    int framesin = first_sample/28;

    uint8_t q = read_8bit(framesin*32+stream->offset+channel,stream->streamfile);
    int32_t hist1 = stream->adpcm_history1_32;
    int32_t hist2 = stream->adpcm_history2_32;

    first_sample = first_sample%28;

    for (i=first_sample,sample_count=0; i<first_sample+samples_to_do; i++,sample_count+=channelspacing) {
        int sample_byte = read_8bit(framesin*32+stream->offset+4+i,stream->streamfile);

        int32_t hist=0;

        switch (q>>4)
        {
            case 0:
                hist = 0;
                break;
            case 1:
                hist = (hist1 * 0x3c);
                break;
            case 2:
                hist = (hist1 * 0x73) - (hist2 * 0x34);
                break;
            case 3:
                hist = (hist1 * 0x62) - (hist2 * 0x37);
                break;
        }

        hist = (hist+0x20)>>6;
        if (hist >  0x1fffff) hist =  0x1fffff;
        if (hist < -0x200000) hist = -0x200000;

        hist2 = hist1;

        hist1 = ((((channel==0?
                    get_low_nibble_signed(sample_byte):
                    get_high_nibble_signed(sample_byte)
                   ) << 12) >> (q & 0xf)) << 6) + hist;

        outbuf[sample_count] = clamp16(hist1 >> 6);
    }

    stream->adpcm_history1_32 = hist1;
    stream->adpcm_history2_32 = hist2;
}
