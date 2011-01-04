#include "meta.h"
#include "../util.h"
#include "../layout/layout.h"

/* matx

   MATX (found in Matrix)
*/

VGMSTREAM * init_vgmstream_xbox_matx(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    char filename[260];

    int loop_flag=0;
	int channel_count;
    int i;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("matx",filename_extension(filename))) goto fail;

	loop_flag = 0;
	channel_count=read_16bitLE(0x4,streamFile);
    
	/* build the VGMSTREAM */
    vgmstream = allocate_vgmstream(channel_count,loop_flag);
    if (!vgmstream) goto fail;

	/* fill in the vital statistics */
	vgmstream->channels = channel_count;
    vgmstream->sample_rate = read_16bitLE(0x06,streamFile) & 0xffff;
	vgmstream->coding_type = coding_XBOX;

    vgmstream->layout_type = layout_matx_blocked;
    vgmstream->meta_type = meta_XBOX_MATX;

    /* open the file for reading by each channel */
    {
        for (i=0;i<channel_count;i++) {
            vgmstream->ch[i].streamfile = streamFile->open(streamFile,filename,36);
            if (!vgmstream->ch[i].streamfile) goto fail;
        }
    }

	/* Calc num_samples */
	matx_block_update(0,vgmstream);
	vgmstream->num_samples=0;

	do {
		vgmstream->num_samples += vgmstream->current_block_size/36*64;
		matx_block_update(vgmstream->next_block_offset,vgmstream);
	} while (vgmstream->next_block_offset<get_streamfile_size(streamFile));

	matx_block_update(0,vgmstream);
    return vgmstream;

    /* clean up anything we may have opened */
fail:
    if (vgmstream) close_vgmstream(vgmstream);
    return NULL;
}
