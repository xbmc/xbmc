#include "meta.h"
#include "../util.h"

/* MUSC (near all Spyro games and many other using this) */
VGMSTREAM * init_vgmstream_musc(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    char filename[260];
    off_t start_offset;
	int musc_version;
    int loop_flag = 0;
	int channel_count = 2;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("musc",filename_extension(filename))) goto fail;

    /* check header */
	if (read_32bitBE(0x0,streamFile) != 0x4D555343)   /* MUSC */
		goto fail;

    loop_flag = 0;
    channel_count = 2;
    
	/* build the VGMSTREAM */
    vgmstream = allocate_vgmstream(channel_count,loop_flag);
    if (!vgmstream) goto fail;

	musc_version=read_32bitLE(0x10,streamFile);
	switch (musc_version) {
		 /* This handles the 'new' MUSC Version, which has only a 32 byte header
		and doesn't provide any loop info!
		Spyro - The Eternal Night, Spyro - A new Beginning, Ty - The Tsamanian Tiger */
		case 0x20:
	  		start_offset = 0x20;
			vgmstream->coding_type = coding_PSX;
			vgmstream->num_samples = (read_32bitLE(0x14,streamFile))*28/16/channel_count;
		if (loop_flag) {
			vgmstream->loop_start_sample = 0;
			vgmstream->loop_end_sample = (read_32bitLE(0x14,streamFile))*28/16/channel_count;
	}
	break;
		default:
			goto fail;
}
	vgmstream->channels = channel_count;
	vgmstream->sample_rate = read_32bitLE(0x06,streamFile);	
	vgmstream->layout_type = layout_interleave;
    vgmstream->interleave_block_size = (read_32bitLE(0x18,streamFile))/2;
    vgmstream->meta_type = meta_MUSC;

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
