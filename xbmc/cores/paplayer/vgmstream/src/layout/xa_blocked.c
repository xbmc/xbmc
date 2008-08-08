#include "layout.h"
#include "../coding/coding.h"
#include "../vgmstream.h"

/* set up for the block at the given offset */
void xa_block_update(off_t block_offset, VGMSTREAM * vgmstream) {

	int i;
	int8_t currentChannel=0;
	int8_t subAudio=0;
	
	init_get_high_nibble(vgmstream);

	if(vgmstream->samples_into_block!=0)
		// don't change this variable in the init process
		vgmstream->xa_sector_length+=128;

	// We get to the end of a sector ?
	if(vgmstream->xa_sector_length==(18*128)) {
		vgmstream->xa_sector_length=0;

		// 0x30 of unused bytes/sector :(
		block_offset+=0x30;
begin:
		// Search for selected channel & valid audio
		currentChannel=read_8bit(block_offset-7,vgmstream->ch[0].streamfile);
		subAudio=read_8bit(block_offset-6,vgmstream->ch[0].streamfile);

		// audio is coded as 0x64
		if(!((subAudio==0x64) && (currentChannel==vgmstream->xa_channel))) {
			// go to next sector
			block_offset+=2352;
			if(currentChannel!=-1) goto begin;
		} 
	}

	vgmstream->current_block_offset = block_offset;

	// Quid : how to stop the current channel ???
	// i set up 0 to current_block_size to make vgmstream not playing bad samples
	// another way to do it ??? 
	// (as the number of samples can be false in cd-xa due to multi-channels)
	vgmstream->current_block_size = (currentChannel==-1?0:112);
	
	vgmstream->next_block_offset = vgmstream->current_block_offset+128;
	for (i=0;i<vgmstream->channels;i++) {
	    vgmstream->ch[i].offset = vgmstream->current_block_offset;
	}		
}
