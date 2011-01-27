#include "meta.h"
#include "../util.h"

/* WVS

   WVS (found in Metal Arms - Glitch in the System)
*/

VGMSTREAM * init_vgmstream_xbox_wvs(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    char filename[260];

    int loop_flag=0;
	int channel_count;
    int i;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("wvs",filename_extension(filename))) goto fail;

	if((read_16bitLE(0x0C,streamFile)!=0x69) && 
	   (read_16bitLE(0x08,streamFile)!=0x4400) && 
	   (read_32bitLE(0x0,streamFile)!=get_streamfile_size(streamFile)+0x20))
		goto fail;

    /* Loop seems to be set if offset(0x0A) == 0x472C */
	loop_flag = (read_16bitLE(0x0A,streamFile)==0x472C);
    
	/* Always stereo files */
	channel_count=read_16bitLE(0x0E,streamFile);
    
	/* build the VGMSTREAM */
    vgmstream = allocate_vgmstream(channel_count,loop_flag);
    if (!vgmstream) goto fail;

	/* fill in the vital statistics */
	/* allways 2 channels @ 44100 Hz */
	vgmstream->channels = channel_count;
    vgmstream->sample_rate = read_32bitLE(0x10,streamFile);

	vgmstream->coding_type = coding_XBOX;
    vgmstream->num_samples = read_32bitLE(0,streamFile) / 36 * 64 / vgmstream->channels;
    vgmstream->layout_type = layout_none;
    vgmstream->meta_type = meta_XBOX_WVS;

	if(loop_flag) {
		vgmstream->loop_start_sample=0;
		vgmstream->loop_end_sample=vgmstream->num_samples;
	}

    /* open the file for reading by each channel */
    {
        for (i=0;i<channel_count;i++) {
            vgmstream->ch[i].streamfile = streamFile->open(streamFile,filename,36);
            vgmstream->ch[i].offset = 0x20;

            if (!vgmstream->ch[i].streamfile) goto fail;
        }
    }

    return vgmstream;

    /* clean up anything we may have opened */
fail:
    if (vgmstream) close_vgmstream(vgmstream);
    return NULL;
}
