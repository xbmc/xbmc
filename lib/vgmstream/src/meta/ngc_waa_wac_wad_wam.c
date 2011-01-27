#include "meta.h"
#include "../util.h"

/* WAC - WAD - WAM (Beyond Good & Evil GC) */
VGMSTREAM * init_vgmstream_waa_wac_wad_wam(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    char filename[260];
    off_t start_offset;
    int loop_flag;
	int channel_count;
	int coef1_start;
	int coef2_start;
	int second_channel_start;

    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("waa",filename_extension(filename)) && 
		strcasecmp("wac",filename_extension(filename)) && 
		strcasecmp("wad",filename_extension(filename)) && 
		strcasecmp("wam",filename_extension(filename))) goto fail;

    /* check header */
    if (read_32bitBE(0x00,streamFile) != 0x52494646 ||	/* "RIFF" */
		read_32bitBE(0x08,streamFile) != 0x57415645 ||	/* "WAVE" */
		read_32bitBE(0x0C,streamFile) != 0x666D7420 ||	/* "fmt\0x20" */
		read_32bitBE(0x10,streamFile) != 0x12000000 ||	/* "0x12000000" */
		read_16bitBE(0x14,streamFile) != (int16_t)0xFEFF)		/* "FEFF" */
	goto fail;

    loop_flag = 1;
    channel_count = (uint16_t)read_16bitLE(0x16,streamFile);
    
	/* build the VGMSTREAM */
    vgmstream = allocate_vgmstream(channel_count,loop_flag);
    if (!vgmstream) goto fail;

	/* fill in the vital statistics */
    start_offset = 0x5C;
	vgmstream->channels = channel_count;
    vgmstream->sample_rate = read_32bitLE(0x18,streamFile);
    vgmstream->coding_type = coding_NGC_DSP;
    vgmstream->num_samples = (read_32bitLE(0x2A,streamFile))*14/8/channel_count;
    if (loop_flag) {
        vgmstream->loop_start_sample = 0;
        vgmstream->loop_end_sample = (read_32bitLE(0x2A,streamFile))*14/8/channel_count;
    }
		vgmstream->layout_type = layout_none;
		vgmstream->meta_type = meta_WAA_WAC_WAD_WAM;

		/* Retrieveing the coef tables and the start of the second channel*/
		coef1_start = 0x2E;
		coef2_start = (read_32bitLE(0x2A,streamFile)/2)+0x5C;
		second_channel_start = coef2_start+0x2E;

		{
        int i;
        for (i=0;i<16;i++)
            vgmstream->ch[0].adpcm_coef[i] = read_16bitBE(coef1_start+i*2,streamFile);
		if (channel_count == 2) {
		for (i=0;i<16;i++)
            vgmstream->ch[1].adpcm_coef[i] = read_16bitBE(coef2_start+i*2,streamFile);
    }
		}


    /* open the file for reading */
    {
        int i;
        STREAMFILE * file;
        file = streamFile->open(streamFile,filename,STREAMFILE_DEFAULT_BUFFER_SIZE);
        if (!file) goto fail;
        for (i=0;i<channel_count;i++) {
            vgmstream->ch[i].streamfile = file;

	/* The first channel */
    vgmstream->ch[0].channel_start_offset=
        vgmstream->ch[0].offset=start_offset;

	/* The second channel */
    if (channel_count == 2) {
        vgmstream->ch[1].streamfile = streamFile->open(streamFile,filename,STREAMFILE_DEFAULT_BUFFER_SIZE);

        if (!vgmstream->ch[1].streamfile) goto fail;

        vgmstream->ch[1].channel_start_offset=
            vgmstream->ch[1].offset=second_channel_start;
		}
	}
    
}

	return vgmstream;

    /* clean up anything we may have opened */
fail:
    if (vgmstream) close_vgmstream(vgmstream);
    return NULL;
}



