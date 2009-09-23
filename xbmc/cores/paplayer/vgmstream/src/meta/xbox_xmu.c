#include "meta.h"
#include "../util.h"

/* XMU

   XMU (found in Alter Echo)
*/

VGMSTREAM * init_vgmstream_xbox_xmu(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    char filename[260];

    int loop_flag=0;
	int channel_count;
    int i;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("xmu",filename_extension(filename))) goto fail;

	if((read_32bitBE(0x00,streamFile)!=0x584D5520) && 
	   (read_32bitBE(0x08,streamFile)!=0x46524D54))
		goto fail;

    /* No Loop found atm */
	loop_flag = read_8bit(0x16,streamFile);;
    
	/* Always stereo files */
	channel_count=read_8bit(0x14,streamFile);
    
	/* build the VGMSTREAM */
    vgmstream = allocate_vgmstream(channel_count,loop_flag);
    if (!vgmstream) goto fail;

	/* fill in the vital statistics */
	vgmstream->channels = channel_count;
    vgmstream->sample_rate = read_32bitLE(0x10,streamFile);

	vgmstream->coding_type = coding_XBOX;
    vgmstream->num_samples = read_32bitLE(0x7FC,streamFile) / 36 * 64 / vgmstream->channels;
    vgmstream->layout_type = layout_none;
    vgmstream->meta_type = meta_XBOX_XMU;

	if(loop_flag) {
		vgmstream->loop_start_sample=0;
		vgmstream->loop_end_sample=vgmstream->num_samples;
	}
	
	/* open the file for reading by each channel */
    {
        for (i=0;i<channel_count;i++) {
            vgmstream->ch[i].streamfile = streamFile->open(streamFile,filename,36);
            vgmstream->ch[i].offset = 0x800;

            if (!vgmstream->ch[i].streamfile) goto fail;
        }
    }

    return vgmstream;

    /* clean up anything we may have opened */
fail:
    if (vgmstream) close_vgmstream(vgmstream);
    return NULL;
}
