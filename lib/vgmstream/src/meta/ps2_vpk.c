#include "meta.h"
#include "../util.h"

/* VPK */

VGMSTREAM * init_vgmstream_ps2_vpk(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    char filename[260];

    int loop_flag=0;
	int channel_count;
    off_t start_offset;
    int i;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("vpk",filename_extension(filename))) goto fail;

    /* check VPK Header */
    if (read_32bitBE(0x00,streamFile) != 0x204B5056)
        goto fail;

	/* check loop */
	loop_flag = (read_32bitLE(0x7FC,streamFile)!=0);
    channel_count=read_32bitLE(0x14,streamFile);
    
	/* build the VGMSTREAM */
    vgmstream = allocate_vgmstream(channel_count,loop_flag);
    if (!vgmstream) goto fail;

	/* fill in the vital statistics */
	vgmstream->channels = read_32bitLE(0x14,streamFile);
    vgmstream->sample_rate = read_32bitLE(0x10,streamFile);

	/* Check for Compression Scheme */
	vgmstream->coding_type = coding_PSX;
    vgmstream->num_samples = read_32bitLE(0x04,streamFile)/16*28;

	/* Get loop point values */
	if(vgmstream->loop_flag) {
		vgmstream->loop_start_sample = read_32bitLE(0x7FC,streamFile);
		vgmstream->loop_end_sample = vgmstream->num_samples;
	}

	vgmstream->interleave_block_size = read_32bitLE(0x0C,streamFile)/2;
    vgmstream->layout_type = layout_interleave;
    vgmstream->meta_type = meta_PS2_VPK;

	start_offset = (off_t)read_32bitLE(0x08,streamFile);

    /* open the file for reading by each channel */
    {
        for (i=0;i<channel_count;i++) {
            vgmstream->ch[i].streamfile = streamFile->open(streamFile,filename,vgmstream->interleave_block_size);

            if (!vgmstream->ch[i].streamfile) goto fail;

            vgmstream->ch[i].channel_start_offset=
                vgmstream->ch[i].offset=
                (off_t)(start_offset+vgmstream->interleave_block_size*i);
        }
    }

    return vgmstream;

    /* clean up anything we may have opened */
fail:
    if (vgmstream) close_vgmstream(vgmstream);
    return NULL;
}
