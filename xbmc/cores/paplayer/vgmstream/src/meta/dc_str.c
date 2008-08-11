#include "meta.h"
#include "../coding/coding.h"
#include "../util.h"

VGMSTREAM * init_vgmstream_dc_str(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    char filename[260];
    off_t start_offset;

    int loop_flag = 0;
	int channel_count;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("str",filename_extension(filename))) goto fail;

    /* check header */
    if (read_32bitBE(0xD5,streamFile) != 0x53656761) /* "Sega" */
        goto fail;

    loop_flag = 0; /* (read_32bitBE(0x14,streamFile)!=0xFFFFFFFF); */
    channel_count = read_32bitLE(0x18,streamFile);
    
	/* build the VGMSTREAM */
    vgmstream = allocate_vgmstream(channel_count,loop_flag);
    if (!vgmstream) goto fail;

	/* fill in the vital statistics */
	vgmstream->channels = channel_count;
    start_offset = 0x800;
    vgmstream->sample_rate = read_32bitLE(0x04,streamFile);
    vgmstream->coding_type = coding_DVI_IMA;

    vgmstream->num_samples = read_32bitLE(0x14,streamFile);
    
	if (loop_flag) {
        vgmstream->loop_start_sample = 0; /* read_32bitBE(0x14,streamFile); */
        vgmstream->loop_end_sample = read_32bitLE(0x14,streamFile);
    }

    vgmstream->layout_type = layout_interleave;
    vgmstream->interleave_block_size = read_32bitLE(0x0C,streamFile);
	vgmstream->meta_type = meta_DC_STR;
    

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
