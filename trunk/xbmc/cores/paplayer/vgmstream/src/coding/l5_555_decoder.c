#include "coding.h"
#include "../util.h"

static const int32_t l5_scales[32] = {
    0x00001000, 0x0000144E, 0x000019C5, 0x000020B4, 0x00002981, 0x000034AC, 0x000042D9, 0x000054D6,
    0x00006BAB, 0x000088A4, 0x0000AD69, 0x0000DC13, 0x0001174C, 0x00016275, 0x0001C1D8, 0x00023AE5,
    0x0002D486, 0x0003977E, 0x00048EEE, 0x0005C8F3, 0x00075779, 0x0009513E, 0x000BD31C, 0x000F01B5,
    0x00130B82, 0x00182B83, 0x001EAC92, 0x0026EDB2, 0x00316777, 0x003EB2E6, 0x004F9232, 0x0064FBD1
};

void decode_l5_555(VGMSTREAMCHANNEL * stream, sample * outbuf, int channelspacing, int32_t first_sample, int32_t samples_to_do) {
    int i=first_sample;
    int32_t sample_count;

    int framesin = first_sample/32;

    uint16_t header = (uint16_t)read_16bitLE(framesin*0x12+stream->offset,stream->streamfile);
    int32_t pos_scale = l5_scales[(header>>5)&0x1f];
    int32_t neg_scale = l5_scales[header&0x1f];

    int coef_index = (header >> 10) & 0x1f;
    int16_t hist1 = stream->adpcm_history1_16;
    int16_t hist2 = stream->adpcm_history2_16;
    int16_t hist3 = stream->adpcm_history3_16;
    int32_t coef1 = stream->adpcm_coef_3by32[coef_index*3];
    int32_t coef2 = stream->adpcm_coef_3by32[coef_index*3+1];
    int32_t coef3 = stream->adpcm_coef_3by32[coef_index*3+2];
    /*printf("offset: %x\nscale: %d\nindex: %d (%lf,%lf)\nhist: %d %d\n",
            (unsigned)stream->offset,scale,coef_index,coef1/2048.0,coef2/2048.0,hist1,hist2);*/

    first_sample = first_sample%32;

    for (i=first_sample,sample_count=0; i<first_sample+samples_to_do; i++,sample_count+=channelspacing) {
        int sample_byte = read_8bit(framesin*0x12+stream->offset+2+i/2,stream->streamfile);
        int nibble = (i&1?
                get_low_nibble_signed(sample_byte):
                get_high_nibble_signed(sample_byte));
        int32_t prediction =
            -(hist1 * coef1 + hist2 * coef2 + hist3 * coef3);

        if (nibble >= 0)
        {
            outbuf[sample_count] = clamp16((prediction + nibble * pos_scale) >> 12);
        }
        else
        {
            outbuf[sample_count] = clamp16((prediction + nibble * neg_scale) >> 12);
        }

        hist3 = hist2;
        hist2 = hist1;
        hist1 = outbuf[sample_count];
    }

    stream->adpcm_history1_16 = hist1;
    stream->adpcm_history2_16 = hist2;
    stream->adpcm_history3_16 = hist3;
}
