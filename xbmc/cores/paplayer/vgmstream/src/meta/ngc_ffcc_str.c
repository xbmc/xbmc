#include "meta.h"
#include "../util.h"

/* STR (Final Fantasy: Crystal Chronicles) */
VGMSTREAM * init_vgmstream_ngc_ffcc_str(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    char filename[260];
    off_t start_offset;
    int loop_flag;
	int channel_count;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("str",filename_extension(filename))) goto fail;

    /* check header */
    if (read_32bitBE(0x00,streamFile) != 0x53545200 || /* "STR\0" */
        read_32bitBE(0x08,streamFile) != get_streamfile_size(streamFile) ||
        read_32bitBE(0x10,streamFile) != -1) /* this might be loop point */
        goto fail;

    loop_flag = 0;
    channel_count = read_32bitBE(0x18,streamFile);
    
	/* build the VGMSTREAM */
    vgmstream = allocate_vgmstream(channel_count,loop_flag);
    if (!vgmstream) goto fail;

	/* fill in the vital statistics */
    start_offset = 0x1000;
	vgmstream->channels = channel_count;
    if (read_32bitBE(0x14,streamFile)==0)
        vgmstream->sample_rate = 32000;
    else
        vgmstream->sample_rate = 44100;
    vgmstream->coding_type = coding_NGC_DSP;
    vgmstream->num_samples = read_32bitBE(0x0C,streamFile)*14;

    if (channel_count > 1)
    {
        vgmstream->layout_type = layout_interleave;
        vgmstream->interleave_block_size = 0x1000;
    }
    else
    {
        vgmstream->layout_type = layout_none;
        vgmstream->interleave_block_size = 0x1000;
    }
    vgmstream->meta_type = meta_FFCC_STR;


    if (vgmstream->coding_type == coding_NGC_DSP) {
        int c;
        for (c=0;c<channel_count;c++)
        {
            int i;
            for (i=0;i<16;i++) {
                vgmstream->ch[c].adpcm_coef[i] = read_16bitBE(0x20 + c * 0x2e + i * 2,streamFile);
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
