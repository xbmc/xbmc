#include "meta.h"
#include "../util.h"

/* STMA

   STMA (found in Midnight Club 2)
*/

VGMSTREAM * init_vgmstream_xbox_stma(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    char filename[260];

    int loop_flag=0;
	int channel_count;
    int i;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("stma",filename_extension(filename))) goto fail;

	if(read_32bitBE(0x0,streamFile)!=0x53544D41)
		goto fail;

	loop_flag = ((read_32bitLE(0x20,streamFile)==1) || (read_32bitLE(0x18,streamFile)>read_32bitLE(0x1C,streamFile)));
    
	/* Seems that the loop flag is not allways well defined */
	/* Some of the tracks should loop, but without flag set */
	channel_count=read_32bitLE(0x14,streamFile);
    
	/* build the VGMSTREAM */
    vgmstream = allocate_vgmstream(channel_count,loop_flag);
    if (!vgmstream) goto fail;

	/* fill in the vital statistics */
	vgmstream->channels = channel_count;
    vgmstream->sample_rate = read_32bitLE(0x0C,streamFile);
	vgmstream->coding_type = coding_INT_DVI_IMA;
    vgmstream->num_samples = read_32bitLE(0x18,streamFile)*2/vgmstream->channels;
    vgmstream->layout_type = layout_interleave;
	vgmstream->interleave_block_size=0x40;
    vgmstream->meta_type = meta_XBOX_STMA;

	if(loop_flag) {
		vgmstream->loop_start_sample=read_32bitLE(0x24,streamFile);
		vgmstream->loop_end_sample=vgmstream->num_samples;
	}

    /* open the file for reading by each channel */
    {
        for (i=0;i<channel_count;i++) {
            vgmstream->ch[i].streamfile = streamFile->open(streamFile,filename,36);
            vgmstream->ch[i].offset = 0x800+(i*vgmstream->interleave_block_size);

            if (!vgmstream->ch[i].streamfile) goto fail;
        }
    }

    return vgmstream;

    /* clean up anything we may have opened */
fail:
    if (vgmstream) close_vgmstream(vgmstream);
    return NULL;
}
