/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DVDVideoCodec.h"

#include "cores/VideoPlayer/DVDCodecs/DVDFactoryCodec.h"

#include <string>
#include <vector>

//******************************************************************************
// VideoPicture
//******************************************************************************
VideoPicture::~VideoPicture()
{
  if (videoBuffer)
  {
    videoBuffer->Release();
  }
}

void VideoPicture::Reset()
{
  if (videoBuffer)
    videoBuffer->Release();
  videoBuffer = nullptr;
  pts = DVD_NOPTS_VALUE;
  dts = DVD_NOPTS_VALUE;
  iFlags = 0;
  iRepeatPicture = 0;
  iDuration = 0;
  iFrameType = 0;
  color_space = AVCOL_SPC_UNSPECIFIED;
  color_range = 0;
  chroma_position = AVCHROMA_LOC_UNSPECIFIED;
  color_primaries = AVColorPrimaries::AVCOL_PRI_UNSPECIFIED;
  color_transfer = AVCOL_TRC_UNSPECIFIED;
  colorBits = 8;
  stereoMode.clear();

  qp_table = nullptr;
  qstride = 0;
  qscale_type = 0;
  pict_type = 0;

  hdrType = StreamHdrType::HDR_TYPE_NONE;

  hasDisplayMetadata = false;
  hasLightMetadata = false;

  iWidth = 0;
  iHeight = 0;
  iDisplayWidth = 0;
  iDisplayHeight = 0;
}

VideoPicture& VideoPicture::CopyRef(const VideoPicture &pic)
{
  if (videoBuffer)
    videoBuffer->Release();
  *this = pic;
  if (videoBuffer)
    videoBuffer->Acquire();
  return *this;
}

VideoPicture& VideoPicture::SetParams(const VideoPicture &pic)
{
  if (videoBuffer)
    videoBuffer->Release();
  *this = pic;
  videoBuffer = nullptr;
  return *this;
}

bool VideoPicture::CompareDisplayMetadata(const VideoPicture& pic) const
{
  if (this->hasDisplayMetadata != pic.hasDisplayMetadata)
    return false;

  // both this and pic not has display metadata (e.g. SDR video)
  // returns true because it is equal and there is no need to compare
  if (!pic.hasDisplayMetadata)
    return true;

  // both this and pic has display metadata
  return this->displayMetadata.max_luminance.num == pic.displayMetadata.max_luminance.num &&
         this->displayMetadata.max_luminance.den == pic.displayMetadata.max_luminance.den &&
         this->displayMetadata.min_luminance.num == pic.displayMetadata.min_luminance.num &&
         this->displayMetadata.min_luminance.den == pic.displayMetadata.min_luminance.den;
}

bool VideoPicture::CompareLightMetadata(const VideoPicture& pic) const
{
  if (this->hasLightMetadata != pic.hasLightMetadata)
    return false;

  // both this and pic not has light metadata (e.g. SDR video)
  // returns true because it is equal and there is no need to compare
  if (!pic.hasLightMetadata)
    return true;

  // both this and pic has light metadata
  return this->lightMetadata.MaxCLL == pic.lightMetadata.MaxCLL &&
         this->lightMetadata.MaxFALL == pic.lightMetadata.MaxFALL;
}

bool VideoPicture::IsSameParams(const VideoPicture& pic) const
{
  return this->iWidth == pic.iWidth && this->iHeight == pic.iHeight &&
         this->iDisplayWidth == pic.iDisplayWidth && this->iDisplayHeight == pic.iDisplayHeight &&
         this->stereoMode == pic.stereoMode && this->color_primaries == pic.color_primaries &&
         this->color_transfer == pic.color_transfer && this->hdrType == pic.hdrType &&
         CompareDisplayMetadata(pic) && CompareLightMetadata(pic);
}
