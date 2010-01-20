#include "meta.h"
#include "../coding/coding.h"
#include "../layout/layout.h"
#include "../util.h"

VGMSTREAM * init_vgmstream_halpst(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    char filename[260];

    int channel_count;
    int loop_flag = 0;

    int32_t samples_l,samples_r;
    int32_t start_sample = 0;

    size_t max_block;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("hps",filename_extension(filename))) goto fail;

    /* check header */
    if ((uint32_t)read_32bitBE(0,streamFile)!=0x2048414C || /* " HAL" */
            read_32bitBE(4,streamFile)!=0x50535400)         /* "PST\0" */
        goto fail;
    
    /* details */
    channel_count = read_32bitBE(0xc,streamFile);
    max_block = read_32bitBE(0x10,streamFile)/channel_count;

    /* have I ever seen a mono .hps? */
    if (channel_count!=2) goto fail;

    /* yay for redundancy, gives us something to test */
    samples_l = dsp_nibbles_to_samples(read_32bitBE(0x18,streamFile))+1;
    samples_r = dsp_nibbles_to_samples(read_32bitBE(0x50,streamFile))+1;

    if (samples_l != samples_r) goto fail;

    /*
     * looping info is implicit in the "next block" field of the final
     * block, so we have to find that
     */
    {
        off_t offset = 0x80, last_offset = 0;
        off_t loop_offset;

        /* determine if there is a loop */
        while (offset > last_offset) {
            last_offset = offset;
            offset = read_32bitBE(offset+8,streamFile);
        }
        if (offset < 0) loop_flag = 0;
        else {
            /* one more pass to determine start sample */
            int32_t start_nibble = 0;
            loop_flag = 1;

            loop_offset = offset;
            offset = 0x80;
            while (offset != loop_offset) {
                start_nibble += read_32bitBE(offset,streamFile);
                offset = read_32bitBE(offset+8,streamFile);
            }

            start_sample = dsp_nibbles_to_samples(start_nibble);
        }

    }

    /* build the VGMSTREAM */

    vgmstream = allocate_vgmstream(channel_count,loop_flag);
    if (!vgmstream) goto fail;

    /* fill in the vital statistics */
    vgmstream->num_samples = samples_l;
    vgmstream->sample_rate = read_32bitBE(8,streamFile);
    /* channels and loop flag are set by allocate_vgmstream */
    if (loop_flag) {
        vgmstream->loop_start_sample = start_sample;
        vgmstream->loop_end_sample = vgmstream->num_samples;
    }

    vgmstream->coding_type = coding_NGC_DSP;
    vgmstream->layout_type = layout_halpst_blocked;
    vgmstream->meta_type = meta_HALPST;

    /* load decode coefs */
    {
        int i;
        for (i=0;i<16;i++)
            vgmstream->ch[0].adpcm_coef[i] = read_16bitBE(0x20+i*2,streamFile);
        for (i=0;i<16;i++)
            vgmstream->ch[1].adpcm_coef[i] = read_16bitBE(0x58+i*2,streamFile);
    }

    /* open the file for reading by each channel */
    {
        int i;
        for (i=0;i<channel_count;i++) {
            vgmstream->ch[i].streamfile = streamFile->open(streamFile,filename,
                    (i==0?
                     max_block+0x20: /* first buffer a bit bigger to 
                                        read block header without
                                        inefficiency */
                     max_block
                    )
                    );

            if (!vgmstream->ch[i].streamfile) goto fail;
        }
    }

    /* start me up */
    halpst_block_update(0x80,vgmstream);

    return vgmstream;

    /* clean up anything we may have opened */
fail:
    if (vgmstream) close_vgmstream(vgmstream);
    return NULL;
}
