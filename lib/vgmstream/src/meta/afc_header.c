#include "meta.h"
#include "../util.h"

VGMSTREAM * init_vgmstream_afc(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    char filename[260];

    int loop_flag;
    const int channel_count = 2;    /* .afc seems to be stereo only */

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("afc",filename_extension(filename))) goto fail;

    /* don't grab AIFF-C with .afc extension */
    if ((uint32_t)read_32bitBE(0x0,streamFile)==0x464F524D) /* FORM */
        goto fail;

    /* we will get a sample rate, that's as close to checking as I think
     * we can get */

    /* build the VGMSTREAM */

    loop_flag = read_32bitBE(0x10,streamFile);

    vgmstream = allocate_vgmstream(channel_count,loop_flag);
    if (!vgmstream) goto fail;

    /* fill in the vital statistics */
    vgmstream->num_samples = read_32bitBE(0x04,streamFile);
    vgmstream->sample_rate = (uint16_t)read_16bitBE(0x08,streamFile);
    /* channels and loop flag are set by allocate_vgmstream */
    vgmstream->loop_start_sample = read_32bitBE(0x14,streamFile);
    vgmstream->loop_end_sample = vgmstream->num_samples;

    vgmstream->coding_type = coding_NGC_AFC;
    vgmstream->layout_type = layout_interleave;
    vgmstream->meta_type = meta_AFC;

    /* frame-level interleave (9 bytes) */
    vgmstream->interleave_block_size = 9;

    /* open the file for reading by each channel */
    {
        STREAMFILE *chstreamfile;
        int i;

        /* both channels use same buffer, as interleave is so small */
        chstreamfile = streamFile->open(streamFile,filename,9*channel_count*0x100);
        if (!chstreamfile) goto fail;

        for (i=0;i<channel_count;i++) {
            vgmstream->ch[i].streamfile = chstreamfile;

            vgmstream->ch[i].channel_start_offset=
                vgmstream->ch[i].offset=
                0x20 + i*vgmstream->interleave_block_size;
        }
    }

    return vgmstream;

    /* clean up anything we may have opened */
fail:
    if (vgmstream) close_vgmstream(vgmstream);
    return NULL;
}
