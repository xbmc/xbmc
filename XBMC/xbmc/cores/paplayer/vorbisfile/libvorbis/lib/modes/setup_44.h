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

 function: toplevel settings for 44.1/48kHz
 last mod: $Id: setup_44.h,v 1.9 2002/07/11 08:57:29 xiphmont Exp $

 ********************************************************************/

#include "modes/floor_all.h"
#include "modes/residue_44.h"
#include "modes/psych_44.h"

static double rate_mapping_44_stereo_low[2]={
  22500.,32000.
};

static double rate_mapping_44_stereo[11]={
  32000.,40000.,48000.,56000.,64000.,
  80000.,96000.,112000.,128000.,160000.,250001.
};
static double quality_mapping_44_stereo_low[2]={
  -.1,.0,
};
static double quality_mapping_44[11]={
  .0,.1,.2,.3,.4,.5,.6,.7,.8,.9,1.0
};

static int blocksize_short_44_low[1]={
  512
};
static int blocksize_long_44_low[1]={
  4096
};

static int blocksize_short_44[10]={
  256,256,256,256,256,256,256,256,256,256
};
static int blocksize_long_44[10]={
  2048,2048,2048,2048,2048,2048,2048,2048,2048,2048
};
static double _psy_compand_short_mapping[11]={
  1., 1., 1.3, 1.6, 2., 2., 2., 2., 2., 2., 2.
};
static double _psy_compand_long_mapping[11]={
  4., 4., 4.3, 4.6, 5., 5., 5., 5., 5., 5., 5.
};
static double _global_mapping_44[11]={
  1., 1., 1.5, 2., 2., 2.5, 2.7, 3.0, 3.5, 4., 4.
};
static double _global_mapping_44_low[2]={
  0., 1.,
};
static int _floor_short_mapping_44_low[1]={
  1,
};
static int _floor_long_mapping_44_low[1]={
  8
};
static int _floor_short_mapping_44[10]={
  0,0,2,2,4,5,5,5,5,5
};
static int _floor_long_mapping_44[10]={
  7,7,7,7,7,7,7,7,7,7
};

ve_setup_data_template ve_setup_44_stereo={
  10,
  rate_mapping_44_stereo,
  quality_mapping_44,
  2,
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
  _noise_thresh_44,

  _psy_ath_floater,
  _psy_ath_abs,
  
  _psy_lowpass_44,

  _psy_global_44,
  _global_mapping_44,
  _psy_stereo_modes_44,

  _floor_books,
  _floor,
  _floor_short_mapping_44,
  _floor_long_mapping_44,

  _mapres_template_44_stereo
};

ve_setup_data_template ve_setup_44_stereo_low={
  1,
  rate_mapping_44_stereo_low,
  quality_mapping_44_stereo_low,
  2,
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
  _noise_thresh_44,

  _psy_ath_floater,
  _psy_ath_abs,
  
  _psy_lowpass_44_low,

  _psy_global_44,
  _global_mapping_44_low,
  _psy_stereo_modes_44_low,

  _floor_books,
  _floor,
  _floor_short_mapping_44_low,
  _floor_long_mapping_44_low,

  _mapres_template_44_stereo
};

