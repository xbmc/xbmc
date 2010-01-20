#include "meta.h"
#include "../coding/coding.h"
#include "../layout/layout.h"
#include "../util.h"

/* 3DO format, .str extension and possibly a CTRL header, blocks and
 * AIFF-C style format specifier. Blocks are not IFF-compliant. Interesting
 * blocks are all SNDS */

VGMSTREAM * init_vgmstream_str_snds(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    char filename[260];

    int channel_count;
    int loop_flag = 0;
    off_t SHDR_offset = -1;
    int FoundSHDR = 0;
    int CTRL_size = -1;

    size_t file_size;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("str",filename_extension(filename))) goto fail;

    /* check for opening CTRL or SNDS chunk */
    if (read_32bitBE(0x0,streamFile) != 0x4354524c &&   /* CTRL */
        read_32bitBE(0x0,streamFile) != 0x534e4453)     /* SNDS */
        goto fail;

    file_size = get_streamfile_size(streamFile);

    /* scan chunks until we find a SNDS containing a SHDR */
    {
        off_t current_chunk;

        current_chunk = 0;

        while (!FoundSHDR && current_chunk < file_size) {
            if (current_chunk < 0) goto fail;

            if (current_chunk+read_32bitBE(current_chunk+4,streamFile) >=
                    file_size) goto fail;

            switch (read_32bitBE(current_chunk,streamFile)) {
                case 0x4354524C: /* CTRL */
                    /* to distinguish between styles */
                    CTRL_size = read_32bitBE(current_chunk+4,streamFile);
                    break;
                case 0x534e4453: /* SNDS */
                    switch (read_32bitBE(current_chunk+16,streamFile)) {
                        case 0x53484452: /* SHDR */
                            FoundSHDR = 1;
                            SHDR_offset = current_chunk+16;
                            break;
                        default:
                            break;
                    }
                    break;
                default:
                    /* ignore others for now */
                    break;
            }

            current_chunk += read_32bitBE(current_chunk+4,streamFile);
        }
    }

    if (!FoundSHDR) goto fail;

    /* details */
    channel_count = read_32bitBE(SHDR_offset+0x20,streamFile);
    loop_flag = 0;

    /* build the VGMSTREAM */

    vgmstream = allocate_vgmstream(channel_count,loop_flag);
    if (!vgmstream) goto fail;

    /* fill in the vital statistics */
    if (CTRL_size == 0x1C) {
        vgmstream->num_samples =
            read_32bitBE(SHDR_offset+0x2c,streamFile)-1; /* sample count? */
    } else {
        vgmstream->num_samples =
            read_32bitBE(SHDR_offset+0x2c,streamFile)   /* frame count? */
            * 0x10;
    }

	vgmstream->num_samples/=vgmstream->channels;

    vgmstream->sample_rate = read_32bitBE(SHDR_offset+0x1c,streamFile);
    switch (read_32bitBE(SHDR_offset+0x24,streamFile)) {
        case 0x53445832:    /* SDX2 */
            if (channel_count > 1) {
                vgmstream->coding_type = coding_SDX2_int;
                vgmstream->interleave_block_size = 1;
            } else
                vgmstream->coding_type = coding_SDX2;
            break;
        default:
            goto fail;
    }
    vgmstream->layout_type = layout_str_snds_blocked;
    vgmstream->meta_type = meta_STR_SNDS;

    /* channels and loop flag are set by allocate_vgmstream */
    if (loop_flag) {
        /* just guessin', no way to set loop flag anyway */
        vgmstream->loop_start_sample = 0;
        vgmstream->loop_end_sample = vgmstream->num_samples;
    }

    /* open the file for reading by each channel */
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
    str_snds_block_update(0,vgmstream);

    return vgmstream;

    /* clean up anything we may have opened */
fail:
    if (vgmstream) close_vgmstream(vgmstream);
    return NULL;
}
