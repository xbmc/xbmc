#include "meta.h"
#include "../util.h"

/* ENTH (from Enthusia - Professional Racing) */
VGMSTREAM * init_vgmstream_ps2_enth(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    char filename[260];
    off_t start_offset;
	int header_check;
    int loop_flag;
	int channel_count;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("enth",filename_extension(filename))) goto fail;

	/* check header and loop_flag */
	header_check = read_32bitBE(0x00,streamFile);
	switch (header_check) {
		case 0x41502020: /* AP  */
		loop_flag = (read_32bitLE(0x14,streamFile)!=0);
	break;
		case 0x4C455020: /* LEP */
		loop_flag = (read_32bitLE(0x58,streamFile)!=0);
	break;
		default:
			goto fail;
	}

	channel_count = 2;
    
	/* build the VGMSTREAM */
    vgmstream = allocate_vgmstream(channel_count,loop_flag);
    if (!vgmstream) goto fail;

	/* fill in the vital statistics */
	header_check = read_32bitBE(0x00,streamFile);
	
	switch (header_check) {
		case 0x41502020: /* AP  */
			start_offset = read_32bitLE(0x1C,streamFile);
			vgmstream->channels = channel_count;
			vgmstream->sample_rate = read_32bitLE(0x08,streamFile);
			vgmstream->coding_type = coding_PSX;
			vgmstream->num_samples = (read_32bitLE(0x18,streamFile))*28/16/channel_count;
		if (loop_flag) {
			vgmstream->loop_start_sample = (read_32bitLE(0x14,streamFile))*28/16/channel_count;
			vgmstream->loop_end_sample = (read_32bitLE(0x18,streamFile))*28/16/channel_count;
		}
			vgmstream->interleave_block_size = read_32bitLE(0x0C,streamFile);
	break;
		case 0x4C455020: /* LEP */
			start_offset = 0x800;
			vgmstream->channels = channel_count;
			vgmstream->sample_rate = (uint16_t)read_16bitLE(0x12,streamFile);
			vgmstream->coding_type = coding_PSX;
			vgmstream->num_samples = (read_32bitLE(0x08,streamFile))*28/16/channel_count;
		if (loop_flag) {
			vgmstream->loop_start_sample = (read_32bitLE(0x58,streamFile))*28/16/channel_count;
			vgmstream->loop_end_sample = (read_32bitLE(0x08,streamFile))*28/16/channel_count;
		}
			vgmstream->interleave_block_size = 0x10;
	break;
		default:
			goto fail;
}


    vgmstream->layout_type = layout_interleave;
    vgmstream->meta_type = meta_PS2_ENTH;

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
