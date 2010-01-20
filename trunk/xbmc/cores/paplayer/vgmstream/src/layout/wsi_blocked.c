#include "layout.h"
#include "../vgmstream.h"

/* set up for the block at the given offset */
void wsi_block_update(off_t block_offset, VGMSTREAM * vgmstream) {
    int i;

    /* assume that all channels have the same size for this block */

    vgmstream->current_block_offset = block_offset;
    /* current_block_size is the data size in this block, so subtract header */
    vgmstream->current_block_size = read_32bitBE(
            vgmstream->current_block_offset,
            vgmstream->ch[0].streamfile) - 0x10;
    vgmstream->next_block_offset = 
        vgmstream->current_block_offset +
        (vgmstream->current_block_size + 0x10) * vgmstream->channels;

    for (i=0;i<vgmstream->channels;i++) {
        vgmstream->ch[i].offset = vgmstream->current_block_offset +
            0x10 + (vgmstream->current_block_size+0x10)*i;
    }
}
