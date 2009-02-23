#include "meta.h"
#include "../layout/layout.h"
#include "../util.h"

/* VS (from Men in Black) */
VGMSTREAM * init_vgmstream_vs(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;

	char filename[260];
    off_t start_offset;
    int loop_flag = 0;
	int channel_count;
    int i;


    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("vs",filename_extension(filename))) goto fail;

    /* check header */
    if (read_32bitBE(0x00,streamFile) != 0xC8000000) /* "0xC8000000" */
        goto fail;

    loop_flag = 0;
    channel_count = 2;
    
	/* build the VGMSTREAM */
    vgmstream = allocate_vgmstream(channel_count,loop_flag);
    if (!vgmstream) goto fail;

	/* fill in the vital statistics */
    start_offset = 0x08;
	vgmstream->channels = channel_count;
	vgmstream->interleave_block_size=0x10;
    vgmstream->sample_rate = read_32bitLE(0x04,streamFile);
    vgmstream->coding_type = coding_PSX;

#if 0
	if (loop_flag) {
        vgmstream->loop_start_sample = 0;
        vgmstream->loop_end_sample = (read_32bitLE(0x0c,streamFile)-start_offset);
    }
#endif

	vgmstream->layout_type = layout_vs_blocked;
	vgmstream->meta_type = meta_VS;

    
	/* open the file for reading */
	{
        for (i=0;i<channel_count;i++) {
            vgmstream->ch[i].streamfile = streamFile->open(streamFile,filename,0x2000);
            if (!vgmstream->ch[i].streamfile) goto fail;
        }
    }
	
	/* Calc num_samples */
	vs_block_update(start_offset,vgmstream);
	vgmstream->num_samples=0;

	do {
		vgmstream->num_samples += vgmstream->current_block_size*28/16;
		vs_block_update(vgmstream->next_block_offset,vgmstream);
	} while (vgmstream->next_block_offset<get_streamfile_size(streamFile));

	vs_block_update(start_offset,vgmstream);
    	

    return vgmstream;

    /* clean up anything we may have opened */
fail:
    if (vgmstream) close_vgmstream(vgmstream);
    return NULL;
}
