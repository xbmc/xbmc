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

VideoPicture::VideoPicture() = default;

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

VideoPicture::VideoPicture(VideoPicture const&) = default;
VideoPicture& VideoPicture::operator=(VideoPicture const&) = default;
