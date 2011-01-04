#include <math.h>
#include "coding.h"
#include "../util.h"

double VAG_f[5][2] = { { 0.0          ,   0.0        },
                       {  60.0 / 64.0 ,   0.0        },
		               { 115.0 / 64.0 , -52.0 / 64.0 },
		               {  98.0 / 64.0 , -55.0 / 64.0 } ,
		               { 122.0 / 64.0 , -60.0 / 64.0 } } ;
long VAG_coefs[5][2] = { {   0 ,   0 },
                         {  60 ,   0 },
                         { 115 , -52 },
                         {  98 , -55 } ,
                         { 122 , -60 } } ;

void decode_psx(VGMSTREAMCHANNEL * stream, sample * outbuf, int channelspacing, int32_t first_sample, int32_t samples_to_do) {

	int predict_nr, shift_factor, sample;
	int32_t hist1=stream->adpcm_history1_32;
	int32_t hist2=stream->adpcm_history2_32;

	short scale;
	int i;
	int32_t sample_count;
	uint8_t flag;

	int framesin = first_sample/28;

	predict_nr = read_8bit(stream->offset+framesin*16,stream->streamfile) >> 4;
	shift_factor = read_8bit(stream->offset+framesin*16,stream->streamfile) & 0xf;
	flag = read_8bit(stream->offset+framesin*16+1,stream->streamfile);

	first_sample = first_sample % 28;
	
	for (i=first_sample,sample_count=0; i<first_sample+samples_to_do; i++,sample_count+=channelspacing) {

		sample=0;

		if(flag<0x07) {
		
			short sample_byte = (short)read_8bit(stream->offset+(framesin*16)+2+i/2,stream->streamfile);

			scale = ((i&1 ?
				     sample_byte >> 4 :
					 sample_byte & 0x0f)<<12);

			sample=(int)((scale >> shift_factor)+hist1*VAG_f[predict_nr][0]+hist2*VAG_f[predict_nr][1]);
		}

		outbuf[sample_count] = clamp16(sample);
		hist2=hist1;
		hist1=sample;
	}
	stream->adpcm_history1_32=hist1;
	stream->adpcm_history2_32=hist2;
}

void decode_invert_psx(VGMSTREAMCHANNEL * stream, sample * outbuf, int channelspacing, int32_t first_sample, int32_t samples_to_do) {

	int predict_nr, shift_factor, sample;
	int32_t hist1=stream->adpcm_history1_32;
	int32_t hist2=stream->adpcm_history2_32;

	short scale;
	int i;
	int32_t sample_count;
	uint8_t flag;

	int framesin = first_sample/28;
    int head = read_8bit(stream->offset+framesin*16,stream->streamfile) ^ stream->bmdx_xor;

	predict_nr = ((head >> 4) & 0xf);
	shift_factor = (head & 0xf);
	flag = read_8bit(stream->offset+framesin*16+1,stream->streamfile);

	first_sample = first_sample % 28;
	
	for (i=first_sample,sample_count=0; i<first_sample+samples_to_do; i++,sample_count+=channelspacing) {

		sample=0;

		if(flag<0x07) {
		
			short sample_byte = (short)read_8bit(stream->offset+(framesin*16)+2+i/2,stream->streamfile);
            if (i/2 == 0)
                sample_byte = (short)(int8_t)(sample_byte+stream->bmdx_add);

			scale = ((i&1 ?
				     sample_byte >> 4 :
					 sample_byte & 0x0f)<<12);

			sample=(int)((scale >> shift_factor)+hist1*VAG_f[predict_nr][0]+hist2*VAG_f[predict_nr][1]);
		}

		outbuf[sample_count] = clamp16(sample);
		hist2=hist1;
		hist1=sample;
	}
	stream->adpcm_history1_32=hist1;
	stream->adpcm_history2_32=hist2;
}

/* some TAITO games have garbage (?) in their flags, this decoder
 * just ignores that byte */
void decode_psx_badflags(VGMSTREAMCHANNEL * stream, sample * outbuf, int channelspacing, int32_t first_sample, int32_t samples_to_do) {

	int predict_nr, shift_factor, sample;
	int32_t hist1=stream->adpcm_history1_32;
	int32_t hist2=stream->adpcm_history2_32;

	short scale;
	int i;
	int32_t sample_count;

	int framesin = first_sample/28;

	predict_nr = read_8bit(stream->offset+framesin*16,stream->streamfile) >> 4;
	shift_factor = read_8bit(stream->offset+framesin*16,stream->streamfile) & 0xf;
	first_sample = first_sample % 28;
	
	for (i=first_sample,sample_count=0; i<first_sample+samples_to_do; i++,sample_count+=channelspacing) {
        short sample_byte = (short)read_8bit(stream->offset+(framesin*16)+2+i/2,stream->streamfile);

        scale = ((i&1 ?
                    sample_byte >> 4 :
                    sample_byte & 0x0f)<<12);

        sample=(int)((scale >> shift_factor)+hist1*VAG_f[predict_nr][0]+hist2*VAG_f[predict_nr][1]);

		outbuf[sample_count] = clamp16(sample);
		hist2=hist1;
		hist1=sample;
	}
	stream->adpcm_history1_32=hist1;
	stream->adpcm_history2_32=hist2;
}

/* FF XI's Vag-ish format */
void decode_ffxi_adpcm(VGMSTREAMCHANNEL * stream, sample * outbuf, int channelspacing, int32_t first_sample, int32_t samples_to_do) {

	int predict_nr, shift_factor, sample;
	int32_t hist1=stream->adpcm_history1_32;
	int32_t hist2=stream->adpcm_history2_32;

	short scale;
	int i;
	int32_t sample_count;
    long predictor;

	int framesin = first_sample/16;

	predict_nr = read_8bit(stream->offset+framesin*9,stream->streamfile) >> 4;
	shift_factor = read_8bit(stream->offset+framesin*9,stream->streamfile) & 0xf;
	first_sample = first_sample % 16;
	
	for (i=first_sample,sample_count=0; i<first_sample+samples_to_do; i++,sample_count+=channelspacing) {
        short sample_byte = (short)read_8bit(stream->offset+(framesin*9)+1+i/2,stream->streamfile);

		sample=0;

        scale = ((i&1 ?
                    sample_byte >> 4 :
                    sample_byte & 0x0f)<<12);

#if 1
        predictor =
                (int)((hist1*VAG_f[predict_nr][0]+hist2*VAG_f[predict_nr][1]));
#else
        predictor = 
                (hist1*VAG_coefs[predict_nr][0]+hist2*VAG_coefs[predict_nr][1])/64;
#endif
        sample=(scale >> shift_factor) + predictor;

		outbuf[sample_count] = clamp16(sample);
		hist2=hist1;
		hist1=sample;
	}
	stream->adpcm_history1_32=hist1;
	stream->adpcm_history2_32=hist2;
}

