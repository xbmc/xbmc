#include "meta.h"
#include "../util.h"

/* a simple PS2 ADPCM format seen in Langrisser 3 */
VGMSTREAM * init_vgmstream_ivb(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    char filename[260];
    off_t start_offset;
    off_t stream_length;

    int loop_flag = 0;
	int channel_count;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("ivb",filename_extension(filename))) goto fail;

    /* check header */
    if (read_32bitBE(0x00,streamFile) != 0x42564949) /* "BVII", probably */
        goto fail;                                   /* supposed to be "IIVB"*/

    loop_flag = 0;
    channel_count = 2;
    
	/* build the VGMSTREAM */
    vgmstream = allocate_vgmstream(channel_count,loop_flag);
    if (!vgmstream) goto fail;

	/* fill in the vital statistics */
	vgmstream->channels = channel_count;
    vgmstream->sample_rate = read_32bitBE(0x8,streamFile);  /* big endian? */
    vgmstream->coding_type = coding_PSX;
    stream_length = read_32bitLE(0x04,streamFile);
    start_offset = 0x10;
    vgmstream->num_samples = stream_length*28/16;

    vgmstream->layout_type = layout_none;
    vgmstream->meta_type = meta_PS2_IVB;

    /* open the file for reading */
    {
        int i;
        for (i=0;i<channel_count;i++) {
            vgmstream->ch[i].streamfile = streamFile->open(streamFile,filename,STREAMFILE_DEFAULT_BUFFER_SIZE);

            if (!vgmstream->ch[i].streamfile) goto fail;

            vgmstream->ch[i].channel_start_offset=
                vgmstream->ch[i].offset=start_offset+stream_length*i;

        }
    }

    return vgmstream;

    /* clean up anything we may have opened */
fail:
    if (vgmstream) close_vgmstream(vgmstream);
    return NULL;
}
