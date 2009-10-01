#include "meta.h"
#include "../coding/coding.h"
#include "../util.h"

/* .dsp w/ Cstr header, seen in Star Fox Assault and Donkey Konga */

VGMSTREAM * init_vgmstream_Cstr(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    char filename[260];

    int loop_flag;
    off_t start_offset;
    off_t first_data;
    off_t loop_offset;
    size_t interleave;
    int loop_adjust;
    int double_loop_end = 0;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("dsp",filename_extension(filename))) goto fail;

    /* check header */
    if ((uint32_t)read_32bitBE(0,streamFile)!=0x43737472)   /* "Cstr" */
        goto fail;
#ifdef DEBUG
    fprintf(stderr,"header ok\n");
#endif

    if (read_8bit(0x1b,streamFile)==1) {
        /* mono version, much simpler to handle */
        /* Only seen in R Racing Evolution radio sfx */

        start_offset = 0x80;
        loop_flag = read_16bitBE(0x2c,streamFile);

        /* check initial predictor/scale */
        if (read_16bitBE(0x5e,streamFile) != (uint8_t)read_8bit(start_offset,streamFile))
            goto fail;

        /* check type==0 and gain==0 */
        if (read_16bitBE(0x2e,streamFile) || read_16bitBE(0x5c,streamFile))
            goto fail;

        loop_offset = start_offset+read_32bitBE(0x10,streamFile);
        if (loop_flag) {
            if (read_16bitBE(0x64,streamFile) != (uint8_t)read_8bit(loop_offset,streamFile)) goto fail;
        }

        /* build the VGMSTREAM */

        vgmstream = allocate_vgmstream(1,loop_flag);
        if (!vgmstream) goto fail;

        /* fill in the vital statistics */
        vgmstream->sample_rate = read_32bitBE(0x28,streamFile);
        vgmstream->num_samples = read_32bitBE(0x20,streamFile);

        if (loop_flag) {
        vgmstream->loop_start_sample = dsp_nibbles_to_samples(
                read_32bitBE(0x30,streamFile));
        vgmstream->loop_end_sample =  dsp_nibbles_to_samples(
                read_32bitBE(0x34,streamFile))+1;
        }

        vgmstream->coding_type = coding_NGC_DSP;
        vgmstream->layout_type = layout_none;
        vgmstream->meta_type = meta_DSP_CSTR;

        {
            int i;
            for (i=0;i<16;i++)
                vgmstream->ch[0].adpcm_coef[i]=read_16bitBE(0x3c+i*2,streamFile);
        }

        /* open the file for reading by each channel */
        vgmstream->ch[0].streamfile = streamFile->open(streamFile,filename,STREAMFILE_DEFAULT_BUFFER_SIZE);

        if (!vgmstream->ch[0].streamfile) goto fail;

        vgmstream->ch[0].channel_start_offset=
            vgmstream->ch[0].offset=
            start_offset;

        return vgmstream;
    }   /* end mono */

    interleave = read_16bitBE(0x06,streamFile);
    start_offset = 0xe0; 
    first_data = start_offset+read_32bitBE(0x0c,streamFile);
    loop_flag = read_16bitBE(0x2c,streamFile);

    if (!loop_flag) {
        /* Nonlooped tracks seem to follow no discernable pattern
         * with where they actually start.
         * But! with the magic of initial p/s redundancy, we can guess.
         */
        while (first_data<start_offset+0x800 &&
                (read_16bitBE(0x5e,streamFile) != (uint8_t)read_8bit(first_data,streamFile) ||
                read_16bitBE(0xbe,streamFile) != (uint8_t)read_8bit(first_data+interleave,streamFile)))
            first_data+=8;
#ifdef DEBUG
        fprintf(stderr,"guessed first_data at %#x\n",first_data);
#endif
    }

    /* check initial predictor/scale */
    if (read_16bitBE(0x5e,streamFile) != (uint8_t)read_8bit(first_data,streamFile))
        goto fail;
    if (read_16bitBE(0xbe,streamFile) != (uint8_t)read_8bit(first_data+interleave,streamFile))
        goto fail;

#ifdef DEBUG
    fprintf(stderr,"p/s ok\n");
#endif

    /* check type==0 and gain==0 */
    if (read_16bitBE(0x2e,streamFile) || read_16bitBE(0x5c,streamFile))
        goto fail;
    if (read_16bitBE(0x8e,streamFile) || read_16bitBE(0xbc,streamFile))
        goto fail;

#ifdef DEBUG
    fprintf(stderr,"type & gain ok\n");
#endif

    /* check for loop flag agreement */
    if (read_16bitBE(0x2c,streamFile) != read_16bitBE(0x8c,streamFile))
        goto fail;

#ifdef DEBUG
    fprintf(stderr,"loop flags agree\n");
#endif

    loop_offset = start_offset+read_32bitBE(0x10,streamFile)*2;
    if (loop_flag) {
        int loops_ok=0;
        /* check loop predictor/scale */
        /* some fuzz allowed */
        for (loop_adjust=0;loop_adjust>=-0x10;loop_adjust-=8) {
#ifdef DEBUG
            fprintf(stderr,"looking for loop p/s at %#x,%#x\n",loop_offset-interleave+loop_adjust,loop_offset+loop_adjust);
#endif
            if (read_16bitBE(0x64,streamFile) == (uint8_t)read_8bit(loop_offset-interleave+loop_adjust,streamFile) &&
                    read_16bitBE(0xc4,streamFile) == (uint8_t)read_8bit(loop_offset+loop_adjust,streamFile)) {
                loops_ok=1;
                break;
            }
        }
        if (!loops_ok)
            for (loop_adjust=interleave;loop_adjust<=interleave+0x10;loop_adjust+=8) {
#ifdef DEBUG
                fprintf(stderr,"looking for loop p/s at %#x,%#x\n",loop_offset-interleave+loop_adjust,loop_offset+loop_adjust);
#endif
                if (read_16bitBE(0x64,streamFile) == (uint8_t)read_8bit(loop_offset-interleave+loop_adjust,streamFile) &&
                        read_16bitBE(0xc4,streamFile) == (uint8_t)read_8bit(loop_offset+loop_adjust,streamFile)) {
                    loops_ok=1;
                    break;
                }
            }

        if (!loops_ok) goto fail;
#ifdef DEBUG
        fprintf(stderr,"loop p/s ok (with %#4x adjust)\n",loop_adjust);
#endif

        /* check for agreement */
        /* loop end (channel 1 & 2 headers) */
        if (read_32bitBE(0x34,streamFile) != read_32bitBE(0x94,streamFile))
            goto fail;
        
        /* Mr. Driller oddity */
        if (dsp_nibbles_to_samples(read_32bitBE(0x34,streamFile)*2)+1 <= read_32bitBE(0x20,streamFile)) {
#ifdef DEBUG
            fprintf(stderr,"loop end <= half total samples, should be doubled\n");
#endif
            double_loop_end = 1;
        }

        /* loop start (Cstr header and channel 1 header) */
        if (read_32bitBE(0x30,streamFile) != read_32bitBE(0x10,streamFile)
#if 0
                /* this particular glitch only true for SFA, though it
                 * seems like something similar happens in Donkey Konga */
                /* loop start (Cstr, channel 1 & 2 headers) */
                || (read_32bitBE(0x0c,streamFile)+read_32bitLE(0x30,streamFile)) !=
                read_32bitBE(0x90,streamFile)
#endif
           )
            /* alternatively (Donkey Konga) the header loop is 0x0c+0x10 */
            if (
                    /* loop start (Cstr header and channel 1 header) */
                    read_32bitBE(0x30,streamFile) != read_32bitBE(0x10,streamFile)+
                    read_32bitBE(0x0c,streamFile))
                /* further alternatively (Donkey Konga), if we loop back to
                 * the very first frame 0x30 might be 0x00000002 (which
                 * is a *valid* std dsp loop start, imagine that) while 0x10
                 * is 0x00000000 */
                if (!(read_32bitBE(0x30,streamFile) == 2 &&
                            read_32bitBE(0x10,streamFile) == 0))
                    /* lest there be too few alternatives, in Mr. Driller we
                     * find that [0x30] + [0x0c] + 8 = [0x10]*2 */
                    if (!(double_loop_end &&
                            read_32bitBE(0x30,streamFile) +
                            read_32bitBE(0x0c,streamFile) + 8 ==
                            read_32bitBE(0x10,streamFile)*2))
                        goto fail;

#ifdef DEBUG
        fprintf(stderr,"loop points agree\n");
#endif
    }

    /* assure that sample counts, sample rates agree */
    if (
            /* sample count (channel 1 & 2 headers) */
            read_32bitBE(0x20,streamFile) != read_32bitBE(0x80,streamFile) ||
            /* sample rate (channel 1 & 2 headers) */
            read_32bitBE(0x28,streamFile) != read_32bitBE(0x88,streamFile) ||
            /* sample count (Cstr header and channel 1 header) */
            read_32bitLE(0x14,streamFile) != read_32bitBE(0x20,streamFile) ||
            /* sample rate (Cstr header and channel 1 header) */
            (uint16_t)read_16bitLE(0x18,streamFile) != read_32bitBE(0x28,streamFile))
        goto fail;

    /* build the VGMSTREAM */

    vgmstream = allocate_vgmstream(2,loop_flag);
    if (!vgmstream) goto fail;

    /* fill in the vital statistics */
    vgmstream->sample_rate = read_32bitBE(0x28,streamFile);
    /* This is a slight hack to counteract their hack.
     * All the data is ofset by first_data so that the loop
     * point occurs at a block boundary. However, I always begin decoding
     * right after the header, as that is the start of the first block and
     * my interleave code relies on starting at the beginning of a block.
     * So we decode a few silent samples at the beginning, and here we make up
     * for it by lengthening the track by that much.
     */
    vgmstream->num_samples = read_32bitBE(0x20,streamFile) +
        (first_data-start_offset)/8*14;

    if (loop_flag) {
        off_t loop_start_bytes = loop_offset-start_offset-interleave;
        vgmstream->loop_start_sample = dsp_nibbles_to_samples((loop_start_bytes/(2*interleave)*interleave+loop_start_bytes%(interleave*2))*2);
        /*dsp_nibbles_to_samples(loop_start_bytes);*/
        /*dsp_nibbles_to_samples(read_32bitBE(0x30,streamFile)*2-inter);*/
        vgmstream->loop_end_sample =  dsp_nibbles_to_samples(
                read_32bitBE(0x34,streamFile))+1;

        if (double_loop_end)
            vgmstream->loop_end_sample =
                dsp_nibbles_to_samples(read_32bitBE(0x34,streamFile)*2)+1;

        if (vgmstream->loop_end_sample > vgmstream->num_samples) {
#ifdef DEBUG
            fprintf(stderr,"loop_end_sample > num_samples, adjusting\n");
#endif
            vgmstream->loop_end_sample = vgmstream->num_samples;
        }
    }

    vgmstream->coding_type = coding_NGC_DSP;
    vgmstream->layout_type = layout_interleave;
    vgmstream->interleave_block_size = interleave;
    vgmstream->meta_type = meta_DSP_CSTR;

    {
        int i;
        for (i=0;i<16;i++)
            vgmstream->ch[0].adpcm_coef[i]=read_16bitBE(0x3c+i*2,streamFile);
        for (i=0;i<16;i++)
            vgmstream->ch[1].adpcm_coef[i]=read_16bitBE(0x9c+i*2,streamFile);
    }
#ifdef DEBUG
    vgmstream->ch[0].loop_history1 = read_16bitBE(0x66,streamFile);
    vgmstream->ch[0].loop_history2 = read_16bitBE(0x68,streamFile);
    vgmstream->ch[1].loop_history1 = read_16bitBE(0xc6,streamFile);
    vgmstream->ch[1].loop_history2 = read_16bitBE(0xc8,streamFile);
#endif

    /* open the file for reading by each channel */
    {
        int i;
        for (i=0;i<2;i++) {
            vgmstream->ch[i].streamfile = streamFile->open(streamFile,filename,interleave);

            if (!vgmstream->ch[i].streamfile) goto fail;

            vgmstream->ch[i].channel_start_offset=
                vgmstream->ch[i].offset=
                start_offset+interleave*i;
        }
    }

    return vgmstream;

    /* clean up anything we may have opened */
fail:
    if (vgmstream) close_vgmstream(vgmstream);
    return NULL;
}
