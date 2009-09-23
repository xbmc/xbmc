#include "meta.h"
#include "../util.h"

/* MIC

   PS2 MIC format is an interleaved format found in most of KOEI Games                
   The header always start the long value 0x800 which is the start
   of the BGM datas.

   2008-05-15 - Fastelbja : First version ...
*/

VGMSTREAM * init_vgmstream_ps2_mic(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    char filename[260];

    int loop_flag=0;
    int channel_count;
    int i;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("mic",filename_extension(filename))) goto fail;

    /* check Header */
    if (read_32bitLE(0x00,streamFile) != 0x800)
        goto fail;

    /* check loop */
    loop_flag = (read_32bitLE(0x14,streamFile)!=1);

    channel_count=read_32bitLE(0x08,streamFile);

    /* build the VGMSTREAM */
    vgmstream = allocate_vgmstream(channel_count,loop_flag);
    if (!vgmstream) goto fail;

    /* fill in the vital statistics */
    vgmstream->channels = channel_count;
    vgmstream->sample_rate = read_32bitLE(0x04,streamFile);

    /* Compression Scheme */
    vgmstream->coding_type = coding_PSX;
    vgmstream->num_samples = read_32bitLE(0x10,streamFile)*14*channel_count;

    /* Get loop point values */
    if(vgmstream->loop_flag) {
        vgmstream->loop_start_sample = read_32bitLE(0x14,streamFile)*14*channel_count;
        vgmstream->loop_end_sample = read_32bitLE(0x10,streamFile)*14*channel_count;
    }

    vgmstream->interleave_block_size = read_32bitLE(0x0C,streamFile);
    vgmstream->layout_type = layout_interleave;
    vgmstream->meta_type = meta_PS2_MIC;

    /* open the file for reading by each channel */
    {
        for (i=0;i<channel_count;i++) {
            vgmstream->ch[i].streamfile = streamFile->open(streamFile,filename,vgmstream->interleave_block_size);

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
