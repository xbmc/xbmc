#include "meta.h"
#include "../layout/layout.h"
#include "../coding/coding.h"
#include "../util.h"

/* .mus, as seen in Star Wars The Force Unleashed for Wii */
/* Doesn't seem to be working quite right yet, coef table looks odd */
VGMSTREAM * init_vgmstream_wii_mus(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    char filename[260];

    off_t start_offset;
    off_t interleave;

    int i;
    struct {
        uint16_t gain;
        uint16_t initial_ps;
        uint16_t initial_hist1;
        uint16_t initial_hist2;
        uint16_t loop_ps;
        /*
        uint16_t loop_hist1;
        uint16_t loop_hist2;
        */
    } channel[2];

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("mus",filename_extension(filename))) goto fail;

    start_offset = read_32bitBE(0x08,streamFile);
    interleave = read_32bitBE(0x04,streamFile);

    for (i=0;i<2;i++)
    {
        channel[i].gain = read_16bitBE(0x30+i*0x2e,streamFile);
        channel[i].initial_ps = read_16bitBE(0x32+i*0x2e,streamFile);
        channel[i].initial_hist1 = read_16bitBE(0x34+i*0x2e,streamFile);
        channel[i].initial_hist2 = read_16bitBE(0x36+i*0x2e,streamFile);
        channel[i].loop_ps = read_16bitBE(0x38+i*0x2e,streamFile);
    }

    /* check initial predictor/scale */
    if (channel[0].initial_ps != (uint8_t)read_8bit(start_offset,streamFile))
        goto fail;
    if (channel[1].initial_ps != (uint8_t)read_8bit(start_offset+interleave,streamFile))
        goto fail;

    /* check type==0 and gain==0 */
    if (channel[0].gain ||
        channel[1].gain)
        goto fail;

#if 0
    if (ch0_header.loop_flag) {
        off_t loop_off;
        /* check loop predictor/scale */
        loop_off = ch0_header.loop_start_offset/16*8;
        loop_off = (loop_off/interleave*interleave*2) + (loop_off%interleave);
        if (ch0_header.loop_ps != (uint8_t)read_8bit(start_offset+loop_off,streamFile))
            goto fail;
        if (ch1_header.loop_ps != (uint8_t)read_8bit(start_offset+loop_off+interleave,streamFile))
            goto fail;
    }
#endif

    /* build the VGMSTREAM */

    vgmstream = allocate_vgmstream(2,0);
    if (!vgmstream) goto fail;

    /* fill in the vital statistics */
    vgmstream->num_samples = read_32bitBE(0x0,streamFile);
    vgmstream->sample_rate = (uint16_t)read_16bitBE(0x6c,streamFile);

    /* TODO: adjust for interleave? */
#if 0
    vgmstream->loop_start_sample = dsp_nibbles_to_samples(
            ch0_header.loop_start_offset);
    vgmstream->loop_end_sample =  dsp_nibbles_to_samples(
            ch0_header.loop_end_offset)+1;
#endif

    vgmstream->coding_type = coding_NGC_DSP;
    vgmstream->layout_type = layout_interleave;
    vgmstream->interleave_block_size = interleave;
    vgmstream->meta_type = meta_DSP_WII_MUS;

    /* coeffs */
    for (i=0;i<16;i++) {
        vgmstream->ch[0].adpcm_coef[i] = read_16bitBE(0x10 + i*2,streamFile);
        vgmstream->ch[1].adpcm_coef[i] = read_16bitBE(0x3e + i*2,streamFile);
    }
    
    /* initial history */
    /* always 0 that I've ever seen, but for completeness... */
    vgmstream->ch[0].adpcm_history1_16 = channel[0].initial_hist1;
    vgmstream->ch[0].adpcm_history2_16 = channel[0].initial_hist2;
    vgmstream->ch[1].adpcm_history1_16 = channel[1].initial_hist1;
    vgmstream->ch[1].adpcm_history2_16 = channel[1].initial_hist2;

    vgmstream->ch[0].streamfile = streamFile->open(streamFile,filename,interleave);
    if (!vgmstream->ch[0].streamfile) goto fail;
    vgmstream->ch[1].streamfile = streamFile->open(streamFile,filename,interleave);
    if (!vgmstream->ch[1].streamfile) goto fail;

    /* open the file for reading */
    for (i=0;i<2;i++) {
        vgmstream->ch[i].channel_start_offset=
            vgmstream->ch[i].offset=start_offset+i*interleave;
    }

    return vgmstream;

fail:
    /* clean up anything we may have opened */
    if (vgmstream) {
        if (vgmstream->ch[0].streamfile) close_streamfile(vgmstream->ch[0].streamfile);
        close_vgmstream(vgmstream);
    }
    return NULL;
}

