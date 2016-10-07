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

#if defined(TARGET_DARWIN_IOS)

#include "cores/VideoPlayer/VideoRenderers/LinuxRendererGLES.h"
#include <CoreVideo/CVOpenGLESTextureCache.h>

class CRendererVTB : public CLinuxRendererGLES
{
public:
  CRendererVTB();
  virtual ~CRendererVTB();

  // Feature support
  virtual void AddVideoPictureHW(DVDVideoPicture &picture, int index) override;
  virtual void ReleaseBuffer(int idx) override;
  virtual bool NeedBuffer(int idx) override;
  virtual CRenderInfo GetRenderInfo() override;

protected:
  // hooks for hw dec renderer
  virtual bool LoadShadersHook() override;
  virtual int  GetImageHook(YV12Image *image, int source = AUTOSOURCE, bool readonly = false) override;
  virtual void AfterRenderHook(int idx) override;

  // textures
  virtual bool UploadTexture(int index) override;
  virtual void DeleteTexture(int index) override;
  virtual bool CreateTexture(int index) override;

  CVOpenGLESTextureCacheRef m_textureCache;
  struct CRenderBuffer
  {
    CVOpenGLESTextureRef m_textureY;
    CVOpenGLESTextureRef m_textureUV;
    CVBufferRef m_videoBuffer;
    GLsync m_fence;
  };
  CRenderBuffer m_vtbBuffers[NUM_BUFFERS];
};

#endif
