#include "meta.h"
#include "../layout/layout.h"
#include "../util.h"

/* THP (Just play audio from .thp movie file) 
  by fastelbja */

VGMSTREAM * init_vgmstream_thp(STREAMFILE *streamFile) {
    
	VGMSTREAM *		vgmstream = NULL;
    
	char			filename[260];
    off_t			start_offset;
	
	uint32_t		maxAudioSize=0;

	uint32_t		numComponents;
	off_t			componentTypeOffset;
	off_t			componentDataOffset;
	
	char			thpVersion;

    int				loop_flag;
	int				channel_count=-1;
	int				i;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("thp",filename_extension(filename)) &&
		strcasecmp("dsp",filename_extension(filename))) goto fail;

    /* check header */
    if (read_32bitBE(0x00,streamFile) != 0x54485000)
        goto fail;

	maxAudioSize = read_32bitBE(0x0C,streamFile);
	thpVersion = read_8bit(0x06,streamFile);

	if(maxAudioSize==0) // no sound
		goto fail;

    loop_flag = 0; // allways unloop

	/* fill in the vital statistics */
    start_offset = read_32bitBE(0x28,streamFile); 

	// Get info from the first block
	componentTypeOffset = read_32bitBE(0x20,streamFile);
	numComponents = read_32bitBE(componentTypeOffset ,streamFile);
	componentDataOffset=componentTypeOffset+0x14;
	componentTypeOffset+=4;

	for(i=0;i<numComponents;i++) {
		if(read_8bit(componentTypeOffset+i,streamFile)==1) {
			channel_count=read_32bitBE(componentDataOffset,streamFile);

			/* build the VGMSTREAM */
			vgmstream = allocate_vgmstream(channel_count,loop_flag);
			if (!vgmstream) goto fail;

			vgmstream->channels=channel_count;
			vgmstream->sample_rate=read_32bitBE(componentDataOffset+4,streamFile);
			vgmstream->num_samples=read_32bitBE(componentDataOffset+8,streamFile);
			break;
		} else {
			if(thpVersion==0x10) 
				componentDataOffset+=0x0c;
			else
				componentDataOffset+=0x08;
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
        }
    }

	vgmstream->thpNextFrameSize=read_32bitBE(0x18,streamFile);
	thp_block_update(start_offset,vgmstream);

	vgmstream->coding_type = coding_NGC_DSP;
    vgmstream->layout_type = layout_thp_blocked;
    vgmstream->meta_type = meta_THP;

    return vgmstream;

    /* clean up anything we may have opened */
fail:
    if (vgmstream) close_vgmstream(vgmstream);
    return NULL;
}
