#include "layout.h"
#include "../vgmstream.h"

/* set up for the block at the given offset */
void str_snds_block_update(off_t block_offset, VGMSTREAM * vgmstream) {
    off_t current_chunk;
    size_t file_size;
    int i;
    STREAMFILE *streamfile;
    int FoundSSMP = 0;
    off_t SSMP_offset = -1;

    current_chunk = block_offset;
    streamfile = vgmstream->ch[0].streamfile;
    file_size = get_streamfile_size(streamfile);

    /* we may have to skip some chunks */
    while (!FoundSSMP && current_chunk < file_size) {
        if (current_chunk+read_32bitBE(current_chunk+4,streamfile)>=file_size)
            break;
        switch (read_32bitBE(current_chunk,streamfile)) {
            case 0x534e4453:    /* SNDS */
                /* SSMP */
                if (read_32bitBE(current_chunk+0x10,streamfile)==0x53534d50) {
                    FoundSSMP = 1;
                    SSMP_offset = current_chunk;
                }
                break;
            case 0x46494c4c:    /* FILL, the main culprit */
            default:
                break;
        }

        current_chunk += read_32bitBE(current_chunk+4,streamfile);
    }

    if (!FoundSSMP) {
        /* if we couldn't find it all we can do is try playing the current
         * block, which is going to suck */
        vgmstream->current_block_offset = block_offset;
    }

    vgmstream->current_block_offset = SSMP_offset;
    vgmstream->current_block_size = (read_32bitBE(
            vgmstream->current_block_offset+4,
            vgmstream->ch[0].streamfile) - 0x18) / vgmstream->channels;
    vgmstream->next_block_offset = vgmstream->current_block_offset +
            read_32bitBE(vgmstream->current_block_offset+4,
                    vgmstream->ch[0].streamfile);

    for (i=0;i<vgmstream->channels;i++) {
        vgmstream->ch[i].offset = vgmstream->current_block_offset + 0x18;
    }
}
