#include "meta.h"
#include "../layout/layout.h"
#include "../util.h"

/* Gurumin .de2 */
/* A ways into the file we have a fake RIFF header wrapping MS ADPCM */

VGMSTREAM * init_vgmstream_de2(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    char filename[260];

    off_t riff_off;
    int channel_count;
    int sample_count;
    int sample_rate;
    off_t start_offset;

    int loop_flag = 0;
    uint32_t data_size;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("de2",filename_extension(filename))) goto fail;

    /* still not sure what this is for, but consistently 0xb */
    if (read_32bitLE(0x04,streamFile)!=0xb) goto fail;

    /* legitimate! really! */
    riff_off = 0x10 +
        (read_32bitLE(0x0c,streamFile) ^ read_32bitLE(0x04,streamFile));

    /* check header */
    if ((uint32_t)read_32bitBE(riff_off+0,streamFile)!=0x52494646) /* "RIFF" */
        goto fail;
    /* check for WAVE form */
    if ((uint32_t)read_32bitBE(riff_off+8,streamFile)!=0x57415645) /* "WAVE" */
        goto fail;
    /* check for "fmt " */
    if ((uint32_t)read_32bitBE(riff_off+12,streamFile)!=0x666d7420) /* "fmt " */
        goto fail;
    /* check for "data" */
    if ((uint32_t)read_32bitBE(riff_off+0x24,streamFile)!=0x64617461) /* "data" */
        goto fail;
    /* check for bad fmt chunk size */
    if (read_32bitLE(riff_off+0x10,streamFile)!=0x12) goto fail;

    sample_rate = read_32bitLE(riff_off+0x18,streamFile);

    channel_count = read_16bitLE(riff_off+0x16,streamFile);
    if (channel_count != 2) goto fail;

    /* PCM */
    if (read_16bitLE(riff_off+0x14,streamFile) != 1) goto fail;

    /* 16-bit */
    if (read_16bitLE(riff_off+0x20,streamFile) != 4 ||
        read_16bitLE(riff_off+0x22,streamFile) != 16) goto fail;

    start_offset = riff_off + 0x2c;
    data_size = read_32bitLE(riff_off+0x28,streamFile);

    sample_count = data_size/2/channel_count;

    /* build the VGMSTREAM */

    vgmstream = allocate_vgmstream(channel_count,loop_flag);
    if (!vgmstream) goto fail;

    /* fill in the vital statistics */
    vgmstream->num_samples = sample_count;
    vgmstream->sample_rate = sample_rate;

    vgmstream->coding_type = coding_MSADPCM;
    vgmstream->layout_type = layout_de2_blocked;
    vgmstream->interleave_block_size = 0x800;

    vgmstream->meta_type = meta_DE2;

    /* open the file, set up each channel */
    {
        int i;

        vgmstream->ch[0].streamfile = streamFile->open(streamFile,filename,
                STREAMFILE_DEFAULT_BUFFER_SIZE);
        if (!vgmstream->ch[0].streamfile) goto fail;

        for (i=0;i<channel_count;i++) {
            vgmstream->ch[i].streamfile = vgmstream->ch[0].streamfile;
        }
    }

    /* start me up */
    de2_block_update(start_offset,vgmstream);

    return vgmstream;

    /* clean up anything we may have opened */
fail:
    if (vgmstream) close_vgmstream(vgmstream);
    return NULL;
}
