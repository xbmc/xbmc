/*
 *      Copyright (C) 2007-2015 Team Kodi
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include "system.h"

#if defined(TARGET_ANDROID)

#include "cores/VideoPlayer/VideoRenderers/LinuxRendererGLES.h"

class CRendererMediaCodec : public CLinuxRendererGLES
{
public:
  CRendererMediaCodec();
  virtual ~CRendererMediaCodec();

  // Registration
  static CBaseRenderer* Create(CVideoBuffer *buffer);
  static bool Register();

  // Player functions
  virtual void AddVideoPicture(const VideoPicture &picture, int index) override;
  virtual void ReleaseBuffer(int idx) override;

  // Feature support
  virtual CRenderInfo GetRenderInfo() override;

protected:
  // textures
  virtual bool UploadTexture(int index) override;
  virtual void DeleteTexture(int index) override;
  virtual bool CreateTexture(int index) override;

  // hooks for hw dec renderer
  virtual bool LoadShadersHook() override;
  virtual bool RenderHook(int index) override;

private:
  float m_textureMatrix[16];
};

#endif
