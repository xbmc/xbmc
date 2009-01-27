#include "meta.h"
#include "../util.h"

/* KRAW (from Geometry Wars - Galaxies) */
VGMSTREAM * init_vgmstream_kraw(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    char filename[260];
    off_t start_offset;
    int loop_flag = 0;
	int channel_count;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("kraw",filename_extension(filename))) goto fail;

    /* check header */
    if (read_32bitBE(0x00,streamFile) != 0x6B524157) /* "kRAW" */
        goto fail;

    loop_flag = 0;
    channel_count = 1;
    
	/* build the VGMSTREAM */
    vgmstream = allocate_vgmstream(channel_count,loop_flag);
    if (!vgmstream) goto fail;

	/* fill in the vital statistics */
    start_offset = 0x8;
	vgmstream->channels = channel_count;
    vgmstream->sample_rate = 32000;
    vgmstream->coding_type = coding_PCM16BE;
    vgmstream->num_samples = read_32bitBE(0x04,streamFile)/2/channel_count;
    if (loop_flag) {
        vgmstream->loop_start_sample = 0;
        vgmstream->loop_end_sample = read_32bitBE(0x04,streamFile)/2/channel_count;
    }

    vgmstream->layout_type = layout_none;
    vgmstream->meta_type = meta_KRAW;

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
