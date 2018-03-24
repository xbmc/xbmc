/*
 *      Copyright (C) 2010-2013 Team XBMC
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "DVDVideoCodec.h"
#include "ServiceBroker.h"
#include "cores/VideoPlayer/DVDCodecs/DVDFactoryCodec.h"
#include "rendering/RenderSystem.h"
#include "settings/Settings.h"
#include "settings/lib/Setting.h"
#include "windowing/WinSystem.h"
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
  chroma_position = 0;
  color_primaries = 0;
  color_transfer = 0;
  colorBits = 8;
  stereoMode.clear();

  qp_table = nullptr;
  qstride = 0;
  qscale_type = 0;
  pict_type = 0;

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
