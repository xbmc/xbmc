#include "meta.h"
#include "../util.h"

/* SDT (Baldur's Gate - Dark Alliance) */
VGMSTREAM * init_vgmstream_sdt(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    char filename[260];
    off_t start_offset;

    int loop_flag;
	int channel_count;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("sdt",filename_extension(filename))) goto fail;

#if 0
    /* check header */
    if (read_32bitBE(0x00,streamFile) != 0x53565300) /* "SVS\0" */
        goto fail;
#endif

    loop_flag = (read_32bitBE(0x04,streamFile)!=0);
    channel_count = 2;
    
	/* build the VGMSTREAM */
    vgmstream = allocate_vgmstream(channel_count,loop_flag);
    if (!vgmstream) goto fail;

	/* fill in the vital statistics */
    start_offset = 0xA0;
	vgmstream->channels = read_32bitBE(0x00,streamFile);
    vgmstream->sample_rate = read_32bitBE(0x08,streamFile);
    vgmstream->coding_type = coding_NGC_DSP;
    vgmstream->num_samples = read_32bitBE(0x14,streamFile)/8*14/channel_count;
    if (loop_flag) {
        vgmstream->loop_start_sample = 0; /* (read_32bitLE(0x08,streamFile)-1)*28; */
        vgmstream->loop_end_sample = read_32bitBE(0x14,streamFile)/8*14/channel_count;
    }

		vgmstream->layout_type = layout_interleave;
		vgmstream->interleave_block_size = 0x8000;
		vgmstream->meta_type = meta_SDT;


if (vgmstream->coding_type == coding_NGC_DSP) {
	int i;
		for (i=0;i<16;i++) {
            vgmstream->ch[0].adpcm_coef[i] = read_16bitBE(0x3C+i*2,streamFile);
			}
		if (vgmstream->channels) {
			for (i=0;i<16;i++) {
            vgmstream->ch[1].adpcm_coef[i] = read_16bitBE(0x6A+i*2,streamFile);
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
