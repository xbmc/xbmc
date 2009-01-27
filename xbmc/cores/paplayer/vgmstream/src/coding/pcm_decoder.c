#include "coding.h"
#include "../util.h"

void decode_pcm16LE(VGMSTREAMCHANNEL * stream, sample * outbuf, int channelspacing, int32_t first_sample, int32_t samples_to_do) {
    int i;
    int32_t sample_count;

    for (i=first_sample,sample_count=0; i<first_sample+samples_to_do; i++,sample_count+=channelspacing) {
        outbuf[sample_count]=read_16bitLE(stream->offset+i*2,stream->streamfile);
    }
}

void decode_pcm16BE(VGMSTREAMCHANNEL * stream, sample * outbuf, int channelspacing, int32_t first_sample, int32_t samples_to_do) {
    int i;
    int32_t sample_count;

    for (i=first_sample,sample_count=0; i<first_sample+samples_to_do; i++,sample_count+=channelspacing) {
        outbuf[sample_count]=read_16bitBE(stream->offset+i*2,stream->streamfile);
    }
}

void decode_pcm8(VGMSTREAMCHANNEL * stream, sample * outbuf, int channelspacing, int32_t first_sample, int32_t samples_to_do) {
    int i;
    int32_t sample_count;

    for (i=first_sample,sample_count=0; i<first_sample+samples_to_do; i++,sample_count+=channelspacing) {
        outbuf[sample_count]=read_8bit(stream->offset+i,stream->streamfile)*0x100;
    }
}

void decode_pcm8_int(VGMSTREAMCHANNEL * stream, sample * outbuf, int channelspacing, int32_t first_sample, int32_t samples_to_do) {
    int i;
    int32_t sample_count;

    for (i=first_sample,sample_count=0; i<first_sample+samples_to_do; i++,sample_count+=channelspacing) {
        outbuf[sample_count]=read_8bit(stream->offset+i*channelspacing,stream->streamfile)*0x100;
    }
}

void decode_pcm8_sb_int(VGMSTREAMCHANNEL * stream, sample * outbuf, int channelspacing, int32_t first_sample, int32_t samples_to_do) {
    int i;
    int32_t sample_count;

    for (i=first_sample,sample_count=0; i<first_sample+samples_to_do; i++,sample_count+=channelspacing) {
        int16_t v = (uint8_t)read_8bit(stream->offset+i*channelspacing,stream->streamfile);
        if (v&0x80) v = 0-(v&0x7f);
        outbuf[sample_count] = v*0x100;
    }
}

void decode_pcm8_unsigned_int(VGMSTREAMCHANNEL * stream, sample * outbuf, int channelspacing, int32_t first_sample, int32_t samples_to_do) {
    int i;
    int32_t sample_count;

    for (i=first_sample,sample_count=0; i<first_sample+samples_to_do; i++,sample_count+=channelspacing) {
        int16_t v = (uint8_t)read_8bit(stream->offset+i*channelspacing,stream->streamfile);
        outbuf[sample_count] = v*0x100 - 0x8000;
    }
}

void decode_pcm16LE_int(VGMSTREAMCHANNEL * stream, sample * outbuf, int channelspacing, int32_t first_sample, int32_t samples_to_do) {
    int i;
    int32_t sample_count;

    for (i=first_sample,sample_count=0; i<first_sample+samples_to_do; i++,sample_count+=channelspacing) {
        outbuf[sample_count]=read_16bitLE(stream->offset+i*2*channelspacing,stream->streamfile);
    }
}
