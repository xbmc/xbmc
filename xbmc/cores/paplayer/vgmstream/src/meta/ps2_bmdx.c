#include "meta.h"
#include "../util.h"

VGMSTREAM * init_vgmstream_ps2_bmdx(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    char filename[260];
    int loop_flag=0;
    int channel_count;
    off_t start_offset;
    int i;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("bmdx",filename_extension(filename))) goto fail;

    /* check NPSF Header */
    if (read_32bitBE(0x00,streamFile) != 0x01006408 ||
            read_32bitBE(0x04,streamFile) != 0)
        goto fail;

    /* check loop */
    loop_flag = (read_32bitLE(0x10,streamFile)!=0);
    channel_count=read_32bitLE(0x1C,streamFile);
    
    /* build the VGMSTREAM */
    vgmstream = allocate_vgmstream(channel_count,loop_flag);
    if (!vgmstream) goto fail;


    /* fill in the vital statistics */
    vgmstream->channels = channel_count;
    vgmstream->sample_rate = read_32bitLE(0x18,streamFile);

    /* Check for Compression Scheme */
    if (read_32bitLE(0x20,streamFile) == 1)
        vgmstream->coding_type = coding_invert_PSX;
    else
        vgmstream->coding_type = coding_PSX;
    vgmstream->num_samples = read_32bitLE(0x0c,streamFile)*28/16/channel_count;

    /* Get loop point values */
    if(vgmstream->loop_flag) {
        vgmstream->loop_start_sample = read_32bitLE(0x10,streamFile)*28/16/channel_count;
        vgmstream->loop_end_sample = vgmstream->num_samples;
    }

    if (channel_count == 1) {
        vgmstream->layout_type = layout_none;
    } else if (channel_count > 1) {
        vgmstream->interleave_block_size = read_32bitLE(0x24,streamFile);
        vgmstream->layout_type = layout_interleave;
    }

    vgmstream->meta_type = meta_PS2_BMDX;

    start_offset = read_32bitLE(0x08,streamFile);

    if (vgmstream->coding_type == coding_invert_PSX)
    {
        uint8_t xor = read_8bit(start_offset,streamFile);
        uint8_t add = (~(uint8_t)read_8bit(start_offset+2,streamFile))+1;
        int c;
        for (c=0;c<channel_count;c++) {
            vgmstream->ch[c].bmdx_xor = xor;
            vgmstream->ch[c].bmdx_add = add;
        }
    }

    /* open the file for reading by each channel */
    {
        for (i=0;i<channel_count;i++) {
            if (!vgmstream->ch[0].streamfile) {
                vgmstream->ch[0].streamfile = streamFile->open(streamFile,filename,0x8000);
            }
            vgmstream->ch[i].streamfile = vgmstream->ch[0].streamfile;

            if (!vgmstream->ch[i].streamfile) goto fail;

            vgmstream->ch[i].channel_start_offset=
                vgmstream->ch[i].offset=
                (off_t)(start_offset+vgmstream->interleave_block_size*i);
        }
    }
    

    return vgmstream;

    /* clean up anything we may have opened */
fail:
    if (vgmstream) close_vgmstream(vgmstream);
    return NULL;
}
