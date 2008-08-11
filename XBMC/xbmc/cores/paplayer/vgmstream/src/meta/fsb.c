#include "meta.h"
#include "../util.h"

/* FSB  - FlatOut (XBOX), Guitar Hero III (WII), FlatOut 1 & 2 (PS2) */
VGMSTREAM * init_vgmstream_fsb(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    char filename[260];
    off_t start_offset;

	/* int fsb3_included_files; */
	int fsb3_headerlen = 0x18;
	int fsb3_format;
	int loop_flag = 0;
	int channel_count;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("fsb",filename_extension(filename))) goto fail;

    /* check header */
    if (read_32bitBE(0x00,streamFile) != 0x46534233) /* "FSB3" */
        goto fail;
 
	/* "Check if the FSB is used as
	conatiner or as single file" */
	if (read_32bitBE(0x04,streamFile) != 0x01000000)
		goto fail;
    


	if (read_32bitBE(0x48,streamFile) == 0x02000806) {
        loop_flag = 1;
    } else {
        loop_flag = 0; /* (read_32bitLE(0x08,streamFile)!=0); */
    }
    
	/* Channel check
	if (read_16bitLE(0x56,streamFile) == 2) {
        channel_count = 2;
    } else {
        goto fail;
    }
	*/
	
	
	channel_count = read_16bitLE(0x56,streamFile);


	/* build the VGMSTREAM */
    vgmstream = allocate_vgmstream(channel_count,loop_flag);
    if (!vgmstream) goto fail;

	/* This will be tricky ;o) */
	fsb3_format = read_32bitBE(0x48,streamFile);
	switch (fsb3_format) {
		case 0x40008800: /* PS2 (Agent Hugo, Flat Out 2) */
		case 0x41008800: /* PS2 (Flat Out) */
		case 0x42008800: /* PS2 (Jackass - The Game) */
		case 0x01008804: /* PS2 (Cold Fear) */
		vgmstream->coding_type = coding_PSX;
		vgmstream->layout_type = layout_interleave;
		vgmstream->interleave_block_size = 0x10;
		vgmstream->num_samples = (read_32bitLE(0x0C,streamFile))*28/16/channel_count;
    if (loop_flag) {
        vgmstream->loop_start_sample = read_32bitLE(0x40,streamFile);
        vgmstream->loop_end_sample = (read_32bitLE(0x0C,streamFile))*28/16/channel_count;
    }
	break;
		case 0x02000806: /* WII (Metroid Prime 3) */
		case 0x01000806: /* WII (Metroid Prime 3) */
		case 0x40000802: /* WII (WWE Smackdown Vs. Raw 2008) */
		vgmstream->coding_type = coding_NGC_DSP;
		vgmstream->layout_type = layout_interleave_byte;
        vgmstream->interleave_block_size = 2;
		vgmstream->num_samples = (read_32bitLE(0x0C,streamFile))*14/8/channel_count;
	if (loop_flag) {
        vgmstream->loop_start_sample = read_32bitLE(0x40,streamFile);
        vgmstream->loop_end_sample = (read_32bitLE(0x0C,streamFile))*14/8/channel_count;
    }
	break;
		case 0x40004020: /* WII (Guitar Hero III), uses Xbox-ish IMA */
		case 0x400040A0: /* WII (Guitar Hero III), uses Xbox-ish IMA */
		case 0x41004800: /* XBOX (FlatOut, Rainbow Six - Lockdown) */
		case 0x01004804: /* XBOX (Cold Fear) <- maybe IMA??? */
		vgmstream->coding_type = coding_XBOX;
		vgmstream->layout_type = layout_none;
		vgmstream->num_samples = read_32bitLE(0x0C,streamFile)*64/36/channel_count;
	if (loop_flag) {
        vgmstream->loop_start_sample = read_32bitLE(0x40,streamFile);
        vgmstream->loop_end_sample = read_32bitLE(0x0C,streamFile)*64/36/channel_count;
    }
	break;
		default:
			goto fail;
	}
	/* fill in the vital statistics */
    start_offset = (read_32bitLE(0x08,streamFile))+fsb3_headerlen;
	vgmstream->channels = channel_count;
    vgmstream->sample_rate = read_32bitLE(0x4C,streamFile);
    vgmstream->meta_type = meta_FSB3;

    if (vgmstream->coding_type == coding_NGC_DSP) {
        int i;
        for (i=0;i<16;i++) {
            vgmstream->ch[0].adpcm_coef[i] = read_16bitBE(0x68+i*2,streamFile);
        }
        if (vgmstream->channels) {
            for (i=0;i<16;i++) {
                vgmstream->ch[1].adpcm_coef[i] = read_16bitBE(0x96+i*2,streamFile);
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

            
            if (vgmstream->coding_type == coding_XBOX) {
                /* xbox interleaving is a little odd */
                vgmstream->ch[i].channel_start_offset=start_offset;
            } else {
                vgmstream->ch[i].channel_start_offset=
                    start_offset+vgmstream->interleave_block_size*i;
            }
            vgmstream->ch[i].offset = vgmstream->ch[i].channel_start_offset;

        }
    }

    return vgmstream;

    /* clean up anything we may have opened */
fail:
    if (vgmstream) close_vgmstream(vgmstream);
    return NULL;
}
