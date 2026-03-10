/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "HDRUtils.h"

extern "C"
{
#include <libavcodec/avcodec.h>
}

namespace KODI
{
namespace UTILS
{

Colorimetry GetColorimetry(const VideoPicture& picture)
{
  switch (picture.color_space)
  {
    case AVCOL_SPC_BT2020_CL:
      return Colorimetry::BT2020_CYCC;
    case AVCOL_SPC_BT2020_NCL:
      return Colorimetry::BT2020_YCC;
    case AVCOL_SPC_BT709:
      return Colorimetry::BT709_YCC;
    case AVCOL_SPC_SMPTE170M:
    case AVCOL_SPC_BT470BG:
      return Colorimetry::SMPTE_170M_YCC;
    default:
      return Colorimetry::DEFAULT;
  }
}

Eotf GetEOTF(const VideoPicture& picture)
{
  switch (picture.color_transfer)
  {
    case AVCOL_TRC_SMPTE2084:
      return Eotf::PQ;
    case AVCOL_TRC_ARIB_STD_B67:
    case AVCOL_TRC_BT2020_10:
      return Eotf::HLG;
    default:
      return Eotf::TRADITIONAL_SDR;
  }
}

const AVMasteringDisplayMetadata* GetMasteringDisplayMetadata(const VideoPicture& picture)
{
  return picture.hasDisplayMetadata ? &picture.displayMetadata : nullptr;
}

const AVContentLightMetadata* GetContentLightMetadata(const VideoPicture& picture)
{
  return picture.hasLightMetadata ? &picture.lightMetadata : nullptr;
}

} // namespace UTILS
} // namespace KODI
