/*
 *      Copyright (C) 2007-2015 Team XBMC
 *      http://xbmc.org
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

#pragma once

#include "system.h"

#include "cores/VideoPlayer/VideoRenderers/LinuxRendererGL.h"

class CRendererVAAPI : public CLinuxRendererGL
{
public:
  CRendererVAAPI();
  virtual ~CRendererVAAPI();

  virtual bool Configure(const VideoPicture &picture, float fps, unsigned flags, unsigned int orientation) override;

  // Player functions
  virtual void AddVideoPicture(const VideoPicture &picture, int index) override;
  virtual void ReleaseBuffer(int idx) override;
  virtual bool ConfigChanged(const VideoPicture &picture) override;

  static bool HandlesVideoBuffer(const VideoPicture &picture);

  // Feature support
  virtual bool Supports(ERENDERFEATURE feature) override;
  virtual bool Supports(ESCALINGMETHOD method) override;

protected:
  virtual bool LoadShadersHook() override;
  virtual bool RenderHook(int idx) override;
  virtual void AfterRenderHook(int idx) override;

  // textures
  virtual bool UploadTexture(int index) override;
  virtual void DeleteTexture(int index) override;
  virtual bool CreateTexture(int index) override;

  virtual EShaderFormat GetShaderFormat() override;

  bool m_isVAAPIBuffer = true;
};

