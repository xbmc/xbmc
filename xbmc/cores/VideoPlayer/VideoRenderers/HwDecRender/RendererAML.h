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

#if defined(HAS_LIBAMCODEC)

#include "cores/VideoPlayer/VideoRenderers/BaseRenderer.h"

class CRendererAML : public CBaseRenderer
{
public:
  CRendererAML();
  virtual ~CRendererAML();

  virtual bool RenderCapture(CRenderCapture* capture);
  virtual void AddVideoPictureHW(VideoPicture &picture, int index);
  virtual void ReleaseBuffer(int idx);
  virtual bool Configure(unsigned int width, unsigned int height, unsigned int d_width, unsigned int d_height, float fps, unsigned flags, ERenderFormat format, void *hwPic, unsigned int orientation);
  virtual bool IsConfigured(){ return m_bConfigured; };
  virtual CRenderInfo GetRenderInfo();
  virtual int GetImage(YuvImage *image, int source = -1, bool readonly = false);
  virtual void ReleaseImage(int source, bool preserve = false){};
  virtual void FlipPage(int source);
  virtual void PreInit(){};
  virtual void UnInit(){};
  virtual void Reset();
  virtual void Update(){};
  virtual void RenderUpdate(bool clear, unsigned int flags = 0, unsigned int alpha = 255);
  virtual bool SupportsMultiPassRendering(){ return false; };

  // Player functions
  virtual bool IsGuiLayer();

  // Feature support
  virtual bool Supports(EINTERLACEMETHOD method);
  virtual bool Supports(ESCALINGMETHOD method);
  virtual bool Supports(ERENDERFEATURE feature);


  virtual EINTERLACEMETHOD AutoInterlaceMethod();

private:

  int m_iRenderBuffer;
  static const int m_numRenderBuffers = 4;

  struct BUFFER
  {
    void *hwDec;
    int duration;
  } m_buffers[m_numRenderBuffers];

  int m_prevVPts;
  bool m_bConfigured;
};

#endif
