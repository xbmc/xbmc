/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - Encoder related header  -
 *
 *  Copyright(C) 2002-2003 Michael Militzer <isibaar@xvid.org>
 *               2002-2003 Peter Ross <pross@xvid.org>
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

#ifndef _ENCODER_H_
#define _ENCODER_H_

#include "xvid.h"
#include "portab.h"
#include "global.h"

/*****************************************************************************
 * Constants
 ****************************************************************************/

/*****************************************************************************
 * Types
 ****************************************************************************/

typedef int bool;

/*****************************************************************************
 * Structures
 ****************************************************************************/

typedef struct
{
    /* --- constants --- */
	uint32_t width;
	uint32_t height;

	uint32_t edged_width;
	uint32_t edged_height;
	uint32_t mb_width;
	uint32_t mb_height;

    int plugin_flags;

	/* frame rate increment & base */
	int32_t fincr;
	uint32_t fbase;

    int profile;

	int global_flags;
	int bquant_ratio;
	int bquant_offset;
	int frame_drop_ratio;

    int min_quant[3];
    int max_quant[3];

	int par;
	int par_width;
	int par_height;

#ifdef _SMP
	int num_threads;
#endif


	int iMaxKeyInterval;
	int max_bframes;

/* --- inbetween vop stuff --- */
	/* rounding type; alternate 0-1 after each interframe */
	/* 1 <= fixed_code <= 4
	   automatically adjusted using motion vector statistics inside
	 */

	/* vars that not "quite" frame independant */
	uint32_t m_rounding_type;
	uint32_t m_fcode;
    int vol_flags;

	int64_t m_stamp;

	uint16_t *mpeg_quant_matrices;
} MBParam;


typedef struct
{
	int iTextBits;
	int iMvSum;
	int iMvCount;
	int kblks;
	int mblks;
	int ublks;
	int gblks;
} Statistics;


/* encoding queue */
typedef struct
{
	xvid_enc_frame_t frame;
	unsigned char quant_intra_matrix[64];
	unsigned char quant_inter_matrix[64];
	IMAGE image;
} QUEUEINFO;


typedef struct
{
    int frame_num;
    int fincr;
	int vol_flags;
    int vop_flags;
	int motion_flags;

	int coding_type;
	uint32_t quant;
	uint32_t rounding_type;
	uint32_t fcode;
	uint32_t bcode;

	uint32_t seconds;
	uint32_t ticks;
	int64_t stamp;

	IMAGE image;

	MACROBLOCK *mbs;

	WARPPOINTS warp;		/* as in bitstream */
	GMC_DATA gmc_data;		/* common data for all MBs */
	NEW_GMC_DATA new_gmc_data;		/* common data for all MBs */

    int length;         /* the encoded size of this frame */

	Statistics sStat;
	int is_edged, is_interpolated;
} FRAMEINFO;


typedef struct
{
	MBParam mbParam;

	int iFrameNum;
	int bitrate;

    /* zones */
    unsigned int num_zones;
    xvid_enc_zone_t * zones;

    /* plugins */
    int num_plugins;    /* note: we store plugin flags in MBPARAM */
    xvid_enc_plugin_t * plugins;

    /* dquant */

    int * temp_dquants;

	/* images */

	FRAMEINFO *current;
	FRAMEINFO *reference;

	IMAGE sOriginal;    /* original image copy for i/p frames */
    IMAGE sOriginal2;   /* original image copy for b-frames */
	IMAGE vInterH;
	IMAGE vInterV;
	IMAGE vInterVf;
	IMAGE vInterHV;
	IMAGE vInterHVf;

	IMAGE vGMC;

	/* image queue */
	int queue_head;
	int queue_tail;
	int queue_size;
	QUEUEINFO *queue;

	/* bframe buffer */
	int bframenum_head;
	int bframenum_tail;
	int flush_bframes;

	FRAMEINFO **bframes;
	IMAGE f_refh;
	IMAGE f_refv;
	IMAGE f_refhv;

    /* closed_gop fixup temporary storage */
	int closed_bframenum; /* == -1 if there is no fixup intended */
    QUEUEINFO closed_qframe;	/* qFrame, only valid when >= 0 */

	int m_framenum; /* debug frame num counter; unlike iFrameNum, does not reset at ivop */

	float fMvPrevSigma;
} Encoder;

/*****************************************************************************
 * Inline functions
 ****************************************************************************/

static __inline uint8_t
get_fcode(uint16_t sr)
{
	if (sr <= 16)
		return 1;

	else if (sr <= 32)
		return 2;

	else if (sr <= 64)
		return 3;

	else if (sr <= 128)
		return 4;

	else if (sr <= 256)
		return 5;

	else if (sr <= 512)
		return 6;

	else if (sr <= 1024)
		return 7;

	else
		return 0;
}


/*****************************************************************************
 * Prototypes
 ****************************************************************************/

void init_encoder(uint32_t cpu_flags);

int enc_create(xvid_enc_create_t * create);
int enc_destroy(Encoder * pEnc);
int enc_encode(Encoder * pEnc,
				   xvid_enc_frame_t * pFrame,
				   xvid_enc_stats_t * stats);

#endif
