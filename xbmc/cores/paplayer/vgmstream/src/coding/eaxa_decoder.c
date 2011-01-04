#include "coding.h"
#include "../util.h"

long EA_XA_TABLE[28] = {0,0,240,0,460,-208,0x0188,-220,
					      0x0000,0x0000,0x00F0,0x0000,
					      0x01CC,0x0000,0x0188,0x0000,
					      0x0000,0x0000,0x0000,0x0000,
					                  -208,-1,-220,-1,
					      0x0000,0x0000,0x0000,0x3F70};

long EA_TABLE[20]= { 0x00000000, 0x000000F0, 0x000001CC, 0x00000188,
			   	    0x00000000, 0x00000000, 0xFFFFFF30, 0xFFFFFF24,
				    0x00000000, 0x00000001, 0x00000003, 0x00000004,
				    0x00000007, 0x00000008, 0x0000000A, 0x0000000B,
				    0x00000000, 0xFFFFFFFF, 0xFFFFFFFD, 0xFFFFFFFC};

void decode_eaxa(VGMSTREAMCHANNEL * stream, sample * outbuf, int channelspacing, int32_t first_sample, int32_t samples_to_do,int channel) {
    uint8_t frame_info;
    int32_t sample_count;
	long coef1,coef2;
	int i,shift;
	off_t channel_offset=stream->channel_start_offset;
	
	first_sample = first_sample%28;
	frame_info = (uint8_t)read_8bit(stream->offset+channel_offset,stream->streamfile);

	if(frame_info==0xEE) {

		channel_offset++;
		stream->adpcm_history1_32 = read_16bitBE(stream->offset+channel_offset,stream->streamfile);
		stream->adpcm_history2_32 = read_16bitBE(stream->offset+channel_offset+2,stream->streamfile);

		channel_offset+=4;
		
		for (i=first_sample,sample_count=0; i<first_sample+samples_to_do; i++,sample_count+=channelspacing) {
			outbuf[sample_count]=read_16bitBE(stream->offset+channel_offset,stream->streamfile);
			channel_offset+=2;
		}

		// Only increment offset on complete frame
		if(channel_offset-stream->channel_start_offset==(2*28)+5)
			stream->channel_start_offset+=(2*28)+5;

	} else {
	
		
		coef1 = EA_XA_TABLE[((frame_info >> 4) & 0x0F) << 1];
		coef2 = EA_XA_TABLE[(((frame_info >> 4) & 0x0F) << 1) + 1];
		shift = (frame_info & 0x0F) + 8;
		
		channel_offset++;

		for (i=first_sample,sample_count=0; i<first_sample+samples_to_do; i++,sample_count+=channelspacing) {
			uint8_t sample_byte = (uint8_t)read_8bit(stream->offset+channel_offset+i/2,stream->streamfile);
			int32_t sample = ((((i&1?
						    sample_byte & 0x0F:
							sample_byte >> 4
						  ) << 0x1C) >> shift) +
						  (coef1 * stream->adpcm_history1_32) + (coef2 * stream->adpcm_history2_32)) >> 8;
			
			outbuf[sample_count] = clamp16(sample);
			stream->adpcm_history2_32 = stream->adpcm_history1_32;
			stream->adpcm_history1_32 = sample;
		}
		
		channel_offset+=i/2;

		// Only increment offset on complete frame
		if(channel_offset-stream->channel_start_offset==0x0F)
			stream->channel_start_offset+=0x0F;
	}
}


void decode_ea_adpcm(VGMSTREAM * vgmstream, sample * outbuf, int channelspacing, int32_t first_sample, int32_t samples_to_do,int channel) {
    uint8_t frame_info;
    int32_t sample_count;
	long coef1,coef2;
	int i,shift;
    VGMSTREAMCHANNEL *stream = &(vgmstream->ch[channel]);
	off_t channel_offset=stream->channel_start_offset;

	vgmstream->get_high_nibble=!vgmstream->get_high_nibble;

	first_sample = first_sample%28;
	frame_info = read_8bit(stream->offset+channel_offset,stream->streamfile);

	coef1 = EA_TABLE[(vgmstream->get_high_nibble? frame_info & 0x0F: frame_info >> 4)];
	coef2 = EA_TABLE[(vgmstream->get_high_nibble? frame_info & 0x0F: frame_info >> 4) + 4];

	channel_offset++;

	frame_info = read_8bit(stream->offset+channel_offset,stream->streamfile);
	shift = (vgmstream->get_high_nibble? frame_info & 0x0F : frame_info >> 4)+8;

	channel_offset++;

	for (i=first_sample,sample_count=0; i<first_sample+samples_to_do; i++,sample_count+=channelspacing) {
		uint8_t sample_byte = (uint8_t)read_8bit(stream->offset+channel_offset+i,stream->streamfile);
		int32_t sample = ((((vgmstream->get_high_nibble?
					    sample_byte & 0x0F:
						sample_byte >> 4
					  ) << 0x1C) >> shift) +
					  (coef1 * stream->adpcm_history1_32) + (coef2 * stream->adpcm_history2_32) + 0x80) >> 8;
		
		outbuf[sample_count] = clamp16(sample);
		stream->adpcm_history2_32 = stream->adpcm_history1_32;
		stream->adpcm_history1_32 = sample;
	}
		
	channel_offset+=i;

	// Only increment offset on complete frame
	if(channel_offset-stream->channel_start_offset==0x1E)
		stream->channel_start_offset+=0x1E;
}

