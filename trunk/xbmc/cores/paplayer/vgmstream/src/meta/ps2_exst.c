#include "meta.h"
#include "../util.h"

/* EXST

   PS2 INT format is an interleaved format found in Shadow of the Colossus                
   The header start with a EXST id.
   The headers and bgm datas was separated in the game, and joined in order
   to add support for vgmstream

   The interleave value is allways 0x400
   known extensions : .STS

   2008-05-13 - Fastelbja : First version ...
*/

VGMSTREAM * init_vgmstream_ps2_exst(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    char filename[260];

    int loop_flag=0;
    int channel_count;
    int i;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("sts",filename_extension(filename))) goto fail;

    /* check EXST Header */
    if (read_32bitBE(0x00,streamFile) != 0x45585354)
        goto fail;

    /* check loop */
    loop_flag = (read_32bitLE(0x0C,streamFile)==1);

    channel_count=read_16bitLE(0x06,streamFile);

    /* build the VGMSTREAM */
    vgmstream = allocate_vgmstream(channel_count,loop_flag);
    if (!vgmstream) goto fail;

    /* fill in the vital statistics */
    vgmstream->channels = read_16bitLE(0x06,streamFile);
    vgmstream->sample_rate = read_32bitLE(0x08,streamFile);

    /* Compression Scheme */
    vgmstream->coding_type = coding_PSX;
    vgmstream->num_samples = (read_32bitLE(0x14,streamFile)*0x400)/16*28;

    /* Get loop point values */
    if(vgmstream->loop_flag) {
        vgmstream->loop_start_sample = (read_32bitLE(0x10,streamFile)*0x400)/16*28;
        vgmstream->loop_end_sample = (read_32bitLE(0x14,streamFile)*0x400)/16*28;
    }

    vgmstream->interleave_block_size = 0x400;
    vgmstream->layout_type = layout_interleave;
    vgmstream->meta_type = meta_PS2_EXST;

    /* open the file for reading by each channel */
    {
        for (i=0;i<channel_count;i++) {
            vgmstream->ch[i].streamfile = streamFile->open(streamFile,filename,0x8000);

            if (!vgmstream->ch[i].streamfile) goto fail;

            vgmstream->ch[i].channel_start_offset=
                vgmstream->ch[i].offset=
                (off_t)(0x78+vgmstream->interleave_block_size*i);
        }
    }

    return vgmstream;

    /* clean up anything we may have opened */
fail:
    if (vgmstream) close_vgmstream(vgmstream);
    return NULL;
}
