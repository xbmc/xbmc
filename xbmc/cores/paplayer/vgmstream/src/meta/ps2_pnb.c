#include "meta.h"
#include "../util.h"

/* PNB : PsychoNauts Bgm File */

VGMSTREAM * init_vgmstream_ps2_pnb(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    char filename[260];

    int loop_flag=0;
	int channel_count;
    off_t start_offset;
    int i;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("pnb",filename_extension(filename))) goto fail;

	/* check loop */
	loop_flag = (read_32bitLE(0x0C,streamFile)!=0xFFFFFFFF);
    channel_count=1;
    
	/* build the VGMSTREAM */
    vgmstream = allocate_vgmstream(channel_count,loop_flag);
    if (!vgmstream) goto fail;

	/* fill in the vital statistics */
	vgmstream->channels = 1;
    vgmstream->sample_rate = 44100;

	/* Check for Compression Scheme */
	vgmstream->coding_type = coding_PSX;
    vgmstream->num_samples = read_32bitBE(0x08,streamFile)/16*28;

	/* Get loop point values */
	if(vgmstream->loop_flag) {
		vgmstream->loop_start_sample = read_32bitBE(0x0C,streamFile)/16*28;
		vgmstream->loop_end_sample = vgmstream->num_samples;
	}

	vgmstream->interleave_block_size = 0x10;
    vgmstream->layout_type = layout_interleave;
    vgmstream->meta_type = meta_PS2_PNB;

	start_offset = (off_t)read_32bitBE(0x00,streamFile);

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
