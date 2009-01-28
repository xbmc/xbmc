#include "meta.h"
#include "../util.h"

/* CAPDSP (found in Capcom games) */
VGMSTREAM * init_vgmstream_capdsp(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    char filename[260];
    off_t start_offset;

    int loop_flag;
	int channel_count;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("capdsp",filename_extension(filename))) goto fail;

    loop_flag = (read_32bitBE(0x14,streamFile) !=2);
    channel_count = read_32bitBE(0x10,streamFile);
    
	/* build the VGMSTREAM */
    vgmstream = allocate_vgmstream(channel_count,loop_flag);
    if (!vgmstream) goto fail;

	/* fill in the vital statistics */
    start_offset = 0x80;
	vgmstream->channels = channel_count;
    vgmstream->sample_rate = read_32bitBE(0x0C,streamFile);
    vgmstream->coding_type = coding_NGC_DSP;
    vgmstream->num_samples = read_32bitBE(0x04,streamFile);
    if (loop_flag) {
        vgmstream->loop_start_sample = read_32bitBE(0x14,streamFile)/8/channel_count*14;
        vgmstream->loop_end_sample = read_32bitBE(0x18,streamFile)/8/channel_count*14;
    }

    vgmstream->layout_type = layout_interleave;
    vgmstream->interleave_block_size = 0x2000;
    vgmstream->meta_type = meta_CAPDSP;

    if (vgmstream->coding_type == coding_NGC_DSP) {
        int i;
        for (i=0;i<8;i++) {
			vgmstream->ch[0].adpcm_coef[i*2]=read_16bitBE(0x20+i*2,streamFile);
			vgmstream->ch[0].adpcm_coef[i*2+1]=read_16bitBE(0x30+i*2,streamFile);
			vgmstream->ch[1].adpcm_coef[i*2]=read_16bitBE(0x40+i*2,streamFile);
			vgmstream->ch[1].adpcm_coef[i*2+1]=read_16bitBE(0x50+i*2,streamFile);
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
