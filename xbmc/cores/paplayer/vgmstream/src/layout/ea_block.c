#include "layout.h"
#include "../coding/coding.h"
#include "../vgmstream.h"

/* set up for the block at the given offset */
void ea_block_update(off_t block_offset, VGMSTREAM * vgmstream) {
    int i;

	init_get_high_nibble(vgmstream);

	// Search for next SCDL or SCEl block ...
	do {
		block_offset+=4;
		if(block_offset>=(off_t)get_streamfile_size(vgmstream->ch[0].streamfile)) {
			vgmstream->next_block_offset=block_offset;
			return;
		}
	} while (read_32bitBE(block_offset,vgmstream->ch[0].streamfile)!=0x5343446C);

	// reset channel offset
	for(i=0;i<vgmstream->channels;i++) {
		vgmstream->ch[i].channel_start_offset=0;
	}

    vgmstream->current_block_offset = block_offset;
    vgmstream->next_block_offset = block_offset+read_32bitLE(block_offset+4,vgmstream->ch[0].streamfile)-4;

	if(vgmstream->ea_big_endian) {
		vgmstream->current_block_size = read_32bitBE(block_offset+8,vgmstream->ch[0].streamfile);
		
		for(i=0;i<vgmstream->channels;i++) {
			vgmstream->ch[i].offset=read_32bitBE(block_offset+0x0C+(i*4),vgmstream->ch[0].streamfile)+(4*vgmstream->channels);
			vgmstream->ch[i].offset+=vgmstream->current_block_offset+0x0C;
		}
		vgmstream->current_block_size /= 28;

	} else {
		switch(vgmstream->coding_type) {
			case coding_PSX:
				vgmstream->ch[0].offset=vgmstream->current_block_offset+0x10;
				vgmstream->ch[1].offset=(read_32bitLE(block_offset+0x04,vgmstream->ch[0].streamfile)-0x10)/vgmstream->channels;
				vgmstream->ch[1].offset+=vgmstream->ch[0].offset;
				vgmstream->current_block_size=read_32bitLE(block_offset+0x04,vgmstream->ch[0].streamfile)-0x10;
				vgmstream->current_block_size/=vgmstream->channels;
				break;
			case coding_EA_ADPCM: 
				vgmstream->current_block_size = read_32bitLE(block_offset+4,vgmstream->ch[0].streamfile);
				for(i=0;i<vgmstream->channels;i++) {
					vgmstream->ch[i].offset=vgmstream->current_block_offset+0x0C+(4*vgmstream->channels);
					vgmstream->ch[i].adpcm_history1_32=(uint32_t)read_16bitLE(vgmstream->current_block_offset+0x0c+(i*4),vgmstream->ch[0].streamfile);
					vgmstream->ch[i].adpcm_history2_32=(uint32_t)read_16bitLE(vgmstream->current_block_offset+0x0e +(i*4),vgmstream->ch[0].streamfile);
				}
				break;
			case coding_PCM16LE_int: 
				vgmstream->current_block_size = read_32bitLE(block_offset+4,vgmstream->ch[0].streamfile)-0x0C;
				for(i=0;i<vgmstream->channels;i++) {
					vgmstream->ch[i].offset=block_offset+0x0C+(i*2);
				}
				vgmstream->current_block_size/=2;
				vgmstream->current_block_size-=2;
				break;
			case coding_XBOX:
				vgmstream->current_block_size = read_32bitLE(block_offset+0x10,vgmstream->ch[0].streamfile);
				for(i=0;i<vgmstream->channels;i++) {
					vgmstream->ch[i].offset=read_32bitLE(block_offset+0x0C+(i*4),vgmstream->ch[0].streamfile)+(4*vgmstream->channels);
					vgmstream->ch[i].offset+=vgmstream->current_block_offset+0x0C;
				}
				break;
			default:
				vgmstream->current_block_size = read_32bitLE(block_offset+8,vgmstream->ch[0].streamfile);
				for(i=0;i<vgmstream->channels;i++) {
					vgmstream->ch[i].offset=read_32bitLE(block_offset+0x0C+(i*4),vgmstream->ch[0].streamfile)+(4*vgmstream->channels);
					vgmstream->ch[i].offset+=vgmstream->current_block_offset+0x0C;
				}
				vgmstream->current_block_size /= 28;
		}
	}

	if((vgmstream->ea_compression_version<3) && (vgmstream->coding_type!=coding_PSX) && (vgmstream->coding_type!=coding_EA_ADPCM) && (vgmstream->coding_type!=coding_XBOX)) {
		for(i=0;i<vgmstream->channels;i++) {
			if(vgmstream->ea_big_endian) {
				vgmstream->ch[i].adpcm_history1_32=read_16bitBE(vgmstream->ch[i].offset,vgmstream->ch[0].streamfile);
				vgmstream->ch[i].adpcm_history2_32=read_16bitBE(vgmstream->ch[i].offset+2,vgmstream->ch[0].streamfile);
			} else {
				vgmstream->ch[i].adpcm_history1_32=read_16bitLE(vgmstream->ch[i].offset,vgmstream->ch[0].streamfile);
				vgmstream->ch[i].adpcm_history2_32=read_16bitLE(vgmstream->ch[i].offset+2,vgmstream->ch[0].streamfile);
			}
			vgmstream->ch[i].offset+=4;
		}
	}
}

void eacs_block_update(off_t block_offset, VGMSTREAM * vgmstream) {
	int i;
	off_t block_size=vgmstream->current_block_size;

	if(read_32bitBE(block_offset,vgmstream->ch[0].streamfile)==0x31534E6C) {
		block_offset+=0x0C;
	}

    vgmstream->current_block_offset = block_offset;

	if(read_32bitBE(block_offset,vgmstream->ch[0].streamfile)==0x31534E64) { /* 1Snd */
		block_offset+=4;
		if(vgmstream->ea_platform==0)
			block_size=read_32bitLE(vgmstream->current_block_offset+0x04,
									vgmstream->ch[0].streamfile);
		else
			block_size=read_32bitBE(vgmstream->current_block_offset+0x04,
									vgmstream->ch[0].streamfile);
		block_offset+=4;
	}

	vgmstream->current_block_size=block_size-8;

	if(vgmstream->coding_type==coding_EACS_IMA) {
		init_get_high_nibble(vgmstream);
		vgmstream->current_block_size=read_32bitLE(block_offset,vgmstream->ch[0].streamfile);

		for(i=0;i<vgmstream->channels;i++) {
			vgmstream->ch[i].adpcm_step_index = read_32bitLE(block_offset+0x04+i*4,vgmstream->ch[0].streamfile);
			vgmstream->ch[i].adpcm_history1_32 = read_32bitLE(block_offset+0x04+i*4+(4*vgmstream->channels),vgmstream->ch[0].streamfile);
			vgmstream->ch[i].offset = block_offset+0x14;
		}
	} else {
		if(vgmstream->coding_type==coding_PSX) {
			for (i=0;i<vgmstream->channels;i++) 
				vgmstream->ch[i].offset = vgmstream->current_block_offset+8+(i*(vgmstream->current_block_size/2));
		} else {

			for (i=0;i<vgmstream->channels;i++) {
				if(vgmstream->coding_type==coding_PCM16LE_int) 
					vgmstream->ch[i].offset = block_offset+(i*2); 
				else
					vgmstream->ch[i].offset = block_offset+i; 
			}
		}
		vgmstream->current_block_size/=vgmstream->channels;
	}
	vgmstream->next_block_offset = vgmstream->current_block_offset + 
		(off_t)block_size;
}
