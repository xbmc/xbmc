#include "meta.h"
#include "../util.h"

/* PCM (from Ephemeral Fantasia) */
VGMSTREAM * init_vgmstream_ps2_pcm(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    char filename[260];
    off_t start_offset;

	int loop_flag;
	int channel_count;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("pcm",filename_extension(filename))) goto fail;

    /* check header */
    if (read_32bitBE(0x410,streamFile) != 0x9CDB0740) /* 0x9CDB0740" */
        goto fail;

    loop_flag = (read_32bitLE(0x08,streamFile)!=0);
    channel_count = 2;
    
	/* build the VGMSTREAM */
    vgmstream = allocate_vgmstream(channel_count,loop_flag);
    if (!vgmstream) goto fail;



	/* fill in the vital statistics */
	start_offset = 0x800;
	vgmstream->channels = channel_count;
    vgmstream->sample_rate = 22050;
    vgmstream->coding_type = coding_PCM16LE;

		if(loop_flag == 0) {
			vgmstream->loop_start_sample = read_32bitLE(0x08,streamFile);
			vgmstream->loop_end_sample = read_32bitLE(0x00,streamFile)/2/channel_count;
			vgmstream->num_samples = read_32bitLE(0x00,streamFile)/2/channel_count;
		} else {
			vgmstream->loop_start_sample = read_32bitLE(0x08,streamFile);
			vgmstream->loop_end_sample = read_32bitLE(0x0C,streamFile);
			vgmstream->num_samples = read_32bitLE(0xC,streamFile);
		}

    vgmstream->layout_type = layout_interleave;
    vgmstream->interleave_block_size = 0x2;
    vgmstream->meta_type = meta_PS2_PCM;


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
