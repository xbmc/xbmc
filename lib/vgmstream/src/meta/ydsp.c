#include "meta.h"
#include "../util.h"

/* YDSP (from WWE Day of Reckoning) */
VGMSTREAM * init_vgmstream_ydsp(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    char filename[260];
    off_t start_offset;

	int loop_flag;
	int channel_count;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("ydsp",filename_extension(filename))) goto fail;

    /* check header */
    if (read_32bitBE(0x00,streamFile) != 0x59445350) /* "YDSP" */
        goto fail;

    loop_flag = (read_32bitBE(0xB0,streamFile)!=0);
    channel_count = 2; /* (uint32_t)read_16bitBE(0x10,streamFile); */
    
	/* build the VGMSTREAM */
    vgmstream = allocate_vgmstream(channel_count,loop_flag);
    if (!vgmstream) goto fail;

	/* fill in the vital statistics */
    start_offset = 0x120;
	vgmstream->channels = channel_count;
    vgmstream->sample_rate = read_32bitBE(0x0C,streamFile);
    vgmstream->coding_type = coding_NGC_DSP;
    vgmstream->num_samples = (read_32bitBE(0x08,streamFile))*14/8/channel_count;
    if (loop_flag) {
        vgmstream->loop_start_sample = read_32bitBE(0xB0,streamFile);
        vgmstream->loop_end_sample = read_32bitBE(0xB4,streamFile);
    }

	if (channel_count == 1) {
    		vgmstream->layout_type = layout_none;
    	} else if (channel_count == 2) {
    		vgmstream->interleave_block_size = read_32bitBE(0x14,streamFile);
    		vgmstream->layout_type = layout_interleave;
    	}
    	
    		vgmstream->meta_type = meta_YDSP;

    /* open the file for reading */
    
	if (vgmstream->coding_type == coding_NGC_DSP) {
        int i;
        for (i=0;i<16;i++) {
            vgmstream->ch[0].adpcm_coef[i] = read_16bitBE(0x20+i*2,streamFile);
        }
        if (vgmstream->channels == 2) {
            for (i=0;i<16;i++) {
                vgmstream->ch[1].adpcm_coef[i] = read_16bitBE(0x44+i*2,streamFile);
            }
        }
    }
				
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
