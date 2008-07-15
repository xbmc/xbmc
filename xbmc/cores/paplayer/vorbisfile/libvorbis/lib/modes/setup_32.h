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

 function: toplevel settings for 32kHz
 last mod: $Id: setup_32.h,v 1.4 2002/07/13 06:12:49 xiphmont Exp $

 ********************************************************************/

static double rate_mapping_32[11]={
  28000.,35000.,45000.,56000.,60000.,
  75000.,90000.,100000.,115000.,150000.,190000.,
};

static double rate_mapping_32_un[11]={
  42000.,52000.,64000.,72000.,78000.,
  86000.,92000.,110000.,120000.,140000.,190000.,
};

static double rate_mapping_32_low[2]={
  20000.,28000.
};

static double rate_mapping_32_un_low[2]={
  24000.,42000.,
};

static double _psy_lowpass_32_low[2]={
  13.,13.,
};
static double _psy_lowpass_32[11]={
  13.,13.,14.,15.,99.,99.,99.,99.,99.,99.,99.
};

ve_setup_data_template ve_setup_32_stereo={
  10,
  rate_mapping_32,
  quality_mapping_44,
  2,
  26000,
  40000,
  
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
  _noise_thresh_44,

  _psy_ath_floater,
  _psy_ath_abs,
  
  _psy_lowpass_32,

  _psy_global_44,
  _global_mapping_44,
  _psy_stereo_modes_44,

  _floor_books,
  _floor,
  _floor_short_mapping_44,
  _floor_long_mapping_44,

  _mapres_template_44_stereo
};

ve_setup_data_template ve_setup_32_uncoupled={
  10,
  rate_mapping_32_un,
  quality_mapping_44,
  -1,
  26000,
  40000,
  
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
  
  _psy_lowpass_32,

  _psy_global_44,
  _global_mapping_44,
  NULL,

  _floor_books,
  _floor,
  _floor_short_mapping_44,
  _floor_long_mapping_44,

  _mapres_template_44_uncoupled
};

ve_setup_data_template ve_setup_32_stereo_low={
  1,
  rate_mapping_32_low,
  quality_mapping_44_stereo_low,
  2,
  26000,
  40000,
  
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
  _noise_thresh_44,

  _psy_ath_floater,
  _psy_ath_abs,
  
  _psy_lowpass_32_low,

  _psy_global_44,
  _global_mapping_44,
  _psy_stereo_modes_44_low,

  _floor_books,
  _floor,
  _floor_short_mapping_44_low,
  _floor_long_mapping_44_low,

  _mapres_template_44_stereo
};


ve_setup_data_template ve_setup_32_uncoupled_low={
  1,
  rate_mapping_32_un_low,
  quality_mapping_44_stereo_low,
  -1,
  26000,
  40000,
  
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
  
  _psy_lowpass_32_low,

  _psy_global_44,
  _global_mapping_44,
  NULL,

  _floor_books,
  _floor,
  _floor_short_mapping_44_low,
  _floor_long_mapping_44_low,

  _mapres_template_44_uncoupled
};
