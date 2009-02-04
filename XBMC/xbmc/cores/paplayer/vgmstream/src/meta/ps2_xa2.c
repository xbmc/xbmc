#include "meta.h"
#include "../util.h"

/* XA2 (XG3 Extreme-G Racing) */
VGMSTREAM * init_vgmstream_ps2_xa2(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    char filename[260];
    off_t start_offset;

    int loop_flag = 0;
	int channel_count;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("xa2",filename_extension(filename))) goto fail;

    loop_flag = (read_32bitLE(0x10,streamFile)!=0);
    channel_count = read_32bitLE(0x0,streamFile);
    
	/* build the VGMSTREAM */
    vgmstream = allocate_vgmstream(channel_count,loop_flag);
    if (!vgmstream) goto fail;

	/* fill in the vital statistics */
    start_offset = 0x800;
	vgmstream->channels = channel_count;
    vgmstream->sample_rate = 44100;
    vgmstream->coding_type = coding_PSX;
    vgmstream->num_samples = read_32bitLE(0x0C,streamFile);
    if (loop_flag) {
        vgmstream->loop_start_sample = vgmstream->num_samples-read_32bitLE(0x08,streamFile);
        vgmstream->loop_end_sample = vgmstream->num_samples;
    }

    vgmstream->layout_type = layout_interleave;
    vgmstream->interleave_block_size = read_32bitLE(0x04,streamFile);
    vgmstream->meta_type = meta_PS2_XA2;

    /* open the file for reading */
    {
        int i;
        STREAMFILE * file;
        file = streamFile->open(streamFile,filename,STREAMFILE_DEFAULT_BUFFER_SIZE);
        if (!file) goto fail;
        for (i=0;i<channel_count;i++) {
            vgmstream->ch[i].streamfile = file;

            vgmstream->ch[i].channel_start_offset=
                vgmstream->ch[i].offset=start_offset;

        }
    }

    return vgmstream;

    /* clean up anything we may have opened */
fail:
    if (vgmstream) close_vgmstream(vgmstream);
    return NULL;
}
