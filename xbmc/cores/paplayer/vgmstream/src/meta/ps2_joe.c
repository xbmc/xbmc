#include "meta.h"
#include "../util.h"

/* JOE (found in Wall-E and some more Pixar games) */
VGMSTREAM * init_vgmstream_ps2_joe(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    char filename[260];
    off_t start_offset;
	uint8_t	testBuffer[0x10];
	off_t	loopStart = 0;
	off_t	loopEnd = 0;
	off_t	readOffset = 0;
	size_t	fileLength;
    int loop_flag;
	int channel_count;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("joe",filename_extension(filename))) goto fail;

    /* check header */
    if (read_32bitBE(0x0C,streamFile) != 0xCCCCCCCC)
        goto fail;

    loop_flag = 1;
    channel_count = 2;

	/* build the VGMSTREAM */
    vgmstream = allocate_vgmstream(channel_count,loop_flag);
    if (!vgmstream) goto fail;

	/* fill in the vital statistics */
    start_offset = 0x4020;
	vgmstream->channels = channel_count;
    vgmstream->sample_rate = read_32bitLE(0x0,streamFile);
    vgmstream->coding_type = coding_PSX;
    vgmstream->num_samples = (get_streamfile_size(streamFile)-start_offset)*28/16/channel_count;
		
	
	fileLength = get_streamfile_size(streamFile);
		
	do {
		
		readOffset+=(off_t)read_streamfile(testBuffer,readOffset,0x10,streamFile); 
		
		/* Loop Start */
		if(testBuffer[0x01]==0x06) {
			if(loopStart == 0) loopStart = readOffset-0x10;
			/* break; */
		}
		/* Loop End */
		if(testBuffer[0x01]==0x03) {
			if(loopEnd == 0) loopEnd = readOffset-0x10;
			/* break; */
		}

	} while (streamFile->get_offset(streamFile)<(int32_t)fileLength);
	
	if(loopStart == 0) {
		loop_flag = 0;
		vgmstream->num_samples = read_32bitLE(0x4,streamFile)*28/16/channel_count;
	} else {
		loop_flag = 1;
		vgmstream->loop_start_sample = (loopStart-start_offset-0x20)*28/16/channel_count;
        	vgmstream->loop_end_sample = (loopEnd-start_offset+0x20)*28/16/channel_count;
    	}

	vgmstream->layout_type = layout_interleave;
	vgmstream->interleave_block_size = 0x10;
	vgmstream->meta_type = meta_PS2_JOE;

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
