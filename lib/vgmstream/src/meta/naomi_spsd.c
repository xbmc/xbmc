#include "meta.h"
#include "../util.h"

/* SPSD (Guilty Gear X [NAOMI GD-ROM]) */
VGMSTREAM * init_vgmstream_naomi_spsd(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    char filename[260];
    off_t start_offset;
    int loop_flag;
	int channel_count;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("spsd",filename_extension(filename))) goto fail;

    /* check header */
    if (read_32bitBE(0x00,streamFile) != 0x53505344) /* "SPSD" */
        goto fail;

    loop_flag = 0;
    channel_count = 2;
    
	/* build the VGMSTREAM */
    vgmstream = allocate_vgmstream(channel_count,loop_flag);
    if (!vgmstream) goto fail;

	/* fill in the vital statistics */
	vgmstream->channels = channel_count;
    start_offset = 0x40;
    vgmstream->sample_rate = (uint16_t)read_16bitLE(0x2A,streamFile);
    vgmstream->coding_type = coding_AICA;

    vgmstream->num_samples = read_32bitLE(0x0C,streamFile);
    
	if (loop_flag) {
        vgmstream->loop_start_sample = 0;
        vgmstream->loop_end_sample = read_32bitLE(0x0C,streamFile);
    }

    vgmstream->interleave_block_size = 0x2000;
    if (channel_count > 1) {
        vgmstream->interleave_smallblock_size = ((get_streamfile_size(streamFile)-start_offset)%(vgmstream->channels*vgmstream->interleave_block_size))/vgmstream->channels;
        vgmstream->layout_type = layout_interleave_shortblock;
    } else {
        vgmstream->layout_type = layout_none;
    }

    vgmstream->meta_type = meta_NAOMI_SPSD;

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
            	vgmstream->ch[i].adpcm_step_index = 0x7f;   /* AICA */
        }
    }

    return vgmstream;

    /* clean up anything we may have opened */
fail:
    if (vgmstream) close_vgmstream(vgmstream);
    return NULL;
}
