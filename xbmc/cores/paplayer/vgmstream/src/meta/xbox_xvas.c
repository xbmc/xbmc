#include "meta.h"
#include "../layout/layout.h"
#include "../util.h"

/* XVAS

   XVAS (found in TMNT 2 & TMNT 3))
*/

VGMSTREAM * init_vgmstream_xbox_xvas(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    char filename[260];

    int loop_flag=0;
	int channel_count;
    int i;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("xvas",filename_extension(filename))) goto fail;

	if((read_32bitLE(0x00,streamFile)!=0x69) && 
	   (read_32bitLE(0x08,streamFile)!=0x48))
		goto fail;

    /* No Loop found atm */
	loop_flag = (read_32bitLE(0x14,streamFile)==read_32bitLE(0x24,streamFile));
    
	/* Always stereo files */
	channel_count=read_32bitLE(0x04,streamFile);
    
	/* build the VGMSTREAM */
    vgmstream = allocate_vgmstream(channel_count,loop_flag);
    if (!vgmstream) goto fail;

	/* fill in the vital statistics */
	vgmstream->channels = channel_count;
    vgmstream->sample_rate = read_32bitLE(0x0c,streamFile);

	vgmstream->coding_type = coding_XBOX;
    vgmstream->num_samples = read_32bitLE(0x24,streamFile);
	vgmstream->num_samples -= ((vgmstream->num_samples/0x20000)*0x20);
	vgmstream->num_samples = vgmstream->num_samples / 36 * 64 / vgmstream->channels;

    vgmstream->layout_type = layout_xvas_blocked;
    vgmstream->meta_type = meta_XBOX_XVAS;

	if(loop_flag) {
		vgmstream->loop_start_sample = read_32bitLE(0x10,streamFile);
		vgmstream->loop_start_sample -= ((vgmstream->loop_start_sample/0x20000)*0x20);
		vgmstream->loop_start_sample = vgmstream->loop_start_sample / 36 * 64 / vgmstream->channels;
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

	xvas_block_update(0x800,vgmstream);
    return vgmstream;

    /* clean up anything we may have opened */
fail:
    if (vgmstream) close_vgmstream(vgmstream);
    return NULL;
}
