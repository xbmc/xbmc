/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2001             *
 * by the XIPHOPHORUS Company http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

 function: vorbis encode-engine setup
 last mod: $Id: vorbisenc.h,v 1.10 2002/07/01 11:20:10 xiphmont Exp $

 ********************************************************************/

#ifndef _OV_ENC_H_
#define _OV_ENC_H_

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#include "codec.h"

extern int vorbis_encode_init(vorbis_info *vi,
			      long channels,
			      long rate,
			      
			      long max_bitrate,
			      long nominal_bitrate,
			      long min_bitrate);

extern int vorbis_encode_setup_managed(vorbis_info *vi,
				       long channels,
				       long rate,
				       
				       long max_bitrate,
				       long nominal_bitrate,
				       long min_bitrate);
  
extern int vorbis_encode_setup_vbr(vorbis_info *vi,
				  long channels,
				  long rate,
				  
				  float /* quality level from 0. (lo) to 1. (hi) */
				  );

extern int vorbis_encode_init_vbr(vorbis_info *vi,
				  long channels,
				  long rate,
				  
				  float base_quality /* quality level from 0. (lo) to 1. (hi) */
				  );

extern int vorbis_encode_setup_init(vorbis_info *vi);

extern int vorbis_encode_ctl(vorbis_info *vi,int number,void *arg);

#define OV_ECTL_RATEMANAGE_GET       0x10

#define OV_ECTL_RATEMANAGE_SET       0x11
#define OV_ECTL_RATEMANAGE_AVG       0x12
#define OV_ECTL_RATEMANAGE_HARD      0x13

#define OV_ECTL_LOWPASS_GET          0x20
#define OV_ECTL_LOWPASS_SET          0x21

#define OV_ECTL_IBLOCK_GET           0x30
#define OV_ECTL_IBLOCK_SET           0x31

struct ovectl_ratemanage_arg {
  int    management_active;

  long   bitrate_hard_min;
  long   bitrate_hard_max;
  double bitrate_hard_window;

  long   bitrate_av_lo;
  long   bitrate_av_hi;
  double bitrate_av_window;
  double bitrate_av_window_center;
};

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif


