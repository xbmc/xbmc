#include "meta.h"
#include "../util.h"

/*
ISH+ISD
*/

VGMSTREAM * init_vgmstream_ish_isd(STREAMFILE *streamFile) {

	VGMSTREAM * vgmstream = NULL;
    STREAMFILE * streamFileISH = NULL;
    char filename[260];
	char filenameISH[260];
	int i;
	int channel_count;
	int loop_flag;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("isd",filename_extension(filename))) goto fail;

	strcpy(filenameISH,filename);
	strcpy(filenameISH+strlen(filenameISH)-3,"ish");

	streamFileISH = streamFile->open(streamFile,filenameISH,STREAMFILE_DEFAULT_BUFFER_SIZE);
	if (!streamFileISH) goto fail;

    /* check header */
    if (read_32bitBE(0x00,streamFileISH) != 0x495F5346) /* "I_SF" */
        goto fail;
	
	channel_count = read_32bitBE(0x14,streamFileISH);
	loop_flag = read_32bitBE(0x20,streamFileISH);

    /* build the VGMSTREAM */
    vgmstream = allocate_vgmstream(channel_count,loop_flag);
    if (!vgmstream) goto fail;

    /* fill in the vital statistics */
	vgmstream->channels = channel_count;
	vgmstream->sample_rate = read_32bitBE(0x08,streamFileISH);
	vgmstream->num_samples=read_32bitBE(0x0C,streamFileISH);
	vgmstream->coding_type = coding_NGC_DSP;
	if(loop_flag) {
		vgmstream->loop_start_sample = read_32bitBE(0x20,streamFileISH)*14/8/channel_count;
		vgmstream->loop_end_sample = read_32bitBE(0x24,streamFileISH)*14/8/channel_count;
	}	

	if (channel_count == 1) {
		vgmstream->layout_type = layout_none;
	} else if (channel_count == 2) {
		vgmstream->layout_type = layout_interleave;
		vgmstream->interleave_block_size = read_32bitBE(0x18,streamFileISH);
	}

    vgmstream->meta_type = meta_ISH_ISD;
	
    /* open the file for reading */
    {
        for (i=0;i<channel_count;i++) {
            vgmstream->ch[i].streamfile = streamFile->open(streamFile,filename,vgmstream->interleave_block_size);
            vgmstream->ch[i].offset = 0;

            if (!vgmstream->ch[i].streamfile) goto fail;
        }
    }
    
	if (vgmstream->coding_type == coding_NGC_DSP) {
        int i;
        for (i=0;i<16;i++) {
            vgmstream->ch[0].adpcm_coef[i] = read_16bitBE(0x40+i*2,streamFileISH);
        }
        if (vgmstream->channels == 2) {
            for (i=0;i<16;i++) {
                vgmstream->ch[1].adpcm_coef[i] = read_16bitBE(0x80+i*2,streamFileISH);
            }
        }
    }

	close_streamfile(streamFileISH); streamFileISH=NULL;
	
    return vgmstream;

    /* clean up anything we may have opened */
fail:
    if (streamFileISH) close_streamfile(streamFileISH);
    if (vgmstream) close_vgmstream(vgmstream);
    return NULL;
}
