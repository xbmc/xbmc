#include "../util.h"
#include "coding.h"

const int32_t ADPCMTable[89] = 

{

    7, 8, 9, 10, 11, 12, 13, 14,
    16, 17, 19, 21, 23, 25, 28, 31,
    34, 37, 41, 45, 50, 55, 60, 66,
    73, 80, 88, 97, 107, 118, 130, 143,
    157, 173, 190, 209, 230, 253, 279, 307,
    337, 371, 408, 449, 494, 544, 598, 658,
    724, 796, 876, 963, 1060, 1166, 1282, 1411,
    1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024,
    3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484,
    7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
    15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794,
    32767

};

const int IMA_IndexTable[16] = 

{
    -1, -1, -1, -1, 2, 4, 6, 8,
    -1, -1, -1, -1, 2, 4, 6, 8 
};

void decode_nds_ima(VGMSTREAMCHANNEL * stream, sample * outbuf, int channelspacing, int32_t first_sample, int32_t samples_to_do) {
    int i=first_sample;
    int32_t sample_count;
    int32_t hist1 = stream->adpcm_history1_16;
    int step_index = stream->adpcm_step_index;

    if (first_sample==0) {
        hist1 = read_16bitLE(stream->offset,stream->streamfile);
        step_index = read_16bitLE(stream->offset+2,stream->streamfile);
    }

    for (i=first_sample,sample_count=0; i<first_sample+samples_to_do; i++,sample_count+=channelspacing) {
        int sample_nibble = 
                (read_8bit(stream->offset+4+i/2,stream->streamfile) >> (i&1?4:0))&0xf;
        int delta;
        int step = ADPCMTable[step_index];

        delta = step >> 3;
        if (sample_nibble & 1) delta += step >> 2;
        if (sample_nibble & 2) delta += step >> 1;
        if (sample_nibble & 4) delta += step;
        if (sample_nibble & 8)
            outbuf[sample_count] = clamp16(hist1 - delta);
        else
            outbuf[sample_count] = clamp16(hist1 + delta);

        step_index += IMA_IndexTable[sample_nibble];
        if (step_index < 0) step_index=0;
        if (step_index > 88) step_index=88;

        hist1 = outbuf[sample_count];
    }

    stream->adpcm_history1_16 = hist1;
    stream->adpcm_step_index = step_index;
}

void decode_xbox_ima(VGMSTREAM * vgmstream,VGMSTREAMCHANNEL * stream, sample * outbuf, int channelspacing, int32_t first_sample, int32_t samples_to_do,int channel) {
    int i=first_sample;
	int sample_nibble;
	int sample_decoded;
    int delta;

    int32_t sample_count=0;
    int32_t hist1=stream->adpcm_history1_32;
    int step_index = stream->adpcm_step_index;
	off_t offset=stream->offset;

	if(vgmstream->channels==1) 
		first_sample = first_sample % 32;
	else
		first_sample = first_sample % (32*(vgmstream->channels&2));

    if (first_sample == 0) {

		if(vgmstream->layout_type==layout_ea_blocked) {
			hist1 = read_16bitLE(offset,stream->streamfile);
			step_index = read_16bitLE(offset+2,stream->streamfile);
		} else {
			hist1 = read_16bitLE(offset+(channel%2)*4,stream->streamfile);
			step_index = read_16bitLE(offset+(channel%2)*4+2,stream->streamfile);
		}
        if (step_index < 0) step_index=0;
        if (step_index > 88) step_index=88;
    }

    for (i=first_sample,sample_count=0; i<first_sample+samples_to_do; i++,sample_count+=channelspacing) {
        int step = ADPCMTable[step_index];

		if(vgmstream->layout_type==layout_ea_blocked) 
			offset = stream->offset + (i/8*4+(i%8)/2+4);
		else {
			if(channelspacing==1)
				offset = stream->offset + 4 + (i/8*4+(i%8)/2+4*(channel%2));
			else
				offset = stream->offset + 4*2 + (i/8*4*2+(i%8)/2+4*(channel%2));
		}

        sample_nibble = (read_8bit(offset,stream->streamfile) >> (i&1?4:0))&0xf;

		sample_decoded=hist1;

        delta = step >> 3;
        if (sample_nibble & 1) delta += step >> 2;
        if (sample_nibble & 2) delta += step >> 1;
        if (sample_nibble & 4) delta += step;
        if (sample_nibble & 8)
            sample_decoded -= delta;
        else
            sample_decoded += delta;

		hist1=clamp16(sample_decoded);

        step_index += IMA_IndexTable[sample_nibble];
        if (step_index < 0) step_index=0;
        if (step_index > 88) step_index=88;
		
        outbuf[sample_count]=(short)(hist1);
    }

	// Only increment offset on complete frame
	if(vgmstream->layout_type==layout_ea_blocked) {
		if(offset-stream->offset==32+3) // ??
			stream->offset+=36;
	} else {
		if(channelspacing==1) {
			if(offset-stream->offset==32+3) // ??
				stream->offset+=36;
		} else {
			if(offset-stream->offset==64+(4*(channel%2))+3) // ??
				stream->offset+=36*channelspacing;
		}
	}
	stream->adpcm_history1_32=hist1;
	stream->adpcm_step_index=step_index;
}

void decode_dvi_ima(VGMSTREAMCHANNEL * stream, sample * outbuf, int channelspacing, int32_t first_sample, int32_t samples_to_do) {
    int i;

    int32_t sample_count=0;
    int32_t hist1=stream->adpcm_history1_32;
    int step_index = stream->adpcm_step_index;

    for (i=first_sample,sample_count=0; i<first_sample+samples_to_do; i++,sample_count+=channelspacing) {
        int step = ADPCMTable[step_index];
        uint8_t sample_byte;
        int sample_nibble;
        int sample_decoded;
        int delta;

        sample_byte = read_8bit(stream->offset+i/2,stream->streamfile);
        /* old-style DVI takes high nibble first */
        sample_nibble = (sample_byte >> (i&1?0:4))&0xf;

        sample_decoded = hist1;
        delta = step >> 3;
        if (sample_nibble & 1) delta += step >> 2;
        if (sample_nibble & 2) delta += step >> 1;
        if (sample_nibble & 4) delta += step;
        if (sample_nibble & 8)
            sample_decoded -= delta;
        else
            sample_decoded += delta;

        hist1=clamp16(sample_decoded);

        step_index += IMA_IndexTable[sample_nibble&0x7];
        if (step_index < 0) step_index=0;
        if (step_index > 88) step_index=88;

        outbuf[sample_count]=(short)(hist1);
    }

    stream->adpcm_history1_32=hist1;
    stream->adpcm_step_index=step_index;
}


void decode_eacs_ima(VGMSTREAM * vgmstream, sample * outbuf, int channelspacing, int32_t first_sample, int32_t samples_to_do, int channel) {
    int i;
    VGMSTREAMCHANNEL * stream = &(vgmstream->ch[channel]);

    int32_t sample_count=0;
    int32_t hist1=stream->adpcm_history1_32;
    int step_index = stream->adpcm_step_index;

	vgmstream->get_high_nibble=!vgmstream->get_high_nibble;

	if((first_sample) && (channelspacing==1))
		vgmstream->get_high_nibble=!vgmstream->get_high_nibble;

    for (i=first_sample,sample_count=0; i<first_sample+samples_to_do; i++,sample_count+=channelspacing) {
        int step = ADPCMTable[step_index];
        uint8_t sample_byte;
        int sample_nibble;
        int sample_decoded;
        int delta;

        sample_byte = read_8bit(stream->offset+i,stream->streamfile);
        sample_nibble = (sample_byte >> (vgmstream->get_high_nibble?0:4))&0xf;

        sample_decoded = hist1;
        delta = step >> 3;
        if (sample_nibble & 1) delta += step >> 2;
        if (sample_nibble & 2) delta += step >> 1;
        if (sample_nibble & 4) delta += step;
        if (sample_nibble & 8)
            sample_decoded -= delta;
        else
            sample_decoded += delta;

        hist1=clamp16(sample_decoded);

        step_index += IMA_IndexTable[sample_nibble&0x7];
        if (step_index < 0) step_index=0;
        if (step_index > 88) step_index=88;

        outbuf[sample_count]=(short)(hist1);
    }

    stream->adpcm_history1_32=hist1;
    stream->adpcm_step_index=step_index;
}

void decode_ima(VGMSTREAMCHANNEL * stream, sample * outbuf, int channelspacing, int32_t first_sample, int32_t samples_to_do) {
    int i;

    int32_t sample_count=0;
    int32_t hist1=stream->adpcm_history1_32;
    int step_index = stream->adpcm_step_index;

    for (i=first_sample,sample_count=0; i<first_sample+samples_to_do; i++,sample_count+=channelspacing) {
        int step = ADPCMTable[step_index];
        uint8_t sample_byte;
        int sample_nibble;
        int sample_decoded;
        int delta;

        sample_byte = read_8bit(stream->offset+i/2,stream->streamfile);
        sample_nibble = (sample_byte >> (i&1?4:0))&0xf;

        sample_decoded = hist1;
        delta = step >> 3;
        if (sample_nibble & 1) delta += step >> 2;
        if (sample_nibble & 2) delta += step >> 1;
        if (sample_nibble & 4) delta += step;
        if (sample_nibble & 8)
            sample_decoded -= delta;
        else
            sample_decoded += delta;

        hist1=clamp16(sample_decoded);

        step_index += IMA_IndexTable[sample_nibble&0x7];
        if (step_index < 0) step_index=0;
        if (step_index > 88) step_index=88;

        outbuf[sample_count]=(short)(hist1);
    }

    stream->adpcm_history1_32=hist1;
    stream->adpcm_step_index=step_index;
}
