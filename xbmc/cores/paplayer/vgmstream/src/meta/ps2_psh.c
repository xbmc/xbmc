#include "meta.h"
#include "../util.h"

/* PSH (from Dawn of Mana - Seiken Densetsu 4) */
/* probably Square Vag Stream */
VGMSTREAM * init_vgmstream_ps2_psh(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    char filename[260];
    off_t start_offset;
	uint8_t	testBuffer[0x10];
	off_t	loopEnd = 0;
	off_t	readOffset = 0;
	size_t	fileLength;

	int loop_flag;
	int channel_count;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("psh",filename_extension(filename))) goto fail;

    /* check header */
    if (read_16bitBE(0x02,streamFile) != 0x6400)
        goto fail;

    loop_flag = (read_16bitLE(0x06,streamFile)!=0);
    channel_count = 2;
    
	/* build the VGMSTREAM */
    vgmstream = allocate_vgmstream(channel_count,loop_flag);
    if (!vgmstream) goto fail;

	/* fill in the vital statistics */
	start_offset = 0;
	vgmstream->channels = channel_count;
	vgmstream->sample_rate = (uint16_t)read_16bitLE(0x08,streamFile);
	vgmstream->coding_type = coding_PSX;
	vgmstream->num_samples = (uint16_t)read_16bitLE(0x0C,streamFile)*0x800*28/16/channel_count;
    
	// loop end is set by the loop marker which we need to find ...
	// there's some extra data on unloop files, so we calculate
	// the sample count with loop marker on this files
	fileLength = get_streamfile_size(streamFile);
	do {
		readOffset+=(off_t)read_streamfile(testBuffer,readOffset,0x10,streamFile); 
		
		// Loop End ...
		if(testBuffer[0x01]==0x03) {
			if(loopEnd==0) loopEnd = readOffset-0x10;
			break;
		}
	} while (streamFile->get_offset(streamFile)<(int32_t)fileLength);
	
	if(loopEnd!=0)
		vgmstream->num_samples = loopEnd*28/16/channel_count;

	if(loop_flag) {
		vgmstream->loop_start_sample =
			((uint16_t)read_16bitLE(0x06,streamFile)-0x8000)*0x400*28/16;
		vgmstream->loop_end_sample=vgmstream->num_samples;
	}

    vgmstream->layout_type = layout_interleave;
    vgmstream->interleave_block_size = 0x800;
    vgmstream->meta_type = meta_PS2_PSH;

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
