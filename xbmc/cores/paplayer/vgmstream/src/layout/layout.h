#ifndef _LAYOUT_H
#define _LAYOUT_H

#include "../streamtypes.h"
#include "../vgmstream.h"

void ast_block_update(off_t block_ofset, VGMSTREAM * vgmstream);

void render_vgmstream_blocked(sample * buffer, int32_t sample_count, VGMSTREAM * vgmstream);

void halpst_block_update(off_t block_ofset, VGMSTREAM * vgmstream);

void xa_block_update(off_t block_offset, VGMSTREAM * vgmstream);

void ea_block_update(off_t block_offset, VGMSTREAM * vgmstream);

void eacs_block_update(off_t block_offset, VGMSTREAM * vgmstream);

void caf_block_update(off_t block_offset, VGMSTREAM * vgmstream);

void wsi_block_update(off_t block_offset, VGMSTREAM * vgmstream);

void str_snds_block_update(off_t block_offset, VGMSTREAM * vgmstream);

void ws_aud_block_update(off_t block_offset, VGMSTREAM * vgmstream);

void matx_block_update(off_t block_offset, VGMSTREAM * vgmstream);

void de2_block_update(off_t block_offset, VGMSTREAM * vgmstream);

void vs_block_update(off_t block_offset, VGMSTREAM * vgmstream);

void emff_ps2_block_update(off_t block_offset, VGMSTREAM * vgmstream);

void emff_ngc_block_update(off_t block_offset, VGMSTREAM * vgmstream);

void gsb_block_update(off_t block_offset, VGMSTREAM * vgmstream);

void xvas_block_update(off_t block_offset, VGMSTREAM * vgmstream);

void thp_block_update(off_t block_offset, VGMSTREAM * vgmstream);

void filp_block_update(off_t block_offset, VGMSTREAM * vgmstream);

void render_vgmstream_interleave(sample * buffer, int32_t sample_count, VGMSTREAM * vgmstream);

void render_vgmstream_nolayout(sample * buffer, int32_t sample_count, VGMSTREAM * vgmstream);

void render_vgmstream_interleave_byte(sample * buffer, int32_t sample_count, VGMSTREAM * vgmstream);

void render_vgmstream_mus_acm(sample * buffer, int32_t sample_count, VGMSTREAM * vgmstream);

void render_vgmstream_aix(sample * buffer, int32_t sample_count, VGMSTREAM * vgmstream);

void render_vgmstream_aax(sample * buffer, int32_t sample_count, VGMSTREAM * vgmstream);

#endif
