#include "meta.h"
#include "../util.h"

/* MUSX */
/* Old MUSX formats, found in Spyro, Ty and other games */
VGMSTREAM * init_vgmstream_musx_v004(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    char filename[260];
    off_t start_offset;
	int musx_type; /* determining the decoder by strings like "PS2_", "GC__" and so on */
	//int musx_version; /* 0x08 provides a "version" byte */
	int loop_flag = 0;
	int channel_count;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("musx",filename_extension(filename))) goto fail;

    /* check header */
    if (read_32bitBE(0x00,streamFile) != 0x4D555358) /* "MUSX" */
		goto fail;
	if (read_32bitBE(0x08,streamFile) != 0x04000000) /* "0x04000000" */
		goto fail;
        
	/* This is tricky, the header changes it's layout if the file is unlooped */
    loop_flag = (read_32bitLE(0x840,streamFile)!=0xFFFFFFFF);
    channel_count = 2;

	/* build the VGMSTREAM */
    vgmstream = allocate_vgmstream(channel_count,loop_flag);
    if (!vgmstream) goto fail;


	/* fill in the vital statistics */	
	musx_type=(read_32bitBE(0x10,streamFile));

	switch (musx_type) {
	case 0x5053325F: /* PS2_ */
			start_offset = read_32bitLE(0x28,streamFile);
			vgmstream->channels = channel_count;
			vgmstream->sample_rate = 32000;
			vgmstream->coding_type = coding_PSX;
			vgmstream->num_samples = (read_32bitLE(0x0C,streamFile))*28/16/channel_count;
			vgmstream->layout_type = layout_interleave;
			vgmstream->interleave_block_size = 0x80;
			vgmstream->meta_type = meta_MUSX_V004;
		if (loop_flag) {
			vgmstream->loop_start_sample = (read_32bitLE(0x890,streamFile))*28/16/channel_count;
			vgmstream->loop_end_sample = (read_32bitLE(0x89C,streamFile))*28/16/channel_count;
		}
	break;
	/* seems to not work for Spyro, maybe i find other games for testing
	case 0x58425F5F: XB__
			start_offset = read_32bitLE(0x28,streamFile);
			vgmstream->channels = channel_count;
			vgmstream->sample_rate = 32000;
			vgmstream->coding_type = coding_XBOX;
			vgmstream->num_samples = (read_32bitLE(0x0C,streamFile))*64/36/channel_count;
			vgmstream->layout_type = layout_none;
			vgmstream->meta_type = meta_MUSX_V004;
		if (loop_flag) {
			vgmstream->loop_start_sample = (read_32bitLE(0x890,streamFile))*64/36/channel_count;
			vgmstream->loop_end_sample = (read_32bitLE(0x89C,streamFile))*64/36/channel_count;
		}
	break;
	*/
		default:
			goto fail;

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



/* MUSX */
/* Old MUSX formats, found in Spyro, Ty and other games */
VGMSTREAM * init_vgmstream_musx_v006(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    char filename[260];
    off_t start_offset;
	int musx_type; /* determining the decoder by strings like "PS2_", "GC__" and so on */
	//int musx_version; /* 0x08 provides a "version" byte */
	int loop_flag = 0;
	int channel_count;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("musx",filename_extension(filename))) goto fail;

    /* check header */
    if (read_32bitBE(0x00,streamFile) != 0x4D555358) /* "MUSX" */
		goto fail;
	if (read_32bitBE(0x08,streamFile) != 0x06000000) /* "0x06000000" */
		goto fail;

    loop_flag = (read_32bitLE(0x840,streamFile)!=0xFFFFFFFF);
    channel_count = 2;

	/* build the VGMSTREAM */
    vgmstream = allocate_vgmstream(channel_count,loop_flag);
    if (!vgmstream) goto fail;

	/* fill in the vital statistics */	
	musx_type=(read_32bitBE(0x10,streamFile));

	switch (musx_type) {
	case 0x5053325F: /* PS2_ */
			start_offset = read_32bitLE(0x28,streamFile);
			vgmstream->channels = channel_count;
			vgmstream->sample_rate = 32000;
			vgmstream->coding_type = coding_PSX;
			vgmstream->num_samples = (read_32bitLE(0x0C,streamFile))*28/16/channel_count;
			vgmstream->layout_type = layout_interleave;
			vgmstream->interleave_block_size = 0x80;
			vgmstream->meta_type = meta_MUSX_V006;
		if (loop_flag) {
			vgmstream->loop_start_sample = (read_32bitLE(0x890,streamFile))*28/16/channel_count;
			vgmstream->loop_end_sample = (read_32bitLE(0x89C,streamFile))*28/16/channel_count;
		}
	break;
		default:
			goto fail;

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


/* MUSX */
/* New MUSX formats, found in Quantum of Solace, The Mummy 3, possibly more */
VGMSTREAM * init_vgmstream_musx_v010(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    char filename[260];
    off_t start_offset;
	int musx_type; /* determining the decoder by strings like "PS2_", "GC__" and so on */
	//int musx_version; /* 0x08 provides a "version" byte */
	int loop_flag = 0;
	int channel_count;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("musx",filename_extension(filename))) goto fail;

    /* check header */
    if (read_32bitBE(0x00,streamFile) != 0x4D555358) /* "MUSX" */
		goto fail;
	if (read_32bitBE(0x08,streamFile) != 0x0A000000) /* "0x0A000000" */
		goto fail;

	loop_flag = (read_32bitLE(0x34,streamFile)!=0x00000000);
    channel_count = 2;
    
	/* build the VGMSTREAM */
    vgmstream = allocate_vgmstream(channel_count,loop_flag);
    if (!vgmstream) goto fail;
	
	/* fill in the vital statistics */	
	musx_type=(read_32bitBE(0x10,streamFile));

	switch (musx_type) {
	case 0x5053325F: /* PS2_ */
			start_offset = 0x800;
			vgmstream->channels = channel_count;
			vgmstream->sample_rate = 32000;
			vgmstream->coding_type = coding_PSX;
			vgmstream->num_samples = read_32bitLE(0x40,streamFile);
			vgmstream->layout_type = layout_interleave;
			vgmstream->interleave_block_size = 0x80;
			vgmstream->meta_type = meta_MUSX_V010;
		if (loop_flag) {
			vgmstream->loop_start_sample = read_32bitLE(0x44,streamFile);
			vgmstream->loop_end_sample = read_32bitLE(0x40,streamFile);
		}
	break;
		default:
			goto fail;

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

/* MUSX */
/* Old MUSX format, this one handles "Sphinx and the cursed Mummy", it's different from the other formats */
VGMSTREAM * init_vgmstream_musx_v201(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    char filename[260];
    off_t start_offset;
	//int musx_version; /* 0x08 provides a "version" byte */
	int loop_flag;
	int channel_count;
	int loop_detect;
	int loop_offsets;
	
    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("musx",filename_extension(filename))) goto fail;

    /* check header */
    if (read_32bitBE(0x00,streamFile) != 0x4D555358) /* "MUSX" */
		goto fail;
	if (read_32bitBE(0x08,streamFile) != 0xC9000000) /* "0xC9000000" */
		goto fail;

    channel_count = 2;

	loop_detect = read_32bitBE(0x800,streamFile);
	switch (loop_detect) {
		case 0x02000000:
		loop_offsets = 0x8E0;
	break;
		case 0x03000000:
		loop_offsets = 0x880;
	break;
		case 0x04000000:
		loop_offsets = 0x8B4;
	break;
		case 0x05000000:
		loop_offsets = 0x8E8;
	break;
		case 0x06000000:
		loop_offsets = 0x91C;
	break;
		default:
			goto fail;
	}

	loop_flag = (read_32bitLE(loop_offsets+0x04,streamFile) !=0x00000000);

	/* build the VGMSTREAM */
    vgmstream = allocate_vgmstream(channel_count,loop_flag);
    if (!vgmstream) goto fail;

	/* fill in the vital statistics */	
		start_offset = read_32bitLE(0x18,streamFile);
		vgmstream->channels = channel_count;
		vgmstream->sample_rate = 32000;
		vgmstream->coding_type = coding_PSX;
		vgmstream->num_samples = read_32bitLE(loop_offsets,streamFile)*28/16/channel_count;
		if (loop_flag) {
			vgmstream->loop_start_sample = read_32bitLE(loop_offsets+0x10,streamFile)*28/16/channel_count;
			vgmstream->loop_end_sample = read_32bitLE(loop_offsets,streamFile)*28/16/channel_count;
		}
		vgmstream->layout_type = layout_interleave;
		vgmstream->interleave_block_size = 0x80;
		vgmstream->meta_type = meta_MUSX_V201;	
	
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
