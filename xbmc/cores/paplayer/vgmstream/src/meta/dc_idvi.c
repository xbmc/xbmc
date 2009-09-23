#include "meta.h"
#include "../util.h"

/* IDVI (Eldorado Gate Volume 1-7) */
VGMSTREAM * init_vgmstream_dc_idvi(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    char filename[260];
    off_t start_offset;

    int loop_flag;
	int channel_count;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("idvi",filename_extension(filename))) goto fail;

    /* check header */
    if (read_32bitBE(0x00,streamFile) != 0x49445649) /* "IDVI." */
        goto fail;

    loop_flag = read_32bitLE(0x0C,streamFile);
    channel_count = read_32bitLE(0x04,streamFile);
    
	/* build the VGMSTREAM */
    vgmstream = allocate_vgmstream(channel_count,loop_flag);
    if (!vgmstream) goto fail;

	/* fill in the vital statistics */
	vgmstream->channels = channel_count;
    start_offset = 0x800;
    vgmstream->sample_rate = read_32bitLE(0x08,streamFile);
    vgmstream->coding_type = coding_INT_DVI_IMA;
    vgmstream->num_samples = (get_streamfile_size(streamFile)-start_offset);
	if (loop_flag) {
        vgmstream->loop_start_sample = read_32bitLE(0x0C,streamFile);
        vgmstream->loop_end_sample = (get_streamfile_size(streamFile)-start_offset);
    }
    vgmstream->meta_type = meta_DC_IDVI;

	/* Calculating the short block... */
	if (channel_count > 1) {
		vgmstream->interleave_block_size = 0x400;
		vgmstream->interleave_smallblock_size = ((get_streamfile_size(streamFile)-start_offset)%(vgmstream->channels*vgmstream->interleave_block_size))/vgmstream->channels;
        vgmstream->layout_type = layout_interleave_shortblock;
    } else {
        vgmstream->layout_type = layout_none;
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
