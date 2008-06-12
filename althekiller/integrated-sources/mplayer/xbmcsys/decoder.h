/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - Decoder related header -
 *
 *  Copyright(C) 2002-2003 Peter Ross <pross@xvid.org>
 *
 *  This program is free software ; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation ; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY ; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program ; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 * $Id$
 *
 ****************************************************************************/

#ifndef _DECODER_H_
#define _DECODER_H_

#include "xvid.h"
#include "portab.h"
#include "global.h"
#include "image/image.h"
#include "image/postprocessing.h"

/*****************************************************************************
 * Structures
 ****************************************************************************/

/* complexity estimation toggles */
typedef struct
{
	int method;

	int opaque;
	int transparent;
	int intra_cae;
	int inter_cae;
	int no_update;
	int upsampling;

	int intra_blocks;
	int inter_blocks;
	int inter4v_blocks;
	int gmc_blocks;
	int not_coded_blocks;

	int dct_coefs;
	int dct_lines;
	int vlc_symbols;
	int vlc_bits;

	int apm;
	int npm;
	int interpolate_mc_q;
	int forw_back_mc_q;
	int halfpel2;
	int halfpel4;

	int sadct;
	int quarterpel;
} ESTIMATION;


typedef struct
{
	/* vol bitstream */

	int time_inc_resolution;
	int fixed_time_inc;
	uint32_t time_inc_bits;

	uint32_t shape;
	uint32_t quant_bits;
	uint32_t quant_type;
	uint16_t *mpeg_quant_matrices;
	int32_t quarterpel;
	int32_t cartoon_mode;
	int complexity_estimation_disable;
	ESTIMATION estimation;

	int interlacing;
	uint32_t top_field_first;
	uint32_t alternate_vertical_scan;

	int aspect_ratio;
	int par_width;
	int par_height;

	int sprite_enable;
	int sprite_warping_points;
	int sprite_warping_accuracy;
	int sprite_brightness_change;

	int newpred_enable;
	int reduced_resolution_enable;

	/* The bitstream version if it's a XviD stream */
	int bs_version;

	/* image */

	int fixed_dimensions;
	uint32_t width;
	uint32_t height;
	uint32_t edged_width;
	uint32_t edged_height;

	IMAGE cur;
	IMAGE refn[2];				/* 0   -- last I or P VOP */
								/* 1   -- first I or P */
	IMAGE tmp;		/* bframe interpolation, and post processing tmp buffer */
	IMAGE qtmp;		/* quarter pel tmp buffer */

	/* postprocessing */
	XVID_POSTPROC postproc;

	/* macroblock */

	uint32_t mb_width;
	uint32_t mb_height;
	MACROBLOCK *mbs;

	/*
	 * for B-frame & low_delay==0
	 * XXX: should move frame based stuff into a DECODER_FRAMEINFO struct
	 */
	MACROBLOCK *last_mbs;			/* last MB */
    int last_coding_type;           /* last coding type value */
	int last_reduced_resolution;	/* last reduced_resolution value */
	int32_t frames;				/* total frame number */
	int32_t packed_mode;		/* bframes packed bitstream? (1 = yes) */
	int8_t scalability;
	VECTOR p_fmv, p_bmv;		/* pred forward & backward motion vector */
	int64_t time;				/* for record time */
	int64_t time_base;
	int64_t last_time_base;
	int64_t last_non_b_time;
	int32_t time_pp;
	int32_t time_bp;
	uint32_t low_delay;			/* low_delay flage (1 means no B_VOP) */
	uint32_t low_delay_default;	/* default value for low_delay flag */

	/* for GMC: central place for all parameters */

	IMAGE gmc;		/* gmc tmp buffer, remove for blockbased compensation */
	GMC_DATA gmc_data;
	NEW_GMC_DATA new_gmc_data;

	xvid_image_t* out_frm;                /* This is used for slice rendering */

	int * qscale;				/* quantization table for decoder's stats */
}
DECODER;

/*****************************************************************************
 * Decoder prototypes
 ****************************************************************************/

void init_decoder(uint32_t cpu_flags);

int decoder_create(xvid_dec_create_t * param);
int decoder_destroy(DECODER * dec);
int decoder_decode(DECODER * dec,
				   xvid_dec_frame_t * frame, xvid_dec_stats_t * stats);


#endif
