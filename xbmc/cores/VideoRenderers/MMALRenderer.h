#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "guilib/GraphicContext.h"
#include "RenderFlags.h"
#include "RenderFormats.h"
#include "BaseRenderer.h"
#include "RenderCapture.h"
#include "settings/VideoSettings.h"
#include "cores/dvdplayer/DVDStreamInfo.h"
#include "guilib/Geometry.h"
#include "BaseRenderer.h"

#include <interface/mmal/mmal.h>
#include <interface/mmal/util/mmal_util.h>
#include <interface/mmal/util/mmal_default_components.h>
#include <interface/mmal/util/mmal_util_params.h>

#define AUTOSOURCE -1

class CBaseTexture;
class CMMALVideoBuffer;

struct DVDVideoPicture;

class CMMALRenderer : public CBaseRenderer
{
  struct YUVBUFFER
  {
    CMMALVideoBuffer *MMALBuffer; // used for hw decoded buffers
    MMAL_BUFFER_HEADER_T *mmal_buffer;  // used for sw decoded buffers
  };
public:
  CMMALRenderer();
  ~CMMALRenderer();

  virtual void Update();
  virtual void SetupScreenshot() {};

  bool RenderCapture(CRenderCapture* capture);

  // Player functions
  virtual bool         Configure(unsigned int width, unsigned int height, unsigned int d_width, unsigned int d_height, float fps, unsigned flags, ERenderFormat format, unsigned extended_format, unsigned int orientation);
  virtual int          GetImage(YV12Image *image, int source = AUTOSOURCE, bool readonly = false);
  virtual void         ReleaseImage(int source, bool preserve = false);
  virtual void         ReleaseBuffer(int idx);
  virtual void         FlipPage(int source);
  virtual unsigned int PreInit();
  virtual void         UnInit();
  virtual void         Reset(); /* resets renderer after seek for example */
  virtual void         Flush();
  virtual bool         IsConfigured() { return m_bConfigured; }
  virtual void         AddProcessor(CMMALVideoBuffer *buffer, int index);
  virtual CRenderInfo GetRenderInfo();

  virtual bool         Supports(ERENDERFEATURE feature);
  virtual bool         Supports(EDEINTERLACEMODE mode);
  virtual bool         Supports(EINTERLACEMETHOD method);
  virtual bool         Supports(ESCALINGMETHOD method);

  virtual EINTERLACEMETHOD AutoInterlaceMethod();

  void                 RenderUpdate(bool clear, DWORD flags = 0, DWORD alpha = 255);

  virtual void         SetBufferSize(int numBuffers) { m_NumYV12Buffers = numBuffers; }
  virtual void SetVideoRect(const CRect& SrcRect, const CRect& DestRect);
  virtual bool         IsGuiLayer() { return false; }

  void vout_input_port_cb(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer);
protected:
  int m_iYV12RenderBuffer;
  int m_NumYV12Buffers;

  std::vector<ERenderFormat> m_formats;

  YUVBUFFER            m_buffers[NUM_BUFFERS];
  bool                 m_bConfigured;
  bool                 m_bMMALConfigured;
  unsigned int         m_extended_format;
  unsigned int         m_destWidth;
  unsigned int         m_destHeight;
  int                  m_neededBuffers;

  CRect                     m_src_rect;
  CRect                     m_dst_rect;
  RENDER_STEREO_MODE        m_video_stereo_mode;
  RENDER_STEREO_MODE        m_display_stereo_mode;
  bool                      m_StereoInvert;

  CCriticalSection m_sharedSection;
  MMAL_COMPONENT_T *m_vout;
  MMAL_PORT_T *m_vout_input;
  MMAL_POOL_T *m_vout_input_pool;

  bool init_vout(ERenderFormat format);
  void ReleaseBuffers();
  void UnInitMMAL();
};
