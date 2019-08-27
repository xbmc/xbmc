/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

enum STREAMCODEC_PROFILE
{
  CodecProfileUnknown = 0,
  CodecProfileNotNeeded,
  H264CodecProfileBaseline,
  H264CodecProfileMain,
  H264CodecProfileExtended,
  H264CodecProfileHigh,
  H264CodecProfileHigh10,
  H264CodecProfileHigh422,
  H264CodecProfileHigh444Predictive,
  VP9CodecProfile0 = 20,
  VP9CodecProfile1,
  VP9CodecProfile2,
  VP9CodecProfile3,
};
