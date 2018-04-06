/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "LinuxRendererGLESBase.h"

class CLinuxRendererGLES3 : public CLinuxRendererGLESBase
{
public:
  // Registration
  static CBaseRenderer* Create(CVideoBuffer *buffer);
  static bool Register();

protected:
  // LinuxRendererGLESBase overrides
  bool CreateYV12Texture(int index) override;
  bool UploadYV12Texture(int index) override;

  bool CreateNV12Texture(int index) override;
  bool UploadNV12Texture(int index) override;

  void LoadPlane(CYuvPlane& plane, int type, unsigned width,  unsigned height, int stride, int bpp, void* data) override;
};
