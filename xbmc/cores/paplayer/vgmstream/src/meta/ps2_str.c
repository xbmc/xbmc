#include "meta.h"
#include "../util.h"

/* STR

   2008-05-19 - Fastelbja : Test version ...
*/

VGMSTREAM * init_vgmstream_ps2_str(STREAMFILE *streamFile) {
    
	VGMSTREAM * vgmstream = NULL;
	STREAMFILE * infileSTH = NULL;
	char filename[260];

	char * filenameSTH = NULL;

	int i, channel_count, loop_flag;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("str",filename_extension(filename))) goto fail;

	/* check for .MIH file */
	filenameSTH=(char *)malloc(strlen(filename)+1);

	if (!filenameSTH) goto fail;

	strcpy(filenameSTH,filename);
	strcpy(filenameSTH+strlen(filenameSTH)-3,"STH");

	infileSTH = streamFile->open(streamFile,filenameSTH,STREAMFILE_DEFAULT_BUFFER_SIZE);

	/* STH File is necessary, so we can't confuse those file */
	/* with others .STR file as it is a very common extension */
	if (!infileSTH) goto fail;

	if((read_32bitLE(0x2C,infileSTH)==0x07) ||
	   (read_32bitLE(0x2C,infileSTH)==0x06))
		channel_count=2;
	if(read_32bitLE(0x2C,infileSTH)==0x05)
		channel_count=1;

	loop_flag = read_32bitLE(0x2C,infileSTH) & 0x01;

    /* build the VGMSTREAM */
    vgmstream = allocate_vgmstream(channel_count,loop_flag);
    if (!vgmstream) goto fail;

    /* fill in the vital statistics */
	vgmstream->channels = channel_count;
	vgmstream->sample_rate = read_32bitLE(0x24,infileSTH);

	vgmstream->interleave_block_size=0x4000;

	if(read_32bitLE(0x40,infileSTH)==0x01) 
		vgmstream->interleave_block_size = 0x8000; 

	vgmstream->num_samples=read_32bitLE(0x20,infileSTH);

	vgmstream->coding_type = coding_PSX;
    vgmstream->layout_type = layout_interleave;
    
	vgmstream->meta_type = meta_PS2_STR;

	if(loop_flag) {
		vgmstream->loop_start_sample = 0;
		vgmstream->loop_end_sample = read_32bitLE(0x20,infileSTH);
	}

	close_streamfile(infileSTH); infileSTH=NULL;

    /* open the file for reading by each channel */
    {
        for (i=0;i<channel_count;i++) {
            vgmstream->ch[i].streamfile = streamFile->open(streamFile,filename,vgmstream->interleave_block_size);

            if (!vgmstream->ch[i].streamfile) goto fail;

            vgmstream->ch[i].channel_start_offset=
                vgmstream->ch[i].offset+=(off_t)(vgmstream->interleave_block_size*i);
        }
    }

    return vgmstream;

    /* clean up anything we may have opened */
fail:
    if (infileSTH) close_streamfile(infileSTH);
    if (filenameSTH) {free(filenameSTH); filenameSTH=NULL;}
    if (vgmstream) close_vgmstream(vgmstream);
    return NULL;
}
