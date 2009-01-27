#include "meta.h"
#include "../util.h"

/* GBTS : Pop'n'Music 9 Bgm File */

VGMSTREAM * init_vgmstream_ps2_gbts(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    char filename[260];

    int loop_flag=0;
	int channel_count;
    off_t start_offset;
	off_t loopStart = 0;
	off_t loopEnd = 0;
	size_t filelength;

	int i;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("gbts",filename_extension(filename))) goto fail;

	/* check loop */
	start_offset=0x801;

	filelength = get_streamfile_size(streamFile);
	do {
		// Loop Start ...
		if(read_8bit(start_offset,streamFile)==0x06) {
			if(loopStart==0) loopStart = start_offset-0x801;
		}

		// Loop End ...
		if(read_8bit(start_offset,streamFile)==0x03) {
			if(loopEnd==0) loopEnd = start_offset-0x801-0x10;
		}

		start_offset+=0x10;

	} while (start_offset<(int32_t)filelength);

	loop_flag = (loopEnd!=0);
    channel_count=read_32bitLE(0x1C,streamFile);
    
	/* build the VGMSTREAM */
    vgmstream = allocate_vgmstream(channel_count,loop_flag);
    if (!vgmstream) goto fail;

	/* fill in the vital statistics */
	vgmstream->channels = channel_count;
    vgmstream->sample_rate = read_32bitLE(0x18,streamFile);;

	/* Check for Compression Scheme */
	vgmstream->coding_type = coding_PSX;
    vgmstream->num_samples = read_32bitLE(0x0C,streamFile)/16*28/vgmstream->channels;
	vgmstream->interleave_block_size = 0x10;

	/* Get loop point values */
	if(vgmstream->loop_flag) {
		vgmstream->loop_start_sample = (loopStart/(vgmstream->interleave_block_size)*vgmstream->interleave_block_size)/16*28;
		vgmstream->loop_start_sample += (loopStart%vgmstream->interleave_block_size)/16*28;
		vgmstream->loop_start_sample /=vgmstream->channels;
		vgmstream->loop_end_sample = (loopEnd/(vgmstream->interleave_block_size)*vgmstream->interleave_block_size)/16*28;
		vgmstream->loop_end_sample += (loopEnd%vgmstream->interleave_block_size)/16*28;
		vgmstream->loop_end_sample /=vgmstream->channels;
	}

    vgmstream->layout_type = layout_interleave;
    vgmstream->meta_type = meta_PS2_GBTS;

	start_offset = (off_t)0x800;

    /* open the file for reading by each channel */
    {
        for (i=0;i<channel_count;i++) {
            vgmstream->ch[i].streamfile = streamFile->open(streamFile,filename,vgmstream->interleave_block_size);

            if (!vgmstream->ch[i].streamfile) goto fail;

            vgmstream->ch[i].channel_start_offset=
                vgmstream->ch[i].offset=
                (off_t)(start_offset+vgmstream->interleave_block_size*i);
        }
    }

    return vgmstream;

    /* clean up anything we may have opened */
fail:
    if (vgmstream) close_vgmstream(vgmstream);
    return NULL;
}
