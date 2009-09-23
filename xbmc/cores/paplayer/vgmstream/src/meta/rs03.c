#include "meta.h"
#include "../coding/coding.h"
#include "../util.h"

/* .dsp w/ RS03 header - from Metroid Prime 2 */

VGMSTREAM * init_vgmstream_rs03(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    char filename[260];

    int channel_count;
    int loop_flag;
    off_t start_offset;
    int i;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("dsp",filename_extension(filename))) goto fail;

    /* check header */
    if ((uint32_t)read_32bitBE(0,streamFile)!=0x52530003)   /* "RS03" */
        goto fail;

    channel_count = read_32bitBE(4,streamFile);
    if (channel_count != 1 && channel_count != 2) goto fail;

    /* build the VGMSTREAM */

    loop_flag = read_16bitBE(0x14,streamFile);

    vgmstream = allocate_vgmstream(channel_count,loop_flag);
    if (!vgmstream) goto fail;

    /* fill in the vital statistics */
    vgmstream->num_samples = read_32bitBE(8,streamFile);
    vgmstream->sample_rate = read_32bitBE(0xc,streamFile);

    if (loop_flag) {
        vgmstream->loop_start_sample = read_32bitBE(0x18,streamFile)/8*14;
        vgmstream->loop_end_sample = read_32bitBE(0x1c,streamFile)/8*14;
    }

    start_offset = 0x60;

    vgmstream->coding_type = coding_NGC_DSP;
    if (channel_count == 2) {
        vgmstream->layout_type = layout_interleave_shortblock;
        vgmstream->interleave_block_size = 0x8f00;
        vgmstream->interleave_smallblock_size = (((get_streamfile_size(streamFile)-start_offset)%(0x8f00*2))/2+7)/8*8;
    } else
        vgmstream->layout_type = layout_none;
    vgmstream->meta_type = meta_DSP_RS03;

    for (i=0;i<16;i++)
        vgmstream->ch[0].adpcm_coef[i]=read_16bitBE(0x20+i*2,streamFile);
    if (channel_count==2) {
        for (i=0;i<16;i++)
            vgmstream->ch[1].adpcm_coef[i]=read_16bitBE(0x40+i*2,streamFile);
    }

    /* open the file for reading by each channel */
    {
        int i;
        for (i=0;i<channel_count;i++) {
            vgmstream->ch[i].streamfile = streamFile->open(streamFile,filename,0x8f00);

            if (!vgmstream->ch[i].streamfile) goto fail;

            vgmstream->ch[i].channel_start_offset=
                vgmstream->ch[i].offset=
                start_offset+0x8f00*i;
        }
    }

    return vgmstream;

    /* clean up anything we may have opened */
fail:
    if (vgmstream) close_vgmstream(vgmstream);
    return NULL;
}
