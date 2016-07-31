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

class CRendererMediaCodecSurface : public CLinuxRendererGLES
{
public:
  CRendererMediaCodecSurface();
  virtual ~CRendererMediaCodecSurface();
  
  virtual bool RenderCapture(CRenderCapture* capture);

  // Player functions
  virtual void AddVideoPictureHW(DVDVideoPicture &picture, int index);
  virtual void ReleaseBuffer(int idx);
  virtual bool IsGuiLayer();

  // Feature support
  virtual bool Supports(EINTERLACEMETHOD method);

  virtual EINTERLACEMETHOD AutoInterlaceMethod();
  virtual CRenderInfo GetRenderInfo();

protected:

  // textures
  virtual bool UploadTexture(int index);
  virtual void DeleteTexture(int index);
  virtual bool CreateTexture(int index);
  
  // hooks for hw dec renderer
  virtual bool LoadShadersHook();
  virtual bool RenderHook(int index);  
  virtual int  GetImageHook(YV12Image *image, int source = AUTOSOURCE, bool readonly = false);
  virtual bool RenderUpdateVideoHook(bool clear, DWORD flags = 0, DWORD alpha = 255);
};

#endif
