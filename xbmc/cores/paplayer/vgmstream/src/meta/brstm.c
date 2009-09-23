#include "meta.h"
#include "../util.h"

VGMSTREAM * init_vgmstream_brstm(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    char filename[260];

    coding_t coding_type;

    off_t head_offset;
    int codec_number;
    int channel_count;
    int loop_flag;
    /* Certain Super Paper Mario tracks have a 44.1KHz sample rate in the
     * header, but they should be played at 22.05KHz. We will make this
     * correction if we see a file with a .brstmspm extension. */
    int spm_flag = 0;
    /* Trauma Center Second Opinion has an odd, semi-corrupt header */
    int atlus_shrunken_head = 0;

    off_t start_offset;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("brstm",filename_extension(filename))) {
        if (strcasecmp("brstmspm",filename_extension(filename))) goto fail;
        else spm_flag = 1;
    }

    /* check header */
    if ((uint32_t)read_32bitBE(0,streamFile)!=0x5253544D) /* "RSTM" */
        goto fail;
    if ((uint32_t)read_32bitBE(4,streamFile)!=0xFEFF0100)
    {
        if ((uint32_t)read_32bitBE(4,streamFile)!=0xFEFF0001)
            goto fail;
        else
            atlus_shrunken_head = 1;
    }

    /* get head offset, check */
    head_offset = read_32bitBE(0x10,streamFile);
    if (atlus_shrunken_head)
    {
        /* the HEAD chunk is where we would expect to find the offset of that
         * chunk... */

        if ((uint32_t)head_offset!=0x48454144 || read_32bitBE(0x14,streamFile) != 8)
            goto fail;

        head_offset = -8;   /* most of the normal Nintendo RSTM offsets work
                               with this assumption */
    }
    else
    {
        if ((uint32_t)read_32bitBE(head_offset,streamFile)!=0x48454144) /* "HEAD" */
            goto fail;
    }

    /* check type details */
    codec_number = read_8bit(head_offset+0x20,streamFile);
    loop_flag = read_8bit(head_offset+0x21,streamFile);
    channel_count = read_8bit(head_offset+0x22,streamFile);

    switch (codec_number) {
        case 0:
            coding_type = coding_PCM8;
            break;
        case 1:
            coding_type = coding_PCM16BE;
            break;
        case 2:
            coding_type = coding_NGC_DSP;
            break;
        default:
            goto fail;
    }

    if (channel_count < 1) goto fail;

    /* build the VGMSTREAM */

    vgmstream = allocate_vgmstream(channel_count,loop_flag);
    if (!vgmstream) goto fail;

    /* fill in the vital statistics */
    vgmstream->num_samples = read_32bitBE(head_offset+0x2c,streamFile);
    vgmstream->sample_rate = (uint16_t)read_16bitBE(head_offset+0x24,streamFile);
    /* channels and loop flag are set by allocate_vgmstream */
    vgmstream->loop_start_sample = read_32bitBE(head_offset+0x28,streamFile);
    vgmstream->loop_end_sample = vgmstream->num_samples;

    vgmstream->coding_type = coding_type;
    if (channel_count==1)
        vgmstream->layout_type = layout_none;
    else
        vgmstream->layout_type = layout_interleave_shortblock;
    vgmstream->meta_type = meta_RSTM;
    if (atlus_shrunken_head)
        vgmstream->meta_type = meta_RSTM_shrunken;

    if (spm_flag&& vgmstream->sample_rate == 44100) {
        vgmstream->meta_type = meta_RSTM_SPM;
        vgmstream->sample_rate = 22050;
    }

    vgmstream->interleave_block_size = read_32bitBE(head_offset+0x38,streamFile);
    vgmstream->interleave_smallblock_size = read_32bitBE(head_offset+0x48,streamFile);

    if (vgmstream->coding_type == coding_NGC_DSP) {
        off_t coef_offset;
        off_t coef_offset1;
        off_t coef_offset2;
        int i,j;
        int coef_spacing = 0x38;

        if (atlus_shrunken_head)
        {
            coef_offset = 0x50;
            coef_spacing = 0x30;
        }
        else
        {
            coef_offset1=read_32bitBE(head_offset+0x1c,streamFile);
            coef_offset2=read_32bitBE(head_offset+0x10+coef_offset1,streamFile);
            coef_offset=coef_offset2+0x10;
        }

        for (j=0;j<vgmstream->channels;j++) {
            for (i=0;i<16;i++) {
                vgmstream->ch[j].adpcm_coef[i]=read_16bitBE(head_offset+coef_offset+j*coef_spacing+i*2,streamFile);
            }
        }
    }

    start_offset = read_32bitBE(head_offset+0x30,streamFile);

    /* open the file for reading by each channel */
    {
        int i;
        for (i=0;i<channel_count;i++) {
            if (vgmstream->layout_type==layout_interleave_shortblock)
                vgmstream->ch[i].streamfile = streamFile->open(streamFile,filename,
                    vgmstream->interleave_block_size);
            else
                vgmstream->ch[i].streamfile = streamFile->open(streamFile,filename,
                    0x1000);

            if (!vgmstream->ch[i].streamfile) goto fail;

            vgmstream->ch[i].channel_start_offset=
                vgmstream->ch[i].offset=
                start_offset + i*vgmstream->interleave_block_size;
        }
    }

    return vgmstream;

    /* clean up anything we may have opened */
fail:
    if (vgmstream) close_vgmstream(vgmstream);
    return NULL;
}
