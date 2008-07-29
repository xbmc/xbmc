#include "meta.h"
#include "../util.h"

/* Sony .ADS with SShd & SSbd Headers */

VGMSTREAM * init_vgmstream_ps2_npsf(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    char filename[260];

    int loop_flag=0;
	int channel_count;
    off_t start_offset;
    int i;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("npsf",filename_extension(filename))) goto fail;

    /* check NPSF Header */
    if (read_32bitBE(0x00,streamFile) != 0x4E505346)
        goto fail;

	/* check loop */
	loop_flag = (read_32bitLE(0x14,streamFile)!=0xFFFFFFFF);
    channel_count=read_32bitLE(0x0C,streamFile);
    
	/* build the VGMSTREAM */
    vgmstream = allocate_vgmstream(channel_count,loop_flag);
    if (!vgmstream) goto fail;

	/* fill in the vital statistics */
	vgmstream->channels = read_32bitLE(0x0C,streamFile);
    vgmstream->sample_rate = read_32bitLE(0x18,streamFile);

	/* Check for Compression Scheme */
	vgmstream->coding_type = coding_PSX;
    vgmstream->num_samples = read_32bitLE(0x08,streamFile)*28/16;

	/* Get loop point values */
	if(vgmstream->loop_flag) {
		vgmstream->loop_start_sample = read_32bitLE(0x14,streamFile);
		vgmstream->loop_end_sample = read_32bitLE(0x08,streamFile)*28/16;
	}

	vgmstream->interleave_block_size = read_32bitLE(0x04,streamFile)/2;
    vgmstream->layout_type = layout_interleave;
    vgmstream->meta_type = meta_PS2_NPSF;

	start_offset = (off_t)read_32bitLE(0x10,streamFile);

    /* open the file for reading by each channel */
    {
        for (i=0;i<channel_count;i++) {
            vgmstream->ch[i].streamfile = streamFile->open(streamFile,filename,0x8000);

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
