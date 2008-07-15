/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2002             *
 * by the XIPHOPHORUS Company http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

 function: toplevel settings for 44.1/48kHz uncoupled modes
 last mod: $Id: setup_44u.h,v 1.3 2002/07/11 06:41:05 xiphmont Exp $

 ********************************************************************/

#include "modes/residue_44u.h"

static double rate_mapping_44_un[11]={
  48000.,60000.,70000.,80000.,86000.,
  96000.,110000.,120000.,140000.,160000.,240001.
};

static double rate_mapping_44_un_low[2]={
  32000.,48000.
};

ve_setup_data_template ve_setup_44_uncoupled={
  10,
  rate_mapping_44_un,
  quality_mapping_44,
  -1,
  40000,
  50000,
  
  blocksize_short_44,
  blocksize_long_44,

  _psy_tone_masteratt_44,
  _psy_tone_0dB,
  _psy_tone_suppress,

  _vp_tonemask_adj_otherblock,
  _vp_tonemask_adj_longblock,
  _vp_tonemask_adj_otherblock,

  _psy_noiseguards_44,
  _psy_noisebias_impulse,
  _psy_noisebias_padding,
  _psy_noisebias_trans,
  _psy_noisebias_long,
  _psy_noise_suppress,
  
  _psy_compand_44,
  _psy_compand_short_mapping,
  _psy_compand_long_mapping,

  {_noise_start_short_44,_noise_start_long_44},
  {_noise_part_short_44,_noise_part_long_44},
  _noise_thresh_44_2,

  _psy_ath_floater,
  _psy_ath_abs,
  
  _psy_lowpass_44,

  _psy_global_44,
  _global_mapping_44,
  NULL,

  _floor_books,
  _floor,
  _floor_short_mapping_44,
  _floor_long_mapping_44,

  _mapres_template_44_uncoupled
};

ve_setup_data_template ve_setup_44_uncoupled_low={
  1,
  rate_mapping_44_un_low,
  quality_mapping_44_stereo_low,
  -1,
  40000,
  50000,
  
  blocksize_short_44_low,
  blocksize_long_44_low,

  _psy_tone_masteratt_44_low,
  _psy_tone_0dB,
  _psy_tone_suppress,

  _vp_tonemask_adj_otherblock,
  _vp_tonemask_adj_longblock,
  _vp_tonemask_adj_otherblock,

  _psy_noiseguards_44,
  _psy_noisebias_trans_low,
  _psy_noisebias_trans_low,
  _psy_noisebias_trans_low,
  _psy_noisebias_long_low,
  _psy_noise_suppress,
  
  _psy_compand_44,
  _psy_compand_short_mapping,
  _psy_compand_long_mapping,

  {_noise_start_short_44_low,_noise_start_long_44_low},
  {_noise_part_short_44,_noise_part_long_44},
  _noise_thresh_44_2,

  _psy_ath_floater,
  _psy_ath_abs,
  
  _psy_lowpass_44_low,

  _psy_global_44,
  _global_mapping_44_low,
  NULL,

  _floor_books,
  _floor,
  _floor_short_mapping_44_low,
  _floor_long_mapping_44_low,

  _mapres_template_44_uncoupled
};
