#include "meta.h"
#include "../layout/layout.h"
#include "../util.h"

/* GSP+GSB - 2008-11-28 - manakoAT
Super Swing Golf 1 & 2 (WII) */
VGMSTREAM * init_vgmstream_gsp_gsb(STREAMFILE *streamFile) {

	VGMSTREAM * vgmstream = NULL;
    STREAMFILE * streamFileGSP = NULL;
    char filename[260];
	char filenameGSP[260];
    off_t start_offset;
	int channel_count;
	int loop_flag;
	int header_len;
	off_t coef1_start;
	off_t coef2_start;
	int dsp_blocks;
	
    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("gsb",filename_extension(filename))) goto fail;


	strcpy(filenameGSP,filename);
	strcpy(filenameGSP+strlen(filenameGSP)-3,"gsp");

	streamFileGSP = streamFile->open(streamFile,filenameGSP,STREAMFILE_DEFAULT_BUFFER_SIZE);
	if (!streamFileGSP) goto fail;

    /* check header */
    if (read_32bitBE(0x00,streamFileGSP) != 0x47534E44)	/* "GSND" */
		goto fail;

	channel_count = (uint16_t)read_16bitBE(0x3A,streamFileGSP);
	loop_flag = (read_32bitBE(0x64,streamFileGSP) !=0xFFFFFFFF);
	header_len = read_32bitBE(0x1C,streamFileGSP);
	
	coef1_start = header_len-0x4C;
	coef2_start = header_len-0x1C;
	dsp_blocks = read_32bitBE(header_len-0x5C,streamFileGSP);
	
    /* build the VGMSTREAM */
    vgmstream = allocate_vgmstream(channel_count,loop_flag);
    if (!vgmstream) goto fail;

    /* fill in the vital statistics */
	vgmstream->channels = channel_count;
	vgmstream->sample_rate = read_32bitBE(0x34,streamFileGSP);
	vgmstream->coding_type = coding_NGC_DSP;
	if(loop_flag) {
		vgmstream->loop_start_sample = read_32bitBE(0x64,streamFileGSP);
		vgmstream->loop_end_sample = read_32bitBE(0x68,streamFileGSP);
	}	

	if (channel_count == 1) {
		vgmstream->layout_type = layout_gsb_blocked;
	} else if (channel_count > 1) {
		vgmstream->layout_type = layout_gsb_blocked;
		vgmstream->interleave_block_size = read_32bitBE(header_len-0x64,streamFileGSP);
	}

    vgmstream->meta_type = meta_GSP_GSB;
	
    /* open the file for reading */
    vgmstream->ch[0].streamfile = streamFile->open(streamFile,filename,STREAMFILE_DEFAULT_BUFFER_SIZE);
    if (!vgmstream->ch[0].streamfile) goto fail;
	    vgmstream->ch[0].channel_start_offset=0;
    if (channel_count == 2) {
        vgmstream->ch[1].streamfile = streamFile->open(streamFile,filename,STREAMFILE_DEFAULT_BUFFER_SIZE);
        if (!vgmstream->ch[1].streamfile) goto fail;
        vgmstream->ch[1].channel_start_offset=vgmstream->interleave_block_size;
    }

	if (vgmstream->coding_type == coding_NGC_DSP) {
        int i;
        for (i=0;i<16;i++) {
            vgmstream->ch[0].adpcm_coef[i] = read_16bitBE(coef1_start+i*2,streamFileGSP);
        }
        if (vgmstream->channels == 2) {
            for (i=0;i<16;i++) {
                vgmstream->ch[1].adpcm_coef[i] = read_16bitBE(coef2_start+i*2,streamFileGSP);
            }
        }
    }

	/* Calc num_samples */
	start_offset = 0x0;
	gsb_block_update(start_offset,vgmstream);
	vgmstream->num_samples=0;

	do {
	
	vgmstream->num_samples += vgmstream->current_block_size*14/8;
		gsb_block_update(vgmstream->next_block_offset,vgmstream);
	} while (vgmstream->next_block_offset<get_streamfile_size(streamFile));

	gsb_block_update(start_offset,vgmstream);

	close_streamfile(streamFileGSP); streamFileGSP=NULL;

    return vgmstream;


    /* clean up anything we may have opened */
fail:
    if (streamFileGSP) close_streamfile(streamFileGSP);
    if (vgmstream) close_vgmstream(vgmstream);
    return NULL;
}
