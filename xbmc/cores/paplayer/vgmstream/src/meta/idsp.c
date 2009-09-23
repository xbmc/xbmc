#include "meta.h"
#include "../util.h"

/* IDSP (Chronicles of Narnia Wii) */
VGMSTREAM * init_vgmstream_idsp(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    char filename[260];
    off_t start_offset;

    int loop_flag;
	int channel_count;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("idsp",filename_extension(filename))) goto fail;

    /* check header */
    if (read_32bitBE(0x00,streamFile) != 0x49445350) /* "IDSP" */
        goto fail;


    loop_flag = 0;
    channel_count = read_32bitBE(0x04,streamFile);
    
	/* build the VGMSTREAM */
    vgmstream = allocate_vgmstream(channel_count,loop_flag);
    if (!vgmstream) goto fail;

	/* fill in the vital statistics */
    start_offset = 0xD0;
	vgmstream->channels = channel_count;
    vgmstream->sample_rate = read_32bitBE(0x28,streamFile);
    vgmstream->coding_type = coding_NGC_DSP;
    vgmstream->num_samples = read_32bitBE(0x20,streamFile);
    if (loop_flag) {
        vgmstream->loop_start_sample = 0;
        vgmstream->loop_end_sample = read_32bitBE(0x20,streamFile);
    }

    vgmstream->layout_type = layout_interleave;
    vgmstream->interleave_block_size = read_32bitBE(0x0C,streamFile);
    vgmstream->meta_type = meta_IDSP;


    if (vgmstream->coding_type == coding_NGC_DSP) {
        int i;
        for (i=0;i<16;i++) {
            vgmstream->ch[0].adpcm_coef[i] = read_16bitBE(0x3C+i*2,streamFile);
        }
        if (vgmstream->channels) {
            for (i=0;i<16;i++) {
                vgmstream->ch[1].adpcm_coef[i] = read_16bitBE(0x9C+i*2,streamFile);
            }
        }
    }

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



/* IDSP (Soul Calibur Legends Wii) */
VGMSTREAM * init_vgmstream_idsp2(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    char filename[260];
    off_t start_offset;

    int loop_flag;
	int channel_count;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("idsp",filename_extension(filename))) goto fail;

    /* check header */
    if (read_32bitBE(0x00,streamFile) != 0x69647370 || /* "idsp" */
				read_32bitBE(0xBC,streamFile) != 0x49445350) /* IDSP */
        goto fail;


    loop_flag = read_32bitBE(0x20,streamFile);
    channel_count = read_32bitBE(0xC4,streamFile);
    
	/* build the VGMSTREAM */
    vgmstream = allocate_vgmstream(channel_count,loop_flag);
    if (!vgmstream) goto fail;

	/* fill in the vital statistics */
    start_offset = 0x1C0;
	vgmstream->channels = channel_count;
    vgmstream->sample_rate = read_32bitBE(0xC8,streamFile);
    vgmstream->coding_type = coding_NGC_DSP;
    vgmstream->num_samples = (read_32bitBE(0x14,streamFile))*14/8/channel_count;
    if (loop_flag) {
        vgmstream->loop_start_sample = (read_32bitBE(0xD0,streamFile));
        vgmstream->loop_end_sample = (read_32bitBE(0xD4,streamFile));
    }

	    vgmstream->layout_type = layout_interleave;
		vgmstream->interleave_block_size = 0x10;
		vgmstream->meta_type = meta_IDSP2;


    if (vgmstream->coding_type == coding_NGC_DSP) {
        int i;
        for (i=0;i<16;i++) {
            vgmstream->ch[0].adpcm_coef[i] = read_16bitBE(0x118+i*2,streamFile);
        }
        if (vgmstream->channels) {
            for (i=0;i<16;i++) {
                vgmstream->ch[1].adpcm_coef[i] = read_16bitBE(0x178+i*2,streamFile);
            }
        }
    }

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



