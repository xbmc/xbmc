#include "layout.h"
#include "../vgmstream.h"

/* set up for the block at the given offset */
void caf_block_update(off_t block_offset, VGMSTREAM * vgmstream) {
    int i;

    vgmstream->current_block_offset = block_offset;
    vgmstream->current_block_size = read_32bitBE(
            vgmstream->current_block_offset+0x14,
            vgmstream->ch[0].streamfile);
    vgmstream->next_block_offset = vgmstream->current_block_offset + 
		(off_t)read_32bitBE(vgmstream->current_block_offset+0x04,
            vgmstream->ch[0].streamfile);

    for (i=0;i<vgmstream->channels;i++) {
        vgmstream->ch[i].offset = vgmstream->current_block_offset +
            read_32bitBE(block_offset+0x10+(8*i),vgmstream->ch[0].streamfile);
    }

    /* coeffs */
    for (i=0;i<16;i++) {
        vgmstream->ch[0].adpcm_coef[i] = read_16bitBE(block_offset+0x34+(2*i),vgmstream->ch[0].streamfile);
        vgmstream->ch[1].adpcm_coef[i] = read_16bitBE(block_offset+0x60+(2*i),vgmstream->ch[0].streamfile);
    }
}
