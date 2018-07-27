/*
 *  Copyright (C) 2007-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/VideoPlayer/VideoRenderers/LinuxRendererGL.h"


class CRendererVTB : public CLinuxRendererGL
{
public:
  CRendererVTB();
  virtual ~CRendererVTB();

  static CBaseRenderer* Create(CVideoBuffer *buffer);
  static bool Register();

  // Player functions
  virtual void ReleaseBuffer(int idx) override;
  virtual bool NeedBuffer(int idx) override;

protected:
  virtual bool LoadShadersHook() override;
  virtual void AfterRenderHook(int idx) override;
  virtual EShaderFormat GetShaderFormat() override;

  // textures
  virtual bool UploadTexture(int index) override;
  virtual void DeleteTexture(int index) override;
  virtual bool CreateTexture(int index) override;
};

