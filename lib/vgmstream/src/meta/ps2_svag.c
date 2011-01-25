#include "meta.h"
#include "../util.h"

/* SVAG

   PS2 SVAG format is an interleaved format found in many konami Games                
   The header start with a Svag id and have the sentence :
		"ALL RIGHTS RESERVED.KONAMITYO Sound Design Dept. "
	or  "ALL RIGHTS RESERVED.KCE-Tokyo Sound Design Dept. "

   2008-05-13 - Fastelbja : First version ...
							Thx to HCS for his awesome work on shortblock interleave
*/

VGMSTREAM * init_vgmstream_ps2_svag(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    char filename[260];

    int loop_flag=0;
    int channel_count;
    int i;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("svag",filename_extension(filename))) goto fail;

    /* check SVAG Header */
    if (read_32bitBE(0x00,streamFile) != 0x53766167)
        goto fail;

    /* check loop */
    loop_flag = (read_32bitLE(0x14,streamFile)==1);

    channel_count=read_16bitLE(0x0C,streamFile);

    /* build the VGMSTREAM */
    vgmstream = allocate_vgmstream(channel_count,loop_flag);
    if (!vgmstream) goto fail;

    /* fill in the vital statistics */
    vgmstream->channels = read_16bitLE(0x0C,streamFile);
    vgmstream->sample_rate = read_32bitLE(0x08,streamFile);

    /* Compression Scheme */
    vgmstream->coding_type = coding_PSX;
    vgmstream->num_samples = read_32bitLE(0x04,streamFile)/16*28/vgmstream->channels;

    /* Get loop point values */
    if(vgmstream->loop_flag) {
        vgmstream->loop_start_sample = read_32bitLE(0x18,streamFile)/16*28;
        vgmstream->loop_end_sample = read_32bitLE(0x04,streamFile)/16*28/vgmstream->channels;
    }

    vgmstream->interleave_block_size = read_32bitLE(0x10,streamFile);
    if (channel_count > 1) {
        vgmstream->interleave_smallblock_size = (read_32bitLE(0x04,streamFile)%(vgmstream->channels*vgmstream->interleave_block_size))/vgmstream->channels;
        vgmstream->layout_type = layout_interleave_shortblock;
    } else {
        vgmstream->layout_type = layout_none;
    }
    vgmstream->meta_type = meta_PS2_SVAG;

    /* open the file for reading by each channel */
    {
        for (i=0;i<channel_count;i++) {
            if (channel_count > 1)
                vgmstream->ch[i].streamfile = streamFile->open(streamFile,filename,vgmstream->interleave_block_size);
            else
                vgmstream->ch[i].streamfile = streamFile->open(streamFile,filename,STREAMFILE_DEFAULT_BUFFER_SIZE);

            if (!vgmstream->ch[i].streamfile) goto fail;

            vgmstream->ch[i].channel_start_offset=
                vgmstream->ch[i].offset=
                (off_t)(0x800+vgmstream->interleave_block_size*i);
        }
    }

    return vgmstream;

    /* clean up anything we may have opened */
fail:
    if (vgmstream) close_vgmstream(vgmstream);
    return NULL;
}
