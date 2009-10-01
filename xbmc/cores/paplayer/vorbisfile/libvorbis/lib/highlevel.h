/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2007             *
 * by the Xiph.Org Foundation http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

 function: highlevel encoder setup struct seperated out for vorbisenc clarity
 last mod: $Id: highlevel.h 13293 2007-07-24 00:09:47Z xiphmont $

 ********************************************************************/

typedef struct highlevel_byblocktype {
  double tone_mask_setting;
  double tone_peaklimit_setting;
  double noise_bias_setting;
  double noise_compand_setting;
} highlevel_byblocktype;
  
typedef struct highlevel_encode_setup {
  void *setup;
  int   set_in_stone;

  double base_setting;
  double long_setting;
  double short_setting;
  double impulse_noisetune;

  int    managed;
  long   bitrate_min;
  long   bitrate_av;
  double bitrate_av_damp;
  long   bitrate_max;
  long   bitrate_reservoir;
  double bitrate_reservoir_bias;
  
  int impulse_block_p;
  int noise_normalize_p;

  double stereo_point_setting;
  double lowpass_kHz;

  double ath_floating_dB;
  double ath_absolute_dB;

  double amplitude_track_dBpersec;
  double trigger_setting;
  
  highlevel_byblocktype block[4]; /* padding, impulse, transition, long */

} highlevel_encode_setup;

