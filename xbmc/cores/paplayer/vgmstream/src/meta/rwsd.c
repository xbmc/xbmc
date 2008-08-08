#include "meta.h"
#include "../coding/coding.h"
#include "../util.h"

/* RWSD is quite similar to BRSTM, but can contain several streams.
 * Still, some games use it for single streams. We only support the
 * single stream form here */
VGMSTREAM * init_vgmstream_rwsd(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    char filename[260];

    coding_t coding_type;

    off_t wave_offset;
    size_t wave_length;
    int codec_number;
    int channel_count;
    int loop_flag;

    off_t start_offset;
    size_t stream_size;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("rwsd",filename_extension(filename))) goto fail;

    /* check header */
    if ((uint32_t)read_32bitBE(0,streamFile)!=0x52575344 || /* "RWSD" */
            (uint32_t)read_32bitBE(4,streamFile)!=0xFEFF0102)
        goto fail;

    /* ideally we would look through the chunk list for a WAVE chunk,
     * but it's always in the same order */
    /* get WAVE offset, check */
    wave_offset = read_32bitBE(0x18,streamFile);
    if ((uint32_t)read_32bitBE(wave_offset,streamFile)!=0x57415645) /* "WAVE" */
        goto fail;
    /* get WAVE size, check */
    wave_length = read_32bitBE(0x1c,streamFile);
    if (read_32bitBE(wave_offset+4,streamFile)!=wave_length)
        goto fail;

    /* check wave count */
    if (read_32bitBE(wave_offset+8,streamFile) != 1)
        goto fail; /* only support 1 */

    /* get type details */
    codec_number = read_8bit(wave_offset+0x10,streamFile);
    loop_flag = read_8bit(wave_offset+0x11,streamFile);
    channel_count = read_8bit(wave_offset+0x12,streamFile);

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
    vgmstream->num_samples = dsp_nibbles_to_samples(read_32bitBE(wave_offset+0x1c,streamFile));
    vgmstream->sample_rate = (uint16_t)read_16bitBE(wave_offset+0x14,streamFile);
    /* channels and loop flag are set by allocate_vgmstream */
    vgmstream->loop_start_sample = dsp_nibbles_to_samples(read_32bitBE(wave_offset+0x18,streamFile));
    vgmstream->loop_end_sample = vgmstream->num_samples;

    vgmstream->coding_type = coding_type;
    vgmstream->layout_type = layout_none;

    vgmstream->meta_type = meta_RWSD;

    if (vgmstream->coding_type == coding_NGC_DSP) {
        off_t coef_offset;
        int i,j;

        coef_offset=0x6c;

        for (j=0;j<vgmstream->channels;j++) {
            for (i=0;i<16;i++) {
                vgmstream->ch[j].adpcm_coef[i]=read_16bitBE(wave_offset+coef_offset+j*0x30+i*2,streamFile);
            }
        }
    }

    start_offset = read_32bitBE(8,streamFile);
    stream_size = read_32bitBE(wave_offset+0x50,streamFile);

    /* open the file for reading by each channel */
    {
        int i;
        for (i=0;i<channel_count;i++) {
            vgmstream->ch[i].streamfile = streamFile->open(streamFile,filename,
                    0x1000);

            if (!vgmstream->ch[i].streamfile) goto fail;

            vgmstream->ch[i].channel_start_offset=
                vgmstream->ch[i].offset=
                start_offset + i*stream_size;
        }
    }

    return vgmstream;

    /* clean up anything we may have opened */
fail:
    if (vgmstream) close_vgmstream(vgmstream);
    return NULL;
}
