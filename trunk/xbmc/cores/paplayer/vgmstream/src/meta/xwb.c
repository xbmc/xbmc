#include "meta.h"
#include "../util.h"
/* XWB (found in King of Fighters 2003 (XBOX)) */
VGMSTREAM * init_vgmstream_xwb(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    char filename[260];
    off_t start_offset;
    int loop_flag = 0;
	int channel_count;
	int coding;
	int coding_flag;
	int num_samples;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("xwb",filename_extension(filename))) goto fail;

	/* check header */
    if (read_32bitBE(0x00,streamFile) != 0x57424E44) /* WBND */
		goto fail;
#if 0
	/* check if the file is been used as container */
	if (read_32bitBE(0x2C,streamFile) != 0x01000000)
		goto fail;
#endif
	/* if it's looped it should be 2, didn't see anything else... */
    loop_flag = (uint8_t)(read_8bit(0x50,streamFile));
	
	if (loop_flag == 2) {
		loop_flag = 1;
	} else if (loop_flag < 2) {
		loop_flag = 0;
	}

    channel_count = 2;
    
	/* build the VGMSTREAM */
    vgmstream = allocate_vgmstream(channel_count,loop_flag);
    if (!vgmstream) goto fail;

	/* fill in the vital statistics */
    start_offset = read_32bitLE(0x20,streamFile);
	vgmstream->channels = channel_count;
    vgmstream->sample_rate = 44100; /* read_32bitLE(0x08,streamFile); */
    
	coding_flag = (uint8_t)(read_8bit(0x52,streamFile));
	switch (coding_flag) {
		case 0:
			coding = coding_PCM16LE;
			vgmstream->layout_type = layout_interleave;
			vgmstream->interleave_block_size = 0x2;
			num_samples = read_32bitLE(0x5C,streamFile)/2/channel_count;
	if (loop_flag) {
		vgmstream->loop_start_sample = read_32bitLE(0x60,streamFile)/2/channel_count;
		vgmstream->loop_end_sample = read_32bitLE(0x5C,streamFile)/2/channel_count;
	}
		break;
		case 1:
			coding = coding_XBOX;
			vgmstream->layout_type = layout_none;
			num_samples = read_32bitLE(0x5C,streamFile)/36/channel_count*64;
	if (loop_flag) {
		vgmstream->loop_start_sample = read_32bitLE(0x60,streamFile)/36/channel_count*64;
		vgmstream->loop_end_sample = read_32bitLE(0x5C,streamFile)/36/channel_count*64;
	}
		break;
			default:
				goto fail;
	}

	vgmstream->coding_type = coding;
    vgmstream->num_samples = num_samples;
    vgmstream->meta_type = meta_XWB;

    /* open the file for reading */
    {
        int i;
        STREAMFILE * file;
        file = streamFile->open(streamFile,filename,STREAMFILE_DEFAULT_BUFFER_SIZE);
        if (!file) goto fail;
        for (i=0;i<channel_count;i++) {
            vgmstream->ch[i].streamfile = file;

            
            if (vgmstream->coding_type == coding_XBOX) {
                /* xbox interleaving is a little odd */
                vgmstream->ch[i].channel_start_offset=start_offset;
            } else {
                vgmstream->ch[i].channel_start_offset=
                    start_offset+vgmstream->interleave_block_size*i;
            }
            vgmstream->ch[i].offset = vgmstream->ch[i].channel_start_offset;

        }
    }

    return vgmstream;

    /* clean up anything we may have opened */
fail:
    if (vgmstream) close_vgmstream(vgmstream);
    return NULL;
}
