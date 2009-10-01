#include "meta.h"
#include "../util.h"
/* RWX (found in Air Force Delta Storm (XBOX)) */
VGMSTREAM * init_vgmstream_rwx(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    char filename[260];
    off_t start_offset;

    int loop_flag = 0;
	int channel_count;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("rwx",filename_extension(filename))) goto fail;

	/* check header */
    if (read_32bitBE(0x00,streamFile) != 0x52415758)
		goto fail;
	
		
    loop_flag = read_32bitLE(0x0C,streamFile);
    channel_count = 2;
    
	/* build the VGMSTREAM */
    vgmstream = allocate_vgmstream(channel_count,loop_flag);
    if (!vgmstream) goto fail;


	/* fill in the vital statistics */
    start_offset = read_32bitLE(0x04,streamFile);
	vgmstream->channels = channel_count;
    vgmstream->sample_rate = read_32bitLE(0x08,streamFile);
    vgmstream->coding_type = coding_PCM16LE;
    vgmstream->num_samples = read_32bitLE(0x10,streamFile);
    if (loop_flag) {
        vgmstream->loop_start_sample = read_32bitLE(0x0C,streamFile);
        vgmstream->loop_end_sample = read_32bitLE(0x10,streamFile);
    }

    vgmstream->layout_type = layout_interleave;
    vgmstream->interleave_block_size = 0x2;
    vgmstream->meta_type = meta_RWX;

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
