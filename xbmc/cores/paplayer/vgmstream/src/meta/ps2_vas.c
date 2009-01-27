#include "meta.h"
#include "../util.h"

/* VAS (from Pro Baseball Spirits 5) */
VGMSTREAM * init_vgmstream_ps2_vas(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    char filename[260];
    off_t start_offset;
	int loop_flag;
	int channel_count;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("vas",filename_extension(filename))) goto fail;

    /* check header */
#if 0
    if (read_32bitBE(0x00,streamFile) != 0x00000000) /* 0x0 */
       goto fail;
#endif

    loop_flag = (read_32bitLE(0x10,streamFile)!=0);
    channel_count = 2;
    
	/* build the VGMSTREAM */
    vgmstream = allocate_vgmstream(channel_count,loop_flag);
    if (!vgmstream) goto fail;

	/* fill in the vital statistics */
    start_offset = 0x800;
	vgmstream->channels = channel_count;
    vgmstream->sample_rate = read_32bitLE(0x04,streamFile);
    vgmstream->coding_type = coding_PSX;
    vgmstream->num_samples = read_32bitLE(0x00,streamFile)*28/16/channel_count;
    if (loop_flag) {
        vgmstream->loop_start_sample = read_32bitLE(0x14,streamFile)*28/16/channel_count;
        vgmstream->loop_end_sample = read_32bitLE(0x00,streamFile)*28/16/channel_count;
    }

    vgmstream->layout_type = layout_interleave;
    vgmstream->interleave_block_size = 0x200;
    vgmstream->meta_type = meta_PS2_VAS;

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
