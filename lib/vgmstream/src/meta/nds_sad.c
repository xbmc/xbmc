#include "meta.h"
#include "../util.h"

/* sadl (only the Professor Layton interleaved IMA version) */
VGMSTREAM * init_vgmstream_sadl(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    char filename[260];
    off_t start_offset;

    int loop_flag;
	int channel_count;
    int coding_type;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("sad",filename_extension(filename))) goto fail;

    /* check header */
    if (read_32bitBE(0x00,streamFile) != 0x7361646c) /* "sadl" */
        goto fail;

    /* check file size */
    if (read_32bitLE(0x40,streamFile) != get_streamfile_size(streamFile) )
        goto fail;

    /* check coding type */
    switch (read_8bit(0x33,streamFile)&0xf0)
    {
        case 0x70:
            coding_type = coding_INT_IMA;
            break;
        case 0xb0:
            coding_type = coding_NDS_PROCYON;
            break;
        default:
            goto fail;
    }

    loop_flag = read_8bit(0x31,streamFile);
    channel_count = read_8bit(0x32,streamFile);
    
	/* build the VGMSTREAM */
    vgmstream = allocate_vgmstream(channel_count,loop_flag);
    if (!vgmstream) goto fail;

	/* fill in the vital statistics */
    start_offset = 0x100;
	vgmstream->channels = channel_count;

    switch (read_8bit(0x33,streamFile) & 6)
    {
        case 4:
            vgmstream->sample_rate = 32728;
            break;
        case 2:
            vgmstream->sample_rate = 16364;
            break;
        default:
            goto fail;
    }

    vgmstream->coding_type = coding_type;

    if (coding_type == coding_INT_IMA)
        vgmstream->num_samples = 
            (read_32bitLE(0x40,streamFile)-start_offset)/channel_count*2;
    else if (coding_type == coding_NDS_PROCYON)
        vgmstream->num_samples = 
            (read_32bitLE(0x40,streamFile)-start_offset)/channel_count/16*30;

    vgmstream->interleave_block_size=0x10;

    if (loop_flag)
    {
        if (coding_type == coding_INT_IMA)
            vgmstream->loop_start_sample = (read_32bitLE(0x54,streamFile)-start_offset)/channel_count*2;
        else
            vgmstream->loop_start_sample = (read_32bitLE(0x54,streamFile)-start_offset)/channel_count/16*30;
        vgmstream->loop_end_sample = vgmstream->num_samples;
    }

    if (channel_count > 1)
        vgmstream->layout_type = layout_interleave;
    else
        vgmstream->layout_type = layout_none;
    vgmstream->meta_type = meta_SADL;

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
