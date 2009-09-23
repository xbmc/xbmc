#include "meta.h"
#include "../util.h"

/* SNG (from Excite Truck [WII]) */
VGMSTREAM * init_vgmstream_wii_sng(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    char filename[260];
    off_t start_offset;
    int i;
    int loop_flag;
    int channel_count;
    int coef1;
    int coef2;
    int first_block_len;
    int second_channel_start;
    int dataBuffer = 0;
    int Founddata = 0;
    size_t file_size;
    off_t current_chunk;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("sng",filename_extension(filename))) goto fail;

    /* check header */
    if (read_32bitBE(0x00,streamFile) != 0x30545352) /* "0STR" */
        goto fail;
    if (read_32bitBE(0x04,streamFile) != 0x34000000) /* 0x34000000 */
        goto fail;
    if (read_32bitBE(0x08,streamFile) != 0x08000000) /* 0x08000000" */
        goto fail;
    if (read_32bitBE(0x0C,streamFile) != 0x01000000) /* 0x01000000 */
        goto fail;
    if (read_32bitLE(0x10,streamFile) != (get_streamfile_size(streamFile)))
        goto fail;

    loop_flag = (read_32bitLE(0x130,streamFile) !=0); /* not sure */
    channel_count = 2;
    
    /* build the VGMSTREAM */
    vgmstream = allocate_vgmstream(channel_count,loop_flag);
    if (!vgmstream) goto fail;

    /* fill in the vital statistics */
    start_offset = 0x180;
    vgmstream->channels = channel_count;
    vgmstream->sample_rate = read_32bitLE(0x110,streamFile);
    vgmstream->coding_type = coding_NGC_DSP;
    vgmstream->num_samples = read_32bitLE(0x100,streamFile)/16*14*2;
    if (loop_flag) {
        vgmstream->loop_start_sample = read_32bitBE(0x130,streamFile)/16*14;
        vgmstream->loop_end_sample = read_32bitBE(0x134,streamFile)/16*14;
    }

    vgmstream->layout_type = layout_none;
    vgmstream->meta_type = meta_WII_SNG;


    /* scan file until we find a "data" string */
    first_block_len = read_32bitLE(0x100,streamFile);
    file_size = get_streamfile_size(streamFile);
    {
        current_chunk = first_block_len;
        /* Start at 0 and loop until we reached the
        file size, or until we found a "data string */
        while (!Founddata && current_chunk < file_size) {
        dataBuffer = (read_32bitLE(current_chunk,streamFile));
            if (dataBuffer == first_block_len) { /* The value from the first block length */
                /* if "data" string found, retrieve the needed infos */
                Founddata = 1;
                second_channel_start = current_chunk+0x80;
                /* We will cancel the search here if we have a match */
            break;	
            }
            /* else we will increase the search offset by 1 */
            current_chunk = current_chunk + 1;
        }
    }

    coef1 = 0x13C;
    if (Founddata == 0) {
        goto fail;
    } else if (Founddata == 1) {
        coef2 = current_chunk+0x3C;
    }


        for (i=0;i<16;i++)
            vgmstream->ch[0].adpcm_coef[i] = read_16bitBE(coef1+i*2,streamFile);
		if (channel_count == 2) {
		for (i=0;i<16;i++)
            vgmstream->ch[1].adpcm_coef[i] = read_16bitBE(coef2+i*2,streamFile);
        }

    /* open the file for reading */
    {
        STREAMFILE * file;
        file = streamFile->open(streamFile,filename,STREAMFILE_DEFAULT_BUFFER_SIZE);
        if (!file) goto fail;
        for (i=0;i<channel_count;i++) {
            vgmstream->ch[i].streamfile = file;

	/* The first channel */
    vgmstream->ch[0].channel_start_offset=
        vgmstream->ch[0].offset=start_offset;

	/* The second channel */
    if (channel_count == 2) {
        vgmstream->ch[1].streamfile = streamFile->open(streamFile,filename,STREAMFILE_DEFAULT_BUFFER_SIZE);

        if (!vgmstream->ch[1].streamfile) goto fail;

        vgmstream->ch[i].channel_start_offset=
            vgmstream->ch[1].offset=second_channel_start;
	    }
    }
}

    return vgmstream;

    /* clean up anything we may have opened */
fail:
    if (vgmstream) close_vgmstream(vgmstream);
    return NULL;
}
