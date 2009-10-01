#include "layout.h"
#include "../vgmstream.h"

/* set up for the block at the given offset */
void matx_block_update(off_t block_offset, VGMSTREAM * vgmstream) {
    int i;

    vgmstream->current_block_offset = block_offset;
    vgmstream->current_block_size = read_32bitLE(
            vgmstream->current_block_offset,
            vgmstream->ch[0].streamfile);
    vgmstream->next_block_offset = vgmstream->current_block_offset + vgmstream->current_block_size + 8;
	vgmstream->current_block_size/=vgmstream->channels;

    for (i=0;i<vgmstream->channels;i++) {
        vgmstream->ch[i].offset = vgmstream->current_block_offset + 8;
    }
}
