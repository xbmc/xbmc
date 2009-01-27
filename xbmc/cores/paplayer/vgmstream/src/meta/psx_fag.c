#include "meta.h"
#include "../util.h"

/* FAG (Jackie Chan - Stuntmaster) */
VGMSTREAM * init_vgmstream_psx_fag(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    char filename[260];
    off_t start_offset;
    int loop_flag = 0;
	int channel_count;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("fag",filename_extension(filename))) goto fail;

    /* check header */

	/* Look if there's more than 1 one file... */
    if (read_32bitBE(0x00,streamFile) != 0x01000000)
        goto fail;

	loop_flag = 0;
    channel_count = 2;
    
	/* build the VGMSTREAM */
    vgmstream = allocate_vgmstream(channel_count,loop_flag);
    if (!vgmstream) goto fail;

	/* fill in the vital statistics */
	start_offset = read_32bitLE(0x04,streamFile);
	vgmstream->channels = channel_count;
    vgmstream->sample_rate = 24000;
    vgmstream->coding_type = coding_PSX;
	vgmstream->num_samples = (read_32bitLE(0x08,streamFile))/channel_count/32*28;
    if (loop_flag) {
        vgmstream->loop_start_sample = 0;
        vgmstream->loop_end_sample = (read_32bitLE(0x08,streamFile))/channel_count/32*28;
	}

    vgmstream->layout_type = layout_interleave;
    vgmstream->interleave_block_size = 0x8000;
    vgmstream->meta_type = meta_PSX_FAG;

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
