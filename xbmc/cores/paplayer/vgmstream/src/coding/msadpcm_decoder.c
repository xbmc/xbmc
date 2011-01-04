#include "../util.h"
#include "coding.h"

/* used to compute next scale */
static const int ADPCMTable[16] = 
{
    230, 230, 230, 230,
    307, 409, 512, 614,
    768, 614, 512, 409,
    307, 230, 230, 230
};

static const int ADPCMCoeffs[7][2] = 
{
    { 256,    0 },
    { 512, -256 },
    {   0,    0 },
    { 192,   64 },
    { 240,    0 },
    { 460, -208 },
    { 392, -232 }
};

void decode_msadpcm_stereo(VGMSTREAM * vgmstream, sample * outbuf, int32_t first_sample, int32_t samples_to_do) {
    VGMSTREAMCHANNEL *ch1,*ch2;
    int i;
    int framesin;
    STREAMFILE *streamfile;
    off_t offset;

    framesin = first_sample/get_vgmstream_samples_per_frame(vgmstream);
    first_sample = first_sample%get_vgmstream_samples_per_frame(vgmstream);

    ch1 = &vgmstream->ch[0];
    ch2 = &vgmstream->ch[1];
    streamfile = ch1->streamfile;
    offset = ch1->offset+framesin*get_vgmstream_frame_size(vgmstream);

    if (first_sample==0) {
        ch1->adpcm_coef[0] = ADPCMCoeffs[read_8bit(offset,streamfile)][0];
        ch1->adpcm_coef[1] = ADPCMCoeffs[read_8bit(offset,streamfile)][1];
        ch2->adpcm_coef[0] = ADPCMCoeffs[read_8bit(offset+1,streamfile)][0];
        ch2->adpcm_coef[1] = ADPCMCoeffs[read_8bit(offset+1,streamfile)][1];
        ch1->adpcm_scale = read_16bitLE(offset+2,streamfile);
        ch2->adpcm_scale = read_16bitLE(offset+4,streamfile);
        ch1->adpcm_history1_16 = read_16bitLE(offset+6,streamfile);
        ch2->adpcm_history1_16 = read_16bitLE(offset+8,streamfile);
        ch1->adpcm_history2_16 = read_16bitLE(offset+10,streamfile);
        ch2->adpcm_history2_16 = read_16bitLE(offset+12,streamfile);

        outbuf[0] = ch1->adpcm_history2_16;
        outbuf[1] = ch2->adpcm_history2_16;

        outbuf+=2;
        first_sample++;
        samples_to_do--;
    }
    if (first_sample==1 && samples_to_do > 0) {
        outbuf[0] = ch1->adpcm_history1_16;
        outbuf[1] = ch2->adpcm_history1_16;

        outbuf+=2;
        first_sample++;
        samples_to_do--;
    }

    for (i=first_sample; i<first_sample+samples_to_do; i++) {
        int j;

        for (j=0;j<2;j++)
        {
            VGMSTREAMCHANNEL *ch = &vgmstream->ch[j];
            int sample_nibble =
                (j == 0 ?
                 get_high_nibble_signed(read_8bit(offset+14+i-2,streamfile)) :
                 get_low_nibble_signed(read_8bit(offset+14+i-2,streamfile))
                );
            int32_t hist1,hist2;
            int32_t predicted;

            hist1 = ch->adpcm_history1_16;
            hist2 = ch->adpcm_history2_16;
            predicted = hist1 * ch->adpcm_coef[0] + hist2 * ch->adpcm_coef[1];
            predicted /= 256;
            predicted += sample_nibble*ch->adpcm_scale;
            outbuf[0] = clamp16(predicted);
            ch->adpcm_history2_16 = ch->adpcm_history1_16;
            ch->adpcm_history1_16 = outbuf[0];
            ch->adpcm_scale = (ADPCMTable[sample_nibble&0xf] *
                    ch->adpcm_scale) / 256;
            if (ch->adpcm_scale < 0x10) ch->adpcm_scale = 0x10;

            outbuf++;
        }
    }
}
