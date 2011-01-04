#include "meta.h"
#include "../util.h"

/* XWAV

   XWAV use the common RIFF/WAVE format with Codec ID = 0x0069
   It has been renamed to xwav to avoid vgmstream to handle all RIFF/WAV format
   known extensions : XWAV

   2008-05-24 - Fastelbja : First version ...
*/

VGMSTREAM * init_vgmstream_xbox_xwav(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    char filename[260];

    int loop_flag=0;
	int channel_count;
    off_t start_offset;
    int i,j=0;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("xwav",filename_extension(filename))) goto fail;

	/* Check for headers */
	if(!((read_32bitBE(0x00,streamFile)==0x52494646) &&
		 (read_32bitBE(0x08,streamFile)==0x57415645) &&
		 (read_32bitBE(0x0C,streamFile)==0x666D7420) && 
		 (read_16bitLE(0x14,streamFile)==0x0069)))
		 goto fail;

    /* No loop on wavm */
	if(read_32bitBE(0x28,streamFile)==0x77736D70) 
		loop_flag = 1;
	else
		loop_flag = 0;
    
	/* Always stereo files */
	channel_count=read_16bitLE(0x16,streamFile);
    
	/* build the VGMSTREAM */
    vgmstream = allocate_vgmstream(channel_count,loop_flag);
    if (!vgmstream) goto fail;

	/* hack for loop wave found on Dynasty warriors */
	if(loop_flag) {
		vgmstream->loop_start_sample = read_32bitLE(0x4C,streamFile);
		vgmstream->loop_end_sample = vgmstream->loop_start_sample + read_32bitLE(0x50,streamFile);
	}

	/* fill in the vital statistics */
	vgmstream->channels = channel_count;
    vgmstream->sample_rate = read_32bitLE(0x18,streamFile);

	/* search for "data" */
	start_offset=0x1C;

	do {
		if(read_32bitBE(start_offset,streamFile)==0x64617461)
			break;
		start_offset+=4;
	} while (start_offset<(off_t)get_streamfile_size(streamFile));

	if(start_offset>=(off_t)get_streamfile_size(streamFile))
		goto fail;

	start_offset+=4;

	vgmstream->coding_type = coding_XBOX;
    vgmstream->num_samples = read_32bitLE(start_offset,streamFile) / 36 * 64 / vgmstream->channels;
    vgmstream->layout_type = layout_none;
	
    vgmstream->meta_type = meta_XBOX_RIFF;

    /* open the file for reading by each channel */

    {
		if(channel_count>2) {
			for (i=0;i<channel_count;i++,j++) {
				if((j&2) && (i!=0)) {
					j=0;
					start_offset+=36*2;
				}

				vgmstream->ch[i].streamfile = streamFile->open(streamFile,filename,36);
				vgmstream->ch[i].offset = start_offset+4;

				if (!vgmstream->ch[i].streamfile) goto fail;
			}
		} else {
			for (i=0;i<channel_count;i++) {
				vgmstream->ch[i].streamfile = streamFile->open(streamFile,filename,36);
				vgmstream->ch[i].offset = start_offset+4;

				if (!vgmstream->ch[i].streamfile) goto fail;
			}
		}
    }

    return vgmstream;

    /* clean up anything we may have opened */
fail:
    if (vgmstream) close_vgmstream(vgmstream);
    return NULL;
}
