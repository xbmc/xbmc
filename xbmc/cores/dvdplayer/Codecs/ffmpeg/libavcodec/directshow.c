/* 
 *  Copyright (C) 2006-2009 mplayerc
 *  Copyright (C) 2010 Team XBMC
 *  http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */



/*#include "avcodec.h"
#include "h264.h"
#include "vc1.h"

#undef NDEBUG
#include <assert.h>
*/

#include "directshow_internal.h"
#include "directshow.h"
#include "h264.h"
#include "h264data.h"
/*int FFH264BuildPicParams (DXVA_PicParams_H264 *pDXVAPicParams, DXVA_Qmatrix_H264 *pDXVAScalingMatrix, int *nFieldType, int *nSliceType, struct AVCodecContext *pAVCtx, int nPCIVendor)*/

const byte ZZ_SCAN[16]  =
{  0,  1,  4,  8,  5,  2,  3,  6,  9, 12, 13, 10,  7, 11, 14, 15
};

const byte ZZ_SCAN8[64] =
{  0,  1,  8, 16,  9,  2,  3, 10, 17, 24, 32, 25, 18, 11,  4,  5,
   12, 19, 26, 33, 40, 48, 41, 34, 27, 20, 13,  6,  7, 14, 21, 28,
   35, 42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23, 30, 37, 44, 51,
   58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63
};
/*
ffmpeg[164C]: [h264_dshow] ff_directshow_h264_fill_slice_long!
ffmpeg[164C]: [h264_dshow] ff_directshow_add_data_chunk needed
field_end  directshow
ffmpeg[164C]: [h264_dshow] ff_directshow_h264_set_reference_frames!
ffmpeg[164C]: [h264_dshow] ff_directshow_h264_picture_complete!
*/
static void fill_picentry(DXVA_PicEntry_H264 *pic,
                               unsigned index, unsigned flag)
{
    assert((index&0x7f) == index && (flag&0x01) == flag);
    pic->bPicEntry = index | (flag << 7);
}

void ff_directshow_h264_fill_slice_long(MpegEncContext *s)
{
	H264Context *h = s->avctx->priv_data;
	directshow_dxva_h264 *pict;
	int						field_pic_flag;
	unsigned int			i,j,k;
	
	pict = (directshow_dxva_h264 *)s->current_picture_ptr->data[0];
	
	DXVA_Slice_H264_Long*	pSlice = &((DXVA_Slice_H264_Long*) pict->slice_long)[h->current_slice-1];
	
    av_log(s->avctx, AV_LOG_ERROR, "ff_directshow_h264_fill_slice_long!\n");
	field_pic_flag = (h->s.picture_structure != PICT_FRAME);

	pSlice->slice_id						= h->current_slice-1;
	pSlice->first_mb_in_slice				= (s->mb_y >> FIELD_OR_MBAFF_PICTURE) * s->mb_width + s->mb_x;/*= h->first_mb_in_slice;*/
	pSlice->NumMbsForSlice					= 0; // h->s.mb_num;				// TODO : to be checked !
	pSlice->BitOffsetToSliceData			= get_bits_count(&s->gb) + 8;
	pSlice->slice_type						= ff_h264_get_slice_type(h);/*h->raw_slice_type; */
	if (h->slice_type_fixed)
        pSlice->slice_type += 5;
	pSlice->luma_log2_weight_denom			= h->luma_log2_weight_denom;
	pSlice->chroma_log2_weight_denom		= h->chroma_log2_weight_denom;
	pSlice->num_ref_idx_l0_active_minus1	= h->ref_count[0]-1;	// num_ref_idx_l0_active_minus1;
	pSlice->num_ref_idx_l1_active_minus1	= h->ref_count[1]-1;	// num_ref_idx_l1_active_minus1;
	pSlice->slice_alpha_c0_offset_div2		= h->slice_alpha_c0_offset / 2;
	pSlice->slice_beta_offset_div2			= h->slice_beta_offset / 2;
	pSlice->Reserved8Bits					= 0;
	
	// Fill prediction weights
	memset (pSlice->Weights, 0, sizeof(pSlice->Weights));
	for(i=0; i<2; i++){
		for(j=0; j<h->ref_count[i]; j++){
			//         L0&L1          Y,Cb,Cr  Weight,Offset
			// Weights  [2]    [32]     [3]         [2]
			pSlice->Weights[i][j][0][0] = h->luma_weight[j][i][0];
			pSlice->Weights[i][j][0][1] = h->luma_weight[j][i][1];

			for(k=0; k<2; k++){
				pSlice->Weights[i][j][k+1][0] = h->chroma_weight[j][i][k][0];
				pSlice->Weights[i][j][k+1][1] = h->chroma_weight[j][i][k][1];
			}
		}
	}

	pSlice->slice_qs_delta    = 0; /* XXX not implemented by FFmpeg */
    pSlice->slice_qp_delta    = s->qscale - h->pps.init_qp;
	pSlice->redundant_pic_cnt				= h->redundant_pic_count;
	pSlice->direct_spatial_mv_pred_flag		= h->direct_spatial_mv_pred;
	pSlice->cabac_init_idc					= h->cabac_init_idc;
	pSlice->disable_deblocking_filter_idc	= h->deblocking_filter;

	for(i=0; i<32; i++)
	{ 
	  /*DXVA_PicEntry_H264*/
	  fill_picentry(&pSlice->RefPicList[0][i],127,1);
	  fill_picentry(&pSlice->RefPicList[1][i],127,1);
	  /*pSlice->RefPicList[0][i].AssociatedFlag = 1;
	  pSlice->RefPicList[0][i].bPicEntry = 255; 
	  pSlice->RefPicList[0][i].Index7Bits = 127;
	  pSlice->RefPicList[1][i].AssociatedFlag = 1; 
	  pSlice->RefPicList[1][i].bPicEntry = 255;
	  pSlice->RefPicList[1][i].Index7Bits = 127;*/
	}
}

void ff_directshow_h264_picture_complete(MpegEncContext *s)
{

int *nFieldType;
int *nSliceType;
    H264Context *h = s->avctx->priv_data;
	directshow_dxva_h264 *pict;
	
	pict = (directshow_dxva_h264 *)s->current_picture_ptr->data[0];
	//DXVA_PicParams_H264 *pDXVAPicParams->pp;
    av_log(s->avctx, AV_LOG_ERROR, "ff_directshow_h264_picture_complete!\n");
    SPS* cur_sps;
    PPS* cur_pps;
    int                        field_pic_flag;
    int                    hr = -1;

    field_pic_flag = (h->s.picture_structure != PICT_FRAME);

    cur_sps = &h->sps;
    cur_pps = &h->pps;

    if (cur_sps && cur_pps)
    {
        *nFieldType = h->s.picture_structure;
        if (h->sps.pic_struct_present_flag)
        {
            switch (h->sei_pic_struct)
            {
            case SEI_PIC_STRUCT_TOP_FIELD:
            case SEI_PIC_STRUCT_TOP_BOTTOM:
            case SEI_PIC_STRUCT_TOP_BOTTOM_TOP:
                *nFieldType = PICT_TOP_FIELD;
                break;
            case SEI_PIC_STRUCT_BOTTOM_FIELD:
            case SEI_PIC_STRUCT_BOTTOM_TOP:
            case SEI_PIC_STRUCT_BOTTOM_TOP_BOTTOM:
                *nFieldType = PICT_BOTTOM_FIELD;
                break;
            case SEI_PIC_STRUCT_FRAME_DOUBLING:
            case SEI_PIC_STRUCT_FRAME_TRIPLING:
            case SEI_PIC_STRUCT_FRAME:
                *nFieldType = PICT_FRAME;
                break;
            }
        }

        *nSliceType = h->slice_type;

        if (cur_sps->mb_width==0 || cur_sps->mb_height==0) 
		    return;
        pict->picture_params.wFrameWidthInMbsMinus1            = cur_sps->mb_width  - 1;        // pic_width_in_mbs_minus1;
        pict->picture_params.wFrameHeightInMbsMinus1            = cur_sps->mb_height * (2 - cur_sps->frame_mbs_only_flag) - 1;        // pic_height_in_map_units_minus1;
        pict->picture_params.num_ref_frames                    = cur_sps->ref_frame_count;        // num_ref_frames;
        /* DXVA_PicParams_H264 */
        pict->picture_params.wBitFields                      = (field_pic_flag                         <<  0) |
                                                          ((h->sps.mb_aff && (field_pic_flag==0)) <<  1) |
                                                          (cur_sps->residual_color_transform_flag <<  2) |
                                                          /* sp_for_switch_flag (not implemented by FFmpeg) ffdshow added h->sp_for_switch_flag to h264context*/
                                                          (0                                    <<  3) |
                                                          (cur_sps->chroma_format_idc           <<  4) |
                                                          ((h->nal_ref_idc != 0)                      <<  6) | /*was h->ref_pic_flag*/
                                                          (cur_pps->constrained_intra_pred      <<  7) |
                                                          (cur_pps->weighted_pred               <<  8) |
                                                          (cur_pps->weighted_bipred_idc         <<  9) |
                                                          /* MbsConsecutiveFlag */
                                                          (1                                    << 11) |
                                                          (cur_sps->frame_mbs_only_flag         << 12) |
                                                          (cur_pps->transform_8x8_mode          << 13) |
                                                          (h->sps.level_idc >= 31               << 14) |
                                                          /* IntraPicFlag (Modified if we detect a non
                                                           * intra slice in decode_slice) Only used with ffmpeg dxva2*/
                                                          ((h->slice_type == FF_I_TYPE )                                    << 15);

        pict->picture_params.bit_depth_luma_minus8            = cur_sps->bit_depth_luma   - 8;    // bit_depth_luma_minus8
        pict->picture_params.bit_depth_chroma_minus8            = cur_sps->bit_depth_chroma - 8;    // bit_depth_chroma_minus8
    //    pict->picture_params.StatusReportFeedbackNumber        = SET IN DecodeFrame;


    //    pict->picture_params.CurrFieldOrderCnt                        = SET IN UpdateRefFramesList;
    //    pict->picture_params.FieldOrderCntList                        = SET IN UpdateRefFramesList;
    //    pict->picture_params.FrameNumList                            = SET IN UpdateRefFramesList;
    //    pict->picture_params.UsedForReferenceFlags                    = SET IN UpdateRefFramesList;
    //    pict->picture_params.NonExistingFrameFlags
        pict->picture_params.frame_num                        = h->frame_num;
    //    pict->picture_params.SliceGroupMap


        pict->picture_params.log2_max_frame_num_minus4                = cur_sps->log2_max_frame_num - 4;                    // log2_max_frame_num_minus4;
        pict->picture_params.pic_order_cnt_type                        = cur_sps->poc_type;                                // pic_order_cnt_type;
        pict->picture_params.log2_max_pic_order_cnt_lsb_minus4        = cur_sps->log2_max_poc_lsb - 4;                    // log2_max_pic_order_cnt_lsb_minus4;
        pict->picture_params.delta_pic_order_always_zero_flag        = cur_sps->delta_pic_order_always_zero_flag;
        pict->picture_params.direct_8x8_inference_flag                = cur_sps->direct_8x8_inference_flag;
        pict->picture_params.entropy_coding_mode_flag                = cur_pps->cabac;                                    // entropy_coding_mode_flag;
        pict->picture_params.pic_order_present_flag                    = cur_pps->pic_order_present;                        // pic_order_present_flag;
        pict->picture_params.num_slice_groups_minus1                    = cur_pps->slice_group_count - 1;                    // num_slice_groups_minus1;
        pict->picture_params.slice_group_map_type                    = cur_pps->mb_slice_group_map_type;                    // slice_group_map_type;
        pict->picture_params.deblocking_filter_control_present_flag    = cur_pps->deblocking_filter_parameters_present;    // deblocking_filter_control_present_flag;
        pict->picture_params.redundant_pic_cnt_present_flag            = cur_pps->redundant_pic_cnt_present;                // redundant_pic_cnt_present_flag;

        pict->picture_params.slice_group_change_rate_minus1= 0;  /* XXX not implemented by FFmpeg */

        pict->picture_params.chroma_qp_index_offset                    = cur_pps->chroma_qp_index_offset[0];
        pict->picture_params.second_chroma_qp_index_offset            = cur_pps->chroma_qp_index_offset[1];
        pict->picture_params.num_ref_idx_l0_active_minus1            = cur_pps->ref_count[0]-1;                            // num_ref_idx_l0_active_minus1;
        pict->picture_params.num_ref_idx_l1_active_minus1            = cur_pps->ref_count[1]-1;                            // num_ref_idx_l1_active_minus1;
        pict->picture_params.pic_init_qp_minus26                        = cur_pps->init_qp - 26;
        pict->picture_params.pic_init_qs_minus26                        = cur_pps->init_qs - 26;

        if (field_pic_flag)
        {
          unsigned cur_associated_flag = (h->s.picture_structure == PICT_BOTTOM_FIELD);
        /*fill_dxva_pic_entry(DXVA_PicEntry_H264 *pic,unsigned index, unsigned flag)*/

			pict->picture_params.CurrPic.bPicEntry = 0 | (cur_associated_flag << 7) ;

            if (cur_associated_flag)
            {
                // Bottom field
                pict->picture_params.CurrFieldOrderCnt[0] = 0;
                pict->picture_params.CurrFieldOrderCnt[1] = h->poc_lsb + h->poc_msb;
            }
            else
            {
                // Top field
                pict->picture_params.CurrFieldOrderCnt[0] = h->poc_lsb + h->poc_msb;
                pict->picture_params.CurrFieldOrderCnt[1] = 0;
            }
        }
        else
        {

			pict->picture_params.CurrPic.bPicEntry = 0 | (0 << 7);

            pict->picture_params.CurrFieldOrderCnt[0]    = h->poc_lsb + h->poc_msb;
            pict->picture_params.CurrFieldOrderCnt[1]    = h->poc_lsb + h->poc_msb;
        }

        /*CopyScalingMatrix (pict->picture_qmatrix, (DXVA_Qmatrix_H264*)cur_pps->scaling_matrix4, nPCIVendor);*/
		DXVA_Qmatrix_H264* qmatrix_source = (DXVA_Qmatrix_H264*)cur_pps->scaling_matrix4;
        int i,j;
		for (i=0; i<6; i++)
            for (j=0; j<16; j++)
                pict->picture_qmatrix.bScalingLists4x4[i][j] = qmatrix_source->bScalingLists4x4[i][ZZ_SCAN[j]];

        for (i=0; i<2; i++)
            for (j=0; j<64; j++)
                pict->picture_qmatrix.bScalingLists8x8[i][j] = qmatrix_source->bScalingLists8x8[i][ZZ_SCAN8[j]];
		hr = 1;
    }

    return hr;
}

void ff_directshow_h264_set_reference_frames(MpegEncContext *s)
{
av_log(s->avctx, AV_LOG_ERROR, "ff_directshow_h264_set_reference_frames!\n");
}

void ff_directshow_h264_picture_start(MpegEncContext *s)
{
av_log(s->avctx, AV_LOG_ERROR, "ff_directshow_h264_picture_start!\n");
}