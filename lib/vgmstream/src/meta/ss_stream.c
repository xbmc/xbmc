#include "meta.h"
#include "../util.h"

/* SS_STREAM

   SS_STREAM is format used by various UBI Soft Games

   2008-11-15 - Fastelbja : First version ...
*/

VGMSTREAM * init_vgmstream_ss_stream(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    char filename[260];
    int loop_flag=0;
	int channel_count;
    int i;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("ss7",filename_extension(filename))) goto fail;

	loop_flag = 0;
	channel_count=read_8bit(0x0C,streamFile)+1;
    
	/* build the VGMSTREAM */
    vgmstream = allocate_vgmstream(channel_count,loop_flag);
    if (!vgmstream) goto fail;

	/* fill in the vital statistics */
	vgmstream->channels = channel_count;
	vgmstream->sample_rate = 44100;
	
	if(channel_count==1)
		vgmstream->coding_type = coding_IMA;
	else
		vgmstream->coding_type = coding_EACS_IMA;

    vgmstream->num_samples = (int32_t)((get_streamfile_size(streamFile) -0x44)* 2 / vgmstream->channels);
    vgmstream->layout_type = layout_none;
	
    vgmstream->meta_type = meta_XBOX_WAVM;
	vgmstream->get_high_nibble=0;

    /* open the file for reading by each channel */
    {
        for (i=0;i<channel_count;i++) {
            vgmstream->ch[i].streamfile = streamFile->open(streamFile,filename,36);
            vgmstream->ch[i].offset = 0x44;
			vgmstream->ch[i].adpcm_history1_32=(int32_t)read_16bitLE(0x10+i*4,streamFile);
			vgmstream->ch[i].adpcm_step_index =(int)read_8bit(0x12+i*4,streamFile);
            if (!vgmstream->ch[i].streamfile) goto fail;
        }
    }

    return vgmstream;

    /* clean up anything we may have opened */
fail:
    if (vgmstream) close_vgmstream(vgmstream);
    return NULL;
}
