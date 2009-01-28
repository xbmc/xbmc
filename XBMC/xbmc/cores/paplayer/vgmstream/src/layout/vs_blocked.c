#include "layout.h"
#include "../vgmstream.h"

/* set up for the block at the given offset */
void vs_block_update(off_t block_offset, VGMSTREAM * vgmstream) {
    int i;

    for (i=0;i<vgmstream->channels;i++) {
		vgmstream->current_block_offset = block_offset;
		vgmstream->current_block_size = read_32bitLE(
				vgmstream->current_block_offset,
				vgmstream->ch[0].streamfile);
		vgmstream->next_block_offset = vgmstream->current_block_offset + vgmstream->current_block_size + 0x4;
        vgmstream->ch[i].offset = vgmstream->current_block_offset + 0x4;
		if(i==0) block_offset=vgmstream->next_block_offset;
    }
}
