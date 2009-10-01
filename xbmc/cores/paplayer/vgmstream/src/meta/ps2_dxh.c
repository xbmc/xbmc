#include "meta.h"
#include "../util.h"

/* DXH (from Tokobot Plus - Mysteries of the Karakuri) */
VGMSTREAM * init_vgmstream_ps2_dxh(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    char filename[260];
    off_t start_offset;
    int loop_flag = 0;
	int channel_count;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("dxh",filename_extension(filename))) goto fail;

    /* check header */
    if (read_32bitBE(0x00,streamFile) != 0x00445848) /* 0\DXH" */
        goto fail;

    loop_flag = (read_32bitLE(0x50,streamFile)!=0);
    channel_count = read_32bitLE(0x08,streamFile);
    
	/* build the VGMSTREAM */
    vgmstream = allocate_vgmstream(channel_count,loop_flag);
    if (!vgmstream) goto fail;

	/* fill in the vital statistics */
	    start_offset = 0x800;
		vgmstream->channels = channel_count;
		vgmstream->sample_rate = read_32bitLE(0x20,streamFile);
	    
		if (read_32bitBE(0x54,streamFile) == 0) {
            /* if (loop_flag) { */
			vgmstream->loop_start_sample = 0;
			vgmstream->loop_end_sample = get_streamfile_size(streamFile)*28/16/channel_count;
			vgmstream->num_samples = get_streamfile_size(streamFile)*28/16/channel_count;
			/* } */

			} else {
		
			if (loop_flag) {
			vgmstream->loop_start_sample = (read_32bitLE(0x50,streamFile)*0x20)*28/16/channel_count;
			vgmstream->loop_end_sample = (read_32bitLE(0x54,streamFile)*0x20)*28/16/channel_count;
			vgmstream->num_samples = (read_32bitLE(0x54,streamFile)*0x20)*28/16/channel_count;

		}
}
    
		vgmstream->coding_type = coding_PSX;
		vgmstream->layout_type = layout_interleave;
		vgmstream->interleave_block_size = read_32bitLE(0x14,streamFile);
		vgmstream->meta_type = meta_PS2_DXH;	

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
