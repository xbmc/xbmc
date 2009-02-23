#include "meta.h"
#include "../util.h"

/* SVS (from Unlimited Saga) */
/* probably Square Vag Stream */
VGMSTREAM * init_vgmstream_svs(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    char filename[260];
    off_t start_offset;

    int loop_flag = 0;
	int channel_count;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("svs",filename_extension(filename))) goto fail;

    /* check header */
    if (read_32bitBE(0x00,streamFile) != 0x53565300) /* "SVS\0" */
        goto fail;

    loop_flag = (read_32bitLE(0x08,streamFile)!=0);
    /* 63.SVS has start and end on the same sample, which crashes stuff */
    if (read_32bitLE(0x08,streamFile)==read_32bitLE(0x0c,streamFile))
        loop_flag = 0;
    channel_count = 2;
    
	/* build the VGMSTREAM */
    vgmstream = allocate_vgmstream(channel_count,loop_flag);
    if (!vgmstream) goto fail;

	/* fill in the vital statistics */
    start_offset = 0x40;
	vgmstream->channels = channel_count;
    vgmstream->sample_rate = 44100;
    vgmstream->coding_type = coding_PSX;
    vgmstream->num_samples = (get_streamfile_size(streamFile)-0x40)*28/16/channel_count;
    if (loop_flag) {
        vgmstream->loop_start_sample = (read_32bitLE(0x08,streamFile)-1)*28;
        vgmstream->loop_end_sample = (read_32bitLE(0x0c,streamFile)-1)*28;
    }

    vgmstream->layout_type = layout_interleave;
    vgmstream->interleave_block_size = 0x10;
    vgmstream->meta_type = meta_PS2_SVS;

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
