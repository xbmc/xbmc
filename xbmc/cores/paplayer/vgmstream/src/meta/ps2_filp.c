#include "meta.h"
#include "../layout/layout.h"
#include "../util.h"

/* FILp (Resident Evil - Dead Aim) */
VGMSTREAM * init_vgmstream_filp(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    char filename[260];
    off_t start_offset;
    int loop_flag = 0;
	int channel_count;
	int i;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("filp",filename_extension(filename))) goto fail;

    /* check header */
    if (read_32bitBE(0x0,streamFile) != 0x46494C70) /* "FILp" */
        goto fail;
	if (read_32bitBE(0x100,streamFile) != 0x56414770) /* "VAGp" */
        goto fail;
	if (read_32bitBE(0x130,streamFile) != 0x56414770) /* "VAGp" */
        goto fail;
	if (get_streamfile_size(streamFile) != read_32bitLE(0xC,streamFile))
		goto fail;

    loop_flag = (read_32bitLE(0x34,streamFile) == 0);
    channel_count = read_32bitLE(0x4,streamFile);

	/* build the VGMSTREAM */
    vgmstream = allocate_vgmstream(channel_count,loop_flag);
    if (!vgmstream) goto fail;

	/* fill in the vital statistics */
    start_offset = 0x0;
	vgmstream->channels = channel_count;
    vgmstream->sample_rate = read_32bitLE(0x110,streamFile);
    vgmstream->coding_type = coding_PSX;
    vgmstream->layout_type = layout_filp_blocked;
    vgmstream->meta_type = meta_FILP;

    /* open the file for reading */
    {
        STREAMFILE * file;
        file = streamFile->open(streamFile,filename,STREAMFILE_DEFAULT_BUFFER_SIZE);
        if (!file) goto fail;
        for (i=0;i<channel_count;i++) {
            vgmstream->ch[i].streamfile = file;
		}
	}
	
    filp_block_update(start_offset,vgmstream);
    vgmstream->num_samples = read_32bitLE(0x10C,streamFile)/16*28;
    if (loop_flag) {
        vgmstream->loop_start_sample = 0;
        vgmstream->loop_end_sample = vgmstream->num_samples;
    }

	
    return vgmstream;

    /* clean up anything we may have opened */
fail:
    if (vgmstream) close_vgmstream(vgmstream);
    return NULL;
}
