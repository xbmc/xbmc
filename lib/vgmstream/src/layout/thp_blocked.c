#include "layout.h"
#include "../vgmstream.h"

/* set up for the block at the given offset */
void thp_block_update(off_t block_offset, VGMSTREAM * vgmstream) {
    int i,j;
	STREAMFILE *streamFile=vgmstream->ch[0].streamfile;
	off_t	start_offset;
	int32_t	nextFrameSize;

	vgmstream->current_block_offset = block_offset;
	nextFrameSize=read_32bitBE(vgmstream->current_block_offset,streamFile);

	vgmstream->next_block_offset = vgmstream->current_block_offset
		                         + vgmstream->thpNextFrameSize;
	vgmstream->thpNextFrameSize=nextFrameSize;

	start_offset=vgmstream->current_block_offset
		         + read_32bitBE(vgmstream->current_block_offset+0x08,streamFile)+0x10;
	vgmstream->current_block_size=read_32bitBE(start_offset,streamFile);
	start_offset+=8;

	for(i=0;i<vgmstream->channels;i++) {
		for(j=0;j<16;j++) {
			vgmstream->ch[i].adpcm_coef[j]=read_16bitBE(start_offset+(i*0x20)+(j*2),streamFile);
		}
		vgmstream->ch[i].adpcm_history1_16=read_16bitBE(start_offset + (0x20*vgmstream->channels) + (i*4),streamFile);
		vgmstream->ch[i].adpcm_history2_16=read_16bitBE(start_offset + (0x20*vgmstream->channels) + (i*4) + 2,streamFile);
        vgmstream->ch[i].offset = start_offset + (0x24*vgmstream->channels)+(i*vgmstream->current_block_size);
	}
}
