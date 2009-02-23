#include "layout.h"
#include "../vgmstream.h"

/* set up for the block at the given offset */
void xvas_block_update(off_t block_offset, VGMSTREAM * vgmstream) {
    int i;

    vgmstream->current_block_offset = block_offset;

	if((vgmstream->current_block_offset-get_streamfile_size(vgmstream->ch[0].streamfile))>(0x20000-0x20))
		vgmstream->current_block_size = 0x20000-0x20;
	else
		vgmstream->current_block_size = vgmstream->current_block_offset-get_streamfile_size(vgmstream->ch[0].streamfile)-0x20;

    vgmstream->next_block_offset = 
        vgmstream->current_block_offset +
        (vgmstream->current_block_size + 0x20);

    for (i=0;i<vgmstream->channels;i++) {
        vgmstream->ch[i].offset = vgmstream->current_block_offset;
    }
	vgmstream->current_block_size /=2;

}
