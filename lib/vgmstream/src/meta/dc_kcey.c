#include "meta.h"
#include "../coding/coding.h"
#include "../util.h"

VGMSTREAM * init_vgmstream_kcey(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    char filename[260];
    off_t start_offset;

    int loop_flag = 0;
	int channel_count;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("kcey",filename_extension(filename))) goto fail;

    /* check header */
    if (read_32bitBE(0x00,streamFile) != 0x4B434559) /* "DVI." */
        goto fail;

    loop_flag = (read_32bitBE(0x14,streamFile)!=0xFFFFFFFF);
    channel_count = read_32bitBE(0x08,streamFile);
    
	/* build the VGMSTREAM */
    vgmstream = allocate_vgmstream(channel_count,loop_flag);
    if (!vgmstream) goto fail;

	/* fill in the vital statistics */
	vgmstream->channels = channel_count;
    start_offset = read_32bitBE(0x10,streamFile);
    vgmstream->sample_rate = 37800;
    vgmstream->coding_type = coding_EACS_IMA;

    vgmstream->num_samples = read_32bitBE(0x0C,streamFile);
    
	if (loop_flag) {
        vgmstream->loop_start_sample = read_32bitBE(0x14,streamFile);
        vgmstream->loop_end_sample = read_32bitBE(0x0C,streamFile);
    }

    vgmstream->layout_type = layout_none;
    vgmstream->meta_type = meta_KCEY;
	vgmstream->get_high_nibble=1;

	/* open the file for reading */
    {
        int i;
        STREAMFILE * file;
        file = streamFile->open(streamFile,filename,STREAMFILE_DEFAULT_BUFFER_SIZE);
        if (!file) goto fail;
        for (i=0;i<channel_count;i++) {
            vgmstream->ch[i].streamfile = file;
            vgmstream->ch[i].channel_start_offset=
                vgmstream->ch[i].offset=start_offset+(i*vgmstream->interleave_block_size);
			vgmstream->ch[i].adpcm_history1_32=0; 
			vgmstream->ch[i].adpcm_step_index=0; 
        }
    }

    return vgmstream;

    /* clean up anything we may have opened */
fail:
    if (vgmstream) close_vgmstream(vgmstream);
    return NULL;
}
