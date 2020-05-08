/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "pvr_defines.h"

#ifdef BUILD_KODI_ADDON
#include "../../../DemuxPacket.h"
#else
#include "cores/VideoPlayer/Interface/Addon/DemuxPacket.h"
#endif

#include <stdint.h>
#include <time.h>

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

  #define PVR_STREAM_MAX_STREAMS 20
  #define PVR_INVALID_CODEC_ID 0
  #define PVR_INVALID_CODEC \
    { \
      PVR_CODEC_TYPE_UNKNOWN, PVR_INVALID_CODEC_ID \
    }

  typedef enum PVR_CODEC_TYPE
  {
    PVR_CODEC_TYPE_UNKNOWN = -1,
    PVR_CODEC_TYPE_VIDEO,
    PVR_CODEC_TYPE_AUDIO,
    PVR_CODEC_TYPE_DATA,
    PVR_CODEC_TYPE_SUBTITLE,
    PVR_CODEC_TYPE_RDS,

    PVR_CODEC_TYPE_NB
  } PVR_CODEC_TYPE;

  typedef struct PVR_CODEC
  {
    enum PVR_CODEC_TYPE codec_type;
    unsigned int codec_id;
  } PVR_CODEC;

  typedef struct PVR_STREAM_PROPERTIES
  {
    unsigned int iStreamCount;
    struct PVR_STREAM
    {
      unsigned int iPID;
      enum PVR_CODEC_TYPE iCodecType;
      unsigned int iCodecId;
      char strLanguage[4];
      int iSubtitleInfo;
      int iFPSScale;
      int iFPSRate;
      int iHeight;
      int iWidth;
      float fAspect;
      int iChannels;
      int iSampleRate;
      int iBlockAlign;
      int iBitRate;
      int iBitsPerSample;
    } stream[PVR_STREAM_MAX_STREAMS];
  } PVR_STREAM_PROPERTIES;

  typedef struct PVR_STREAM_TIMES
  {
    time_t startTime;
    int64_t ptsStart;
    int64_t ptsBegin;
    int64_t ptsEnd;
  } PVR_STREAM_TIMES;

#ifdef __cplusplus
}
#endif /* __cplusplus */
