#include "meta.h"
#include "../util.h"

/* RXW file (Arc the Lad) */
VGMSTREAM * init_vgmstream_ps2_rxw(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    char filename[260];
    int loop_flag=0;
	int channel_count;
    off_t start_offset;
    int i;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("rxw",filename_extension(filename))) goto fail;

    /* check RXWS/FORM Header */
    if (!((read_32bitBE(0x00,streamFile) == 0x52585753) && 
	      (read_32bitBE(0x10,streamFile) == 0x464F524D)))
        goto fail;

	/* check loop */
	loop_flag = (read_32bitLE(0x3C,streamFile)!=0xFFFFFFFF);
    
	/* Always stereo files */
	channel_count=2;
    
	/* build the VGMSTREAM */
    vgmstream = allocate_vgmstream(channel_count,loop_flag);
    if (!vgmstream) goto fail;

	/* fill in the vital statistics */
	vgmstream->channels = channel_count;
    vgmstream->sample_rate = read_32bitLE(0x2E,streamFile);

	vgmstream->coding_type = coding_PSX;
    vgmstream->num_samples = (read_32bitLE(0x38,streamFile)*28/16)/2;

	/* Get loop point values */
	if(vgmstream->loop_flag) {
		vgmstream->loop_start_sample = read_32bitLE(0x3C,streamFile)/16*14;
		vgmstream->loop_end_sample = read_32bitLE(0x38,streamFile)/16*14;
	}

	vgmstream->interleave_block_size = read_32bitLE(0x1c,streamFile)+0x10;
    vgmstream->layout_type = layout_interleave;
    vgmstream->meta_type = meta_PS2_RXW;

	start_offset = 0x40;

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
