/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/VideoPlayer/DVDCodecs/Video/DVDVideoCodec.h"
#include "utils/DisplayInfo.h"

extern "C"
{
#include <libavutil/mastering_display_metadata.h>
}

namespace KODI
{
namespace UTILS
{

// HDR enums copied from linux include/linux/hdmi.h (not part of uapi)
enum hdmi_metadata_type
{
  HDMI_STATIC_METADATA_TYPE1 = 0,
};

Colorimetry GetColorimetry(const VideoPicture& picture);
const char* ColorimetryToString(Colorimetry colorimetry);
Eotf GetEOTF(const VideoPicture& picture);
const AVMasteringDisplayMetadata* GetMasteringDisplayMetadata(const VideoPicture& picture);
const AVContentLightMetadata* GetContentLightMetadata(const VideoPicture& picture);

} // namespace UTILS
} // namespace KODI
