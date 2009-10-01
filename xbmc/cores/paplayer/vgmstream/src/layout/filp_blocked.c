#include "layout.h"
#include "../vgmstream.h"

/* set up for the block at the given offset */
void filp_block_update(off_t block_offset, VGMSTREAM * vgmstream) {
    int i;

	vgmstream->current_block_offset = block_offset;
	vgmstream->current_block_size = read_32bitLE(
			vgmstream->current_block_offset+0x18,
			vgmstream->ch[0].streamfile)-0x800;
	vgmstream->next_block_offset = vgmstream->current_block_offset+vgmstream->current_block_size+0x800;
	vgmstream->current_block_size/=vgmstream->channels;

	for (i=0;i<vgmstream->channels;i++) {
        vgmstream->ch[i].offset = vgmstream->current_block_offset+0x800+(vgmstream->current_block_size*i);
		
    }
}
