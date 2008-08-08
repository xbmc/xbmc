#include "meta.h"
#include "../util.h"

/* RSD (Crash Bandicot games, possibly more) */
VGMSTREAM * init_vgmstream_rsd(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    char filename[260];
    off_t start_offset;
    
	coding_t coding_type;
    
	int loop_flag=0;
	int channel_count;
	int rsd_ident;
    
	/* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("rsd",filename_extension(filename))) goto fail;


    /* check header */
	if (read_32bitBE(0x0,streamFile) != 0x52534436 &&   /* RSD6 */
		read_32bitBE(0x0,streamFile) != 0x52534434)	/* RSD4 */
        goto fail;

	loop_flag = 0; /* (read_32bitLE(checkZERO-0x4,streamFile)!=0); */
    channel_count = (read_32bitLE(0x8,streamFile));
    

	/* build the VGMSTREAM */
    vgmstream = allocate_vgmstream(channel_count,loop_flag);
	if (!vgmstream) goto fail;


	rsd_ident = read_32bitBE(0x4,streamFile);

	/* fill in the vital statistics */
    
	switch (rsd_ident) {
    case 0x56414720: /* RSD4VAG & RSD6VAG */
		start_offset = 0x800;
		coding_type = coding_PSX;
		vgmstream->interleave_block_size = read_32bitLE(0x0C,streamFile);
		vgmstream->num_samples = (get_streamfile_size(streamFile)-start_offset)*28/16/channel_count;
	if (loop_flag) {
        vgmstream->loop_start_sample = (uint16_t)(read_32bitLE(0x12,streamFile))*28/16/channel_count;
        vgmstream->loop_end_sample = (get_streamfile_size(streamFile)-start_offset)*28/16/channel_count;
	}
	break;

	case 0x50434D20: /* RSD4PCM */
		start_offset = 0x800;
		coding_type = coding_PCM16LE;
		vgmstream->interleave_block_size = 0x2;
		vgmstream->num_samples = (get_streamfile_size(streamFile)-start_offset)/2/channel_count;
	if (loop_flag) {
        vgmstream->loop_start_sample = (uint16_t)(read_32bitLE(0x12,streamFile))/2/channel_count;;
        vgmstream->loop_end_sample = (get_streamfile_size(streamFile)-start_offset)/2/channel_count;
    }
	break;
    
	case 0x52414450: /* RSD4RADP */
		start_offset = 0x800;
		coding_type = coding_NGC_DTK; /* or DSP??? */
		vgmstream->interleave_block_size = 0x10;
		vgmstream->num_samples = (get_streamfile_size(streamFile)-start_offset)/2/channel_count;
	if (loop_flag) {
        vgmstream->loop_start_sample = (uint16_t)(read_32bitLE(0x12,streamFile))/2/channel_count;;
        vgmstream->loop_end_sample = (get_streamfile_size(streamFile)-start_offset)/2/channel_count;
    }
	break;
  	
	case 0x50434D42: /* RSD4PCMB */
		start_offset = 0x80;
		coding_type = coding_PCM16BE;
		vgmstream->interleave_block_size = 0x2;
		vgmstream->num_samples = (get_streamfile_size(streamFile)-start_offset)/2/channel_count;

		/* loop_flag = checkZERO; */

	if (loop_flag) {
        vgmstream->loop_start_sample = 0; /* (uint16_t)(read_32bitLE(0x12,streamFile))/2/channel_count;; */
        vgmstream->loop_end_sample = (get_streamfile_size(streamFile)-start_offset)/2/channel_count;
    }
	break;


	case 0x584D4120: /* RSD6XMA */
		goto fail;	
		default:
			goto fail;

}




	
	vgmstream->channels = channel_count;
    vgmstream->sample_rate = (uint16_t)read_16bitLE(0x10,streamFile);
	vgmstream->coding_type = coding_type;

	vgmstream->layout_type = layout_interleave;
	vgmstream->meta_type = meta_RSD;
	

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

