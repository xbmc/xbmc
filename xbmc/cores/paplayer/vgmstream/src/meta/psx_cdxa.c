#include "meta.h"
#include "../layout/layout.h"
#include "../util.h"

/* Sony PSX CD-XA */
/* No looped file ! */

off_t init_xa_channel(int channel,STREAMFILE *streamFile);

uint8_t AUDIO_CODING_GET_STEREO(uint8_t value) {
	return (uint8_t)(value & 3);
}

uint8_t AUDIO_CODING_GET_FREQ(uint8_t value) {
	return (uint8_t)((value >> 2) & 3);
}

VGMSTREAM * init_vgmstream_cdxa(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    char filename[260];

	int channel_count;
	uint8_t bCoding;
	off_t start_offset;

    int i;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("xa",filename_extension(filename))) goto fail;

    /* check RIFF Header */
    if (!((read_32bitBE(0x00,streamFile) == 0x52494646) && 
	      (read_32bitBE(0x08,streamFile) == 0x43445841) && 
		  (read_32bitBE(0x0C,streamFile) == 0x666D7420)))
        goto fail;

	/* First init to have the correct info of the channel */
	start_offset=init_xa_channel(0,streamFile);

	/* No sound ? */
	if(start_offset==0)
		goto fail;

	bCoding = read_8bit(start_offset-5,streamFile);

	switch (AUDIO_CODING_GET_STEREO(bCoding)) {
		case 0: channel_count = 1; break;
		case 1: channel_count = 2; break;
		default: channel_count = 0; break;
	}

	/* build the VGMSTREAM */
    vgmstream = allocate_vgmstream(channel_count,0);
    if (!vgmstream) goto fail;

	/* fill in the vital statistics */
	vgmstream->channels = channel_count;
	vgmstream->xa_channel = 0;

	switch (AUDIO_CODING_GET_FREQ(bCoding)) {
		case 0: vgmstream->sample_rate = 37800; break;
		case 1: vgmstream->sample_rate = 18900; break;
		default: vgmstream->sample_rate = 0; break;
	}

	/* Check for Compression Scheme */
	vgmstream->coding_type = coding_XA;
    vgmstream->num_samples = (int32_t)((((get_streamfile_size(streamFile) - 0x3C)/2352)*0x1F80)/(2*channel_count));

    vgmstream->layout_type = layout_xa_blocked;
    vgmstream->meta_type = meta_PSX_XA;

	/* open the file for reading by each channel */
    {
        STREAMFILE *chstreamfile;
        chstreamfile = streamFile->open(streamFile,filename,2352);

        if (!chstreamfile) goto fail;

        for (i=0;i<channel_count;i++) {
            vgmstream->ch[i].streamfile = chstreamfile;
        }
    }
	
	xa_block_update(start_offset,vgmstream);

	return vgmstream;

    /* clean up anything we may have opened */
fail:
    if (vgmstream) close_vgmstream(vgmstream);
    return NULL;
}

off_t init_xa_channel(int channel,STREAMFILE* streamFile) {
	
	off_t block_offset=0x44;
	size_t filelength=get_streamfile_size(streamFile);

	int8_t currentChannel;
	int8_t subAudio;

begin:

	// 0 can't be a correct value
	if(block_offset>=(off_t)filelength)
		return 0;

	currentChannel=read_8bit(block_offset-7,streamFile);
	subAudio=read_8bit(block_offset-6,streamFile);
	if (!((currentChannel==channel) && (subAudio==0x64))) {
		block_offset+=2352;
		goto begin;
	}
	return block_offset;
}
