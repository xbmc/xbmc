#include "meta.h"
#include "../util.h"

/* STR -ASR (from Donkey Kong Jet Race) */
VGMSTREAM * init_vgmstream_str_asr(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    char filename[260];
    off_t start_offset;

    int loop_flag = 0;
	int channel_count;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("str",filename_extension(filename)) &&	/* PCM Files */
		strcasecmp("asr",filename_extension(filename)))		/* DSP Files */
	goto fail;
		
    /* check header */
    if (read_32bitBE(0x00,streamFile) != 0x4B4E4F4E &&			/* "KNON" */
		read_32bitBE(0x04,streamFile) != 0x00000000 &&			/* "0x0" */
		read_32bitBE(0x08,streamFile) != 0x57494920) goto fail;	/* "WII\0x20" */


    loop_flag = (read_32bitBE(0x44,streamFile)!=0);
    channel_count = 2;
	/* build the VGMSTREAM */
    vgmstream = allocate_vgmstream(channel_count,loop_flag);
    if (!vgmstream) goto fail;

	/* fill in the vital statistics */
    start_offset = 0x800;
	vgmstream->channels = channel_count;
    vgmstream->sample_rate = read_32bitBE(0x40,streamFile);
    
	switch (read_32bitBE(0x20,streamFile)) {
		case 0x4B415354: /* KAST - DSP encoding */
		vgmstream->coding_type = coding_NGC_DSP;
		vgmstream->num_samples = (read_32bitBE(0x3C,streamFile))*14/8/channel_count;
    if (loop_flag) {
        vgmstream->loop_start_sample = (read_32bitBE(0x44,streamFile))*14/8/channel_count;
        vgmstream->loop_end_sample = (read_32bitBE(0x48,streamFile))*14/8/channel_count;
		}
	    vgmstream->interleave_block_size = 0x10;
	break;
		case 0x4B505354: /* KPST - PCM encoding */
		vgmstream->coding_type = coding_PCM16BE;
		vgmstream->num_samples = (read_32bitBE(0x3C,streamFile))/2/channel_count;
    if (loop_flag) {
        vgmstream->loop_start_sample = (read_32bitBE(0x44,streamFile))/2/channel_count;
        vgmstream->loop_end_sample = (read_32bitBE(0x48,streamFile))/2/channel_count;
    }
    vgmstream->interleave_block_size = 0x10;
	break;
		default:
			goto fail;
	}

	/* Interleave and Layout settings */
    vgmstream->layout_type = layout_interleave;
    vgmstream->meta_type = meta_STR_ASR;

	    if (vgmstream->coding_type == coding_NGC_DSP) {
        int i;
        for (i=0;i<16;i++) {
            vgmstream->ch[0].adpcm_coef[i] = read_16bitBE(0x8C+i*2,streamFile);
        }
        if (vgmstream->channels) {
            for (i=0;i<16;i++) {
                vgmstream->ch[1].adpcm_coef[i] = read_16bitBE(0xEC+i*2,streamFile);
            }
        }
    }

    /* open the file for reading */
    {
        int i;
        STREAMFILE * file;
        file = streamFile->open(streamFile,filename,STREAMFILE_DEFAULT_BUFFER_SIZE);
        if (!file) goto fail;
        for (i=0;i<channel_count;i++) {
            vgmstream->ch[i].streamfile = file;

            vgmstream->ch[i].channel_start_offset=
                vgmstream->ch[i].offset=start_offset+
                vgmstream->interleave_block_size*i;

        }
    }

    return vgmstream;

    /* clean up anything we may have opened */
fail:
    if (vgmstream) close_vgmstream(vgmstream);
    return NULL;
}
