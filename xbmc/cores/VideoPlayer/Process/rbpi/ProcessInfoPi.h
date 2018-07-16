/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/IPlayer.h"
#include "cores/VideoPlayer/Process/ProcessInfo.h"

namespace MMAL {
  class CMMALYUVBuffer;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

class CVideoBufferManagerPi : public CVideoBufferManager
{
public:
  CVideoBufferManagerPi();
  void RegisterPool(std::shared_ptr<IVideoBufferPool> pool);
  void ReleasePools();
  CVideoBuffer* Get(AVPixelFormat format, int width, int height);
  CVideoBuffer* Get(AVPixelFormat format, int size);
  void SetDimensions(int width, int height, const int (&strides)[YuvImage::MAX_PLANES]);

protected:
};

class CProcessInfoPi : public CProcessInfo
{
public:
  CProcessInfoPi();
  static CProcessInfo* Create();
  static void Register();
  EINTERLACEMETHOD GetFallbackDeintMethod() override;
  bool AllowDTSHDDecode() override;

//protected:
};
