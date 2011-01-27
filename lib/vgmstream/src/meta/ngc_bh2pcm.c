#include "meta.h"
#include "../util.h"

/* BH2PCM (from Bio Hazard 2) */
VGMSTREAM * init_vgmstream_ngc_bh2pcm(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    char filename[260];
    off_t start_offset;
	int channel_count;
	int format_detect;
    int loop_flag;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("bh2pcm",filename_extension(filename))) goto fail;

#if 0
    /* check header */
    if (read_32bitBE(0x00,streamFile) != 0x00000000)
        goto fail;
#endif

    loop_flag = 0;
	channel_count = 2;

	/* build the VGMSTREAM */
    vgmstream = allocate_vgmstream(channel_count,loop_flag);
    if (!vgmstream) goto fail;

	/* fill in the vital statistics */
    format_detect=read_32bitLE(0x00,streamFile);
		switch (format_detect) {
			case 1:
				start_offset = 0x20;
				channel_count = 2;
				vgmstream->channels = channel_count;
				vgmstream->sample_rate = 32000;
				vgmstream->num_samples = read_32bitLE(0x04,streamFile)/2;
				vgmstream->layout_type = layout_interleave;
				vgmstream->interleave_block_size = read_32bitLE(0x04,streamFile);
				if (loop_flag) {
					vgmstream->loop_start_sample = 0;
					vgmstream->loop_end_sample = read_32bitLE(0x14,streamFile);
				}
			break;
			case 0:
				start_offset = 0x20;
				channel_count = 1;
				vgmstream->channels = channel_count;
				vgmstream->sample_rate = 32000;
				vgmstream->num_samples = read_32bitLE(0x0C,streamFile);
				vgmstream->layout_type = layout_none;
				if (loop_flag) {
					vgmstream->loop_start_sample = read_32bitLE(0x08,streamFile);
					vgmstream->loop_end_sample = read_32bitLE(0x0C,streamFile);
				}
			break;
				default:
					goto fail;
			}

		vgmstream->coding_type = coding_PCM16BE;
		vgmstream->meta_type = meta_NGC_BH2PCM;

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
