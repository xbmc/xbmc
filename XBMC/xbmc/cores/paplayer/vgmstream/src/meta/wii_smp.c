#include "meta.h"
#include "../util.h"

/* SMP (Mushroom Men: The Spore Wars ) */
VGMSTREAM * init_vgmstream_wii_smp(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    char filename[260];
    off_t start_offset;

    int loop_flag = 0;
	int channel_count;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("smp",filename_extension(filename))) goto fail;

    /* check header */
    if (read_32bitBE(0x00,streamFile) != 0x05000000) /* 0x05000000 */
        goto fail;

    loop_flag = 0;
    channel_count = read_32bitLE(0x28,streamFile);
    
	/* build the VGMSTREAM */
    vgmstream = allocate_vgmstream(channel_count,loop_flag);
    if (!vgmstream) goto fail;

	/* fill in the vital statistics */
    start_offset = read_32bitLE(0x1C,streamFile);
	vgmstream->channels = channel_count;
    vgmstream->sample_rate = read_32bitLE(0x30,streamFile);
    vgmstream->coding_type = coding_NGC_DSP;
    vgmstream->num_samples = read_32bitLE(0x34,streamFile)/2;
    if (loop_flag) {
        vgmstream->loop_start_sample = 0;
        vgmstream->loop_end_sample = read_32bitLE(0x34,streamFile)/2;
    }

	/* We have no interleave, so we have no layout */
    vgmstream->layout_type = layout_none;
    vgmstream->meta_type = meta_WII_SMP;

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
    
	/*Retrieving the coef table...
	This game has an exception, the coefs are stored in Little Endian... */
	{
        int i;
        for (i=0;i<16;i++) {
            vgmstream->ch[0].adpcm_coef[i] = read_16bitLE(0x50+i*2,streamFile);
        }
    }
        

    return vgmstream;

    /* clean up anything we may have opened */
fail:
    if (vgmstream) close_vgmstream(vgmstream);
    return NULL;
}
