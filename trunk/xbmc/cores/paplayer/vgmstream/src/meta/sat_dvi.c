#include "meta.h"
#include "../util.h"

/* DVI (Castlevania Symphony of the Night) */
VGMSTREAM * init_vgmstream_dvi(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    char filename[260];
    off_t start_offset;

    int loop_flag = 0;
	int channel_count;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("dvi",filename_extension(filename))) goto fail;

    /* check header */
    if (read_32bitBE(0x00,streamFile) != 0x4456492E) /* "DVI." */
        goto fail;

    loop_flag = (read_32bitBE(0x0C,streamFile)!=0xFFFFFFFF);
    channel_count = 2;
    
	/* build the VGMSTREAM */
    vgmstream = allocate_vgmstream(channel_count,loop_flag);
    if (!vgmstream) goto fail;

	/* fill in the vital statistics */
	vgmstream->channels = channel_count;
    start_offset = read_32bitBE(0x04,streamFile);
    vgmstream->sample_rate = 44100;
    vgmstream->coding_type = coding_INT_DVI_IMA;

    vgmstream->num_samples = read_32bitBE(0x08,streamFile);
    
	if (loop_flag) {
        vgmstream->loop_start_sample = read_32bitBE(0x0C,streamFile);
        vgmstream->loop_end_sample = read_32bitBE(0x08,streamFile);
    }

    vgmstream->layout_type = layout_interleave;
	vgmstream->interleave_block_size = 4;
    vgmstream->meta_type = meta_DVI;
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
