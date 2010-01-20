#include "meta.h"
#include "../coding/coding.h"
#include "../util.h"

/* SEGA Stream Asset Builder...
    this meta handles only V1 and V3... */

VGMSTREAM * init_vgmstream_dc_str(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    char filename[260];
    off_t start_offset;
    int loop_flag = 0;
    int interleave;
    int channel_count;
    int samples;
    int i;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("str",filename_extension(filename))) goto fail;

    /* check header */
    if (read_32bitBE(0xD5,streamFile) != 0x53656761) /* "Sega" */
        goto fail;

    interleave = read_32bitLE(0xC,streamFile);
    if ((get_streamfile_size(streamFile)-0x800) != (read_32bitLE(0x10,streamFile) *
        ((read_32bitLE(0x0,streamFile)*(read_32bitLE(0x18,streamFile))))*interleave))
        goto fail;


    loop_flag = 0; /* (read_32bitLE(0x00,streamFile)!=0x00000000); */
    samples = read_32bitLE(0x08,streamFile);
    channel_count = (read_32bitLE(0x0,streamFile))*(read_32bitLE(0x18,streamFile));

    vgmstream = allocate_vgmstream(channel_count,loop_flag);
    if (!vgmstream) goto fail;


    /* fill in the vital statistics */
    switch (samples) {
        case 4:
            vgmstream->coding_type = coding_AICA;
            vgmstream->num_samples = read_32bitLE(0x14,streamFile);
        if (loop_flag) {
            vgmstream->loop_start_sample = 0;
            vgmstream->loop_end_sample = read_32bitLE(0x14,streamFile);
        }
            break;
        case 16:
            vgmstream->coding_type = coding_PCM16LE;
            vgmstream->num_samples = read_32bitLE(0x14,streamFile)/2/channel_count;
        if (loop_flag) {
            vgmstream->loop_start_sample = 0;
            vgmstream->loop_end_sample = read_32bitLE(0x14,streamFile)/2/channel_count;
        }
            break;
        default:
    goto fail;
}

    
    start_offset = 0x800;
    vgmstream->channels = channel_count;
    vgmstream->sample_rate = read_32bitLE(0x04,streamFile);
    
    if (vgmstream->channels == 1) {
        vgmstream->layout_type = layout_none;
    } else if (vgmstream->channels > 1) {
        vgmstream->interleave_block_size = interleave;
        vgmstream->layout_type = layout_interleave;
    }

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

/* This handles V2, not sure if it is really V2, cause the header is always
    the same, not like in V1 and V3, only found in "102 Dalmatians - Puppies to the Rescue"
    until now... */

VGMSTREAM * init_vgmstream_dc_str_v2(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    char filename[260];
    off_t start_offset;
    int loop_flag = 0;
	int channel_count;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("str",filename_extension(filename))) goto fail;
#if 0
    /* check header */
    if ((read_32bitBE(0x00,streamFile) != 0x00000002) &&
        (read_32bitBE(0x10,streamFile) != 0x00000100) &&
        (read_32bitBE(0x1C,streamFile) != 0x1F000000))
    goto fail;
#endif

    channel_count = 2;
   
	/* build the VGMSTREAM */
    vgmstream = allocate_vgmstream(channel_count,loop_flag);
    if (!vgmstream) goto fail;

	/* fill in the vital statistics */
    start_offset = 0x800;
	vgmstream->channels = channel_count;
    vgmstream->sample_rate = read_32bitLE(0x4,streamFile);
    vgmstream->coding_type = coding_PCM16LE;
    vgmstream->num_samples = (get_streamfile_size(streamFile)-start_offset)/2/channel_count;
    if (loop_flag) {
        vgmstream->loop_start_sample = 0;
        vgmstream->loop_end_sample = (get_streamfile_size(streamFile)-start_offset)/2/channel_count;
    }

    vgmstream->layout_type = layout_interleave;
    vgmstream->interleave_block_size = read_32bitLE(0xC,streamFile);
    vgmstream->meta_type = meta_DC_STR_V2;

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
