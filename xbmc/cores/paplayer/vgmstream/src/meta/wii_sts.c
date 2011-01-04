#include "meta.h"
#include "../util.h"

/* STS

   STS (found in Shikigami No Shiro 3)
   Don't confuse with ps2 .STS (EXST) format, this one is for WII
*/

VGMSTREAM * init_vgmstream_wii_sts(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    char filename[260];

    int loop_flag=0;
	int channel_count;
    int i,j;
	off_t	start_offset;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("sts",filename_extension(filename))) goto fail;

	/* First bytes contain the size of the file (-4) */
	if(read_32bitBE(0x0,streamFile)!=get_streamfile_size(streamFile)-4)
		goto fail;

	loop_flag = (read_32bitLE(0x4C,streamFile)!=0xFFFFFFFF);
	channel_count=read_8bit(0x8,streamFile)+1;
    
	/* build the VGMSTREAM */
    vgmstream = allocate_vgmstream(channel_count,loop_flag);
    if (!vgmstream) goto fail;

	/* fill in the vital statistics */
	vgmstream->channels = channel_count;
    vgmstream->sample_rate = read_32bitBE(0x0A,streamFile);
	vgmstream->coding_type = coding_NGC_DSP;

	if(vgmstream->channels==1) 
		vgmstream->num_samples = (read_32bitBE(0x0,streamFile)+4-0x70)/8*14;
	else
		vgmstream->num_samples = (read_32bitBE(0x0,streamFile)+4-0x50-0x26)/8*14/2;

    vgmstream->layout_type = layout_none;
    vgmstream->meta_type = meta_STS_WII;

	if(loop_flag) {
		vgmstream->loop_start_sample=read_32bitLE(0x24,streamFile);
		vgmstream->loop_end_sample=vgmstream->num_samples;
	}

	/* setting coef tables */
	if(vgmstream->channels==1) 
		start_offset = 0x70;
	else
		start_offset = 0x50;

	// First channel
	for(j=0;j<16;j++) {
		vgmstream->ch[0].adpcm_coef[j]=read_16bitBE(0x1E + (j*2),streamFile);
	}
	
	// Second channel ?
	if(vgmstream->channels==2) {
		start_offset+=read_32bitBE(0x1a,streamFile);
		for(j=0;j<16;j++) {
			vgmstream->ch[1].adpcm_coef[j]=read_16bitBE(start_offset+(j*2),streamFile);
		}
	}

    /* open the file for reading by each channel */
    {
        for (i=0;i<channel_count;i++) {
            vgmstream->ch[i].streamfile = streamFile->open(streamFile,filename,36);
            vgmstream->ch[i].offset = 0x50+(i*(start_offset+0x26-0x50));

            if (!vgmstream->ch[i].streamfile) goto fail;
        }
    }

    return vgmstream;

    /* clean up anything we may have opened */
fail:
    if (vgmstream) close_vgmstream(vgmstream);
    return NULL;
}
