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
      if (picture.iWidth > 1024 || picture.iHeight >= 600)
        return Colorimetry::BT709_YCC;
      else
        return Colorimetry::SMPTE_170M_YCC;
  }
}

const char* ColorimetryToString(Colorimetry colorimetry)
{
  // DRM connector "Colorspace" property strings from drm_connector.c
  switch (colorimetry)
  {
    case Colorimetry::DEFAULT:
      return "Default";
    case Colorimetry::SMPTE_170M_YCC:
      return "SMPTE_170M_YCC";
    case Colorimetry::BT709_YCC:
      return "BT709_YCC";
    case Colorimetry::XVYCC_601:
      return "XVYCC_601";
    case Colorimetry::XVYCC_709:
      return "XVYCC_709";
    case Colorimetry::SYCC_601:
      return "SYCC_601";
    case Colorimetry::OPYCC_601:
      return "opYCC_601";
    case Colorimetry::OPRGB:
      return "opRGB";
    case Colorimetry::BT2020_CYCC:
      return "BT2020_CYCC";
    case Colorimetry::BT2020_RGB:
      return "BT2020_RGB";
    case Colorimetry::BT2020_YCC:
      return "BT2020_YCC";
    case Colorimetry::DCI_P3_RGB_D65:
      return "DCI-P3_RGB_D65";
    case Colorimetry::DCI_P3_RGB_THEATER:
      return "DCI-P3_RGB_Theater";
    case Colorimetry::RGB_WIDE_FIXED:
      return "RGB_WIDE_FIXED";
    case Colorimetry::RGB_WIDE_FLOAT:
      return "RGB_WIDE_FLOAT";
    case Colorimetry::BT601_YCC:
      return "BT601_YCC";
    case Colorimetry::ST2113_RGB:
      return "ST2113_RGB";
    case Colorimetry::ICTCP:
      return "ICtCp";
    default:
      return "Default";
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
