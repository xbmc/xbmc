#include "meta.h"
#include "../util.h"

/* SPT+SPT

   2008-11-27 - manakoAT : First try for splitted files...
*/

VGMSTREAM * init_vgmstream_spt_spd(STREAMFILE *streamFile) {

	VGMSTREAM * vgmstream = NULL;
    STREAMFILE * streamFileSPT = NULL;
    char filename[260];
	char filenameSPT[260];
	
	int i;
	int channel_count;
	int loop_flag;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("spd",filename_extension(filename))) goto fail;


	strcpy(filenameSPT,filename);
	strcpy(filenameSPT+strlen(filenameSPT)-3,"spt");

	streamFileSPT = streamFile->open(streamFile,filenameSPT,STREAMFILE_DEFAULT_BUFFER_SIZE);


	if (!streamFileSPT) goto fail;
	
	channel_count = read_32bitBE(0x00,streamFileSPT);
	loop_flag = read_32bitBE(0x04,streamFileSPT);

    /* build the VGMSTREAM */
    vgmstream = allocate_vgmstream(channel_count,loop_flag);
    if (!vgmstream) goto fail;

    /* fill in the vital statistics */
	vgmstream->channels = channel_count;
	vgmstream->sample_rate = read_32bitBE(0x08,streamFileSPT);
	vgmstream->num_samples=read_32bitBE(0x14,streamFileSPT)*14/16/channel_count;
	vgmstream->coding_type = coding_NGC_DSP;
	
	if(loop_flag) {
		vgmstream->loop_start_sample = 0;
		vgmstream->loop_end_sample = read_32bitBE(0x14,streamFileSPT)*14/16/channel_count;
	}	

	if (channel_count == 1) {
		vgmstream->layout_type = layout_none;
	} else if (channel_count == 2) {
		vgmstream->layout_type = layout_interleave;
		vgmstream->interleave_block_size=0x4000; /* Unknown now, never seen 2ch files in psd+spt */
	}



    vgmstream->meta_type = meta_SPT_SPD;
	
    /* open the file for reading */
    {
        for (i=0;i<channel_count;i++) {
			/* Not sure, i'll put a fake value here for now */
            vgmstream->ch[i].streamfile = streamFile->open(streamFile,filename,0x8000);
            vgmstream->ch[i].offset = 0;

            if (!vgmstream->ch[i].streamfile) goto fail;
        }
    }


    
	if (vgmstream->coding_type == coding_NGC_DSP) {
        int i;
        for (i=0;i<16;i++) {
            vgmstream->ch[0].adpcm_coef[i] = read_16bitBE(0x20+i*2,streamFileSPT);
        }
        if (vgmstream->channels == 2) {
            for (i=0;i<16;i++) {
                vgmstream->ch[1].adpcm_coef[i] = read_16bitBE(0x40+i*2,streamFileSPT);
            }
        }
    }


	close_streamfile(streamFileSPT); streamFileSPT=NULL;
    return vgmstream;

    /* clean up anything we may have opened */
fail:
    if (streamFileSPT) close_streamfile(streamFileSPT);
    if (vgmstream) close_vgmstream(vgmstream);
    return NULL;
}
