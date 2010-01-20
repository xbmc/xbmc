#include <math.h>
#include "coding.h"
#include "../util.h"

/* Westwood Studios ADPCM */

/* Based on Valery V. Anisimovsky's WS-AUD.txt */

static char WSTable2bit[4]={-2,-1,0,1};
static char WSTable4bit[16]={-9,-8,-6,-5,-4,-3,-2,-1,
                              0, 1, 2, 3, 4, 5 ,6, 8};

/* We pass in the VGMSTREAM here, unlike in other codings, because
   the decoder has to know about the block structure. */
void decode_ws(VGMSTREAM * vgmstream, int channel, sample * outbuf, int channelspacing, int32_t first_sample,
        int32_t samples_to_do) {

    VGMSTREAMCHANNEL * stream = &(vgmstream->ch[channel]);
	int16_t hist = stream->adpcm_history1_16;
    off_t offset = stream->offset;
    int samples_left_in_frame = stream->samples_left_in_frame;
    off_t header_off = stream->frame_header_offset;

	int i;
	int32_t sample_count;
	
    if (vgmstream->ws_output_size == vgmstream->current_block_size) {
        /* uncompressed, we just need to convert to 16-bit */
        for (i=first_sample,sample_count=0; i<first_sample+samples_to_do; i++,sample_count+=channelspacing,offset++) {
            outbuf[sample_count]=((uint8_t)read_8bit(offset,stream->streamfile)-0x80)*0x100;
        }
    } else {
        if (first_sample == 0) {
            hist = 0x80;
            samples_left_in_frame = 0;
        }
        /* actually decompress */
        for (i=first_sample,sample_count=0; i<first_sample+samples_to_do; ) {
            uint8_t header;
            uint8_t count;

            if (samples_left_in_frame == 0) {
                header_off = offset;
                offset++;
            }

            header = read_8bit(header_off,stream->streamfile);
            count = header & 0x3f;
            switch (header>>6) {    /* code */
                case 0: /* 2-bit ADPCM */
                    if (samples_left_in_frame == 0) {
                        samples_left_in_frame = (count + 1)*4;
                    }
                    for (;samples_left_in_frame>0 &&        /* read this frame */
                            i<first_sample+samples_to_do;   /* up to samples_to_do */
                            i++,sample_count+=channelspacing, /* done with writing a sample */
                            samples_left_in_frame--) { /* done with reading a sample */
                        int twobit = ((count + 1)*4-samples_left_in_frame)%4;
                        uint8_t sample;
                        sample = read_8bit(offset,stream->streamfile);
                        sample = (sample >> (twobit*2)) & 0x3;
                        hist += WSTable2bit[sample];
                        if (hist < 0) hist = 0;
                        if (hist > 0xff) hist = 0xff;
                        outbuf[sample_count]=(hist-0x80)*0x100;

                        if (twobit == 3)
                            offset++;  /* done with that byte */
                    }
                    break;
                case 1: /* 4-bit ADPCM */
                    if (samples_left_in_frame == 0) {
                        samples_left_in_frame = (count + 1)*2;
                    }
                    for (;samples_left_in_frame>0 &&        /* read this frame */
                            i<first_sample+samples_to_do;   /* up to samples_to_do */
                            i++,sample_count+=channelspacing, /* done with writing a sample */
                            samples_left_in_frame--) { /* done with reading a sample */
                        int nibble = ((count + 1)*4-samples_left_in_frame)%2;
                        uint8_t sample;
                        sample = read_8bit(offset,stream->streamfile);
                        if (nibble == 0)
                            sample &= 0xf;
                        else
                            sample >>= 4;
                        hist += WSTable4bit[sample];
                        if (hist < 0) hist = 0;
                        if (hist > 0xff) hist = 0xff;
                        outbuf[sample_count]=(hist-0x80)*0x100;

                        if (nibble == 1)
                            offset++;  /* done with that byte */
                    }
                    break;
                case 2: /* no compression */
                    if (count & 0x20) { /* delta */
                        /* Note no checks against samples_to_do here,
                           at the top of the for loop we can always do at
                           least one sample */
                        /* low 5 bits are a signed delta */
                        if (count & 0x10) {
                            hist -= ((count & 0xf)^0xf) + 1;
                        } else {
                            hist += count & 0xf;
                        }

                        /* Valery doesn't specify this, but I will assume */
                        if (hist < 0) hist = 0;
                        if (hist > 0xff) hist = 0xff;

                        outbuf[sample_count]=(hist-0x80)*0x100;
                        sample_count+=channelspacing;
                        i++;

                        /* just one, and we got it */
                        samples_left_in_frame = 0;
                    } else {    /* copy bytes verbatim */
                        if (samples_left_in_frame == 0)
                            samples_left_in_frame=count+1;
                        for (;samples_left_in_frame>0 &&        /* read this frame */
                                i<first_sample+samples_to_do;   /* up to samples_to_do */
                                offset++,                       /* done with a byte */
                                i++,sample_count+=channelspacing,   /* done with writing a sample */
                                samples_left_in_frame--) {      /* done with reading a sample */
                            outbuf[sample_count]=((hist=(uint8_t)read_8bit(offset,stream->streamfile))-0x80)*0x100;
                        }
                    }
                    break;
                case 3: /* RLE */
                    if (samples_left_in_frame == 0)
                        samples_left_in_frame=count+1;
                    for (;samples_left_in_frame>0 &&            /* read this frame */
                            i<first_sample+samples_to_do;       /* up to samples_to_do */
                            i++,sample_count+=channelspacing,   /* done with writing a sample */
                            samples_left_in_frame--) {          /* done with reading a sample */
                        outbuf[sample_count]=(hist-0x80)*0x100;
                    }
            }
        }
    }

    stream->offset = offset;
    stream->adpcm_history1_16 = hist;
    stream->samples_left_in_frame = samples_left_in_frame;
    stream->frame_header_offset = header_off;
}
