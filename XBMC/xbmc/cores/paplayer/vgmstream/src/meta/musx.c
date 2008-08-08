#include "meta.h"
#include "../util.h"

/* MUSX (from Spyro, possibly more Vivendi games) */
VGMSTREAM * init_vgmstream_musx(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    char filename[260];
    off_t start_offset;

	/*int musx_version;*/ /* 0x08 provides a "version" byte??? */
	int musx_type;
	int loop_flag = 0;
	int channel_count;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("musx",filename_extension(filename))) goto fail;

    /* check header */
    if (read_32bitBE(0x00,streamFile) != 0x4D555358) /* "MUSX" */
        goto fail;

    loop_flag = (read_32bitLE(0x840,streamFile)!=0xFFFFFFFF);
    channel_count = 2;
    
	/* build the VGMSTREAM */
    vgmstream = allocate_vgmstream(channel_count,loop_flag);
    if (!vgmstream) goto fail;
	
	/* fill in the vital statistics */	
	musx_type=(read_32bitBE(0x10,streamFile));

	switch (musx_type) {
	case 0x5053325F: /* PS2_ */
			start_offset = read_32bitLE(0x28,streamFile);
			vgmstream->channels = channel_count;
			vgmstream->sample_rate = 32000;
			vgmstream->coding_type = coding_PSX;
			vgmstream->num_samples = (read_32bitLE(0x0C,streamFile))*28/16/channel_count;;
			vgmstream->layout_type = layout_interleave;
			vgmstream->interleave_block_size = 0x80;
			vgmstream->meta_type = meta_MUSX;
		if (loop_flag) {
			vgmstream->loop_start_sample = (read_32bitLE(0x890,streamFile))*28/16/channel_count;
			vgmstream->loop_end_sample = (read_32bitLE(0x89C,streamFile))*28/16/channel_count;
		}

		
	
	
	break;
		default:
			goto fail;

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
