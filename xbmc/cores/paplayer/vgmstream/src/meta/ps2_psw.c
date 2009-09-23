#include "meta.h"
#include "../util.h"

/* PSW (from Rayman Raving Rabbids)
...coefs are missing for the dsp type... */
VGMSTREAM * init_vgmstream_ps2_psw(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    char filename[260];
    off_t start_offset;
    int loop_flag = 0;
	int channel_count;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("psw",filename_extension(filename))) goto fail;

    /* check header */
    if (read_32bitBE(0x00,streamFile) != 0x52494646 &&	/* "RIFF" */
		read_32bitBE(0x08,streamFile) != 0x57415645 &&	/* "WAVE" */
		read_32bitBE(0x26,streamFile) != 0x64617461)	/* "data" */
	goto fail;

    loop_flag = 0;
    channel_count = read_16bitLE(0x16,streamFile);
    
	/* build the VGMSTREAM */
    vgmstream = allocate_vgmstream(channel_count,loop_flag);
    if (!vgmstream) goto fail;

	/* fill in the vital statistics */
	switch ((uint16_t)read_16bitBE(0x14,streamFile)) {
		case 0xFFFF:
		start_offset = 0x2E;
		vgmstream->channels = channel_count;
		vgmstream->sample_rate = read_16bitLE(0x1C,streamFile);
	    vgmstream->coding_type = coding_PSX;
		vgmstream->num_samples = read_32bitLE(0x2A,streamFile)*28/16/channel_count;
    if (loop_flag) {
        vgmstream->loop_start_sample = loop_flag;
        vgmstream->loop_end_sample = read_32bitLE(0x2A,streamFile)*28/16/channel_count;
    }
		vgmstream->layout_type = layout_interleave;
		vgmstream->interleave_block_size = 0x6400;
		vgmstream->meta_type = meta_PS2_PSW;
	
	break;
		case 0xFEFF:
		start_offset = 0x2E;
		vgmstream->channels = channel_count;
		vgmstream->sample_rate = read_16bitLE(0x1C,streamFile);
	    vgmstream->coding_type = coding_NGC_DSP;
		vgmstream->num_samples = read_32bitLE(0x2A,streamFile)*28/16/channel_count;
    if (loop_flag) {
        vgmstream->loop_start_sample = loop_flag;
        vgmstream->loop_end_sample = read_32bitLE(0x2A,streamFile)*28/16/channel_count;
    }
		vgmstream->layout_type = layout_interleave;
		vgmstream->interleave_block_size = 0x12C00;
		vgmstream->meta_type = meta_PS2_PSW;

	break;
default:
	goto fail;
}

    /* open the file for reading */
    {
        int i;
        STREAMFILE * file;
        file = streamFile->open(streamFile,filename,STREAMFILE_DEFAULT_BUFFER_SIZE);
        if (!file) goto fail;
        for (i=0;i<channel_count;i++) {
            vgmstream->ch[i].streamfile = file;

            vgmstream->ch[i].channel_start_offset=
                vgmstream->ch[i].offset=start_offset+
                vgmstream->interleave_block_size*i;

        }
    }

    return vgmstream;

    /* clean up anything we may have opened */
fail:
    if (vgmstream) close_vgmstream(vgmstream);
    return NULL;
}
