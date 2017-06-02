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

#include <vector>

#include "guilib/GraphicContext.h"
#include "../RenderFlags.h"
#include "../BaseRenderer.h"
#include "../RenderCapture.h"
#include "settings/VideoSettings.h"
#include "cores/VideoPlayer/DVDStreamInfo.h"
#include "cores/VideoPlayer/DVDCodecs/Video/MMALFFmpeg.h"
#include "guilib/Geometry.h"
#include "threads/Thread.h"

#include <interface/mmal/mmal.h>
#include <interface/mmal/util/mmal_util.h>
#include <interface/mmal/util/mmal_default_components.h>
#include <interface/mmal/util/mmal_util_params.h>

#define NOSOURCE   -2
#define AUTOSOURCE -1

// worst case number of buffers. 12 for decoder. 8 for multi-threading in ffmpeg. NUM_BUFFERS for renderer.
// Note, generally these won't necessarily result in allocated pictures
#define MMAL_NUM_OUTPUT_BUFFERS (12 + 8 + NUM_BUFFERS)

class CBaseTexture;
class CMMALBuffer;

struct VideoPicture;

class CMMALPool : public std::enable_shared_from_this<CMMALPool>
{
public:
  CMMALPool(const char *component_name, bool input, uint32_t num_buffers, uint32_t buffer_size, uint32_t encoding, MMALState state);
  ~CMMALPool();
  MMAL_COMPONENT_T *GetComponent() { return m_component; }
  static void AlignedSize(AVCodecContext *avctx, uint32_t &w, uint32_t &h);
  CMMALBuffer *GetBuffer(uint32_t timeout);
  CGPUMEM *AllocateBuffer(uint32_t numbytes);
  void ReleaseBuffer(CGPUMEM *gmem);
  void Close();
  void Prime();
  void SetDecoder(void *dec) { m_dec = dec; }
  void SetProcessInfo(CProcessInfo *processInfo) { m_processInfo = processInfo; }
  void SetFormat(uint32_t mmal_format, uint32_t width, uint32_t height, uint32_t aligned_width, uint32_t aligned_height, uint32_t size, AVCodecContext *avctx)
    { m_mmal_format = mmal_format; m_width = width; m_height = height; m_aligned_width = aligned_width; m_aligned_height = aligned_height; m_size = size, m_avctx = avctx; m_software = true; }
  bool IsSoftware() { return m_software; }
  void SetVideoDeintMethod(std::string method) { if (m_processInfo) m_processInfo->SetVideoDeintMethod(method); }
protected:
  uint32_t m_mmal_format, m_width, m_height, m_aligned_width, m_aligned_height, m_size;
  AVCodecContext *m_avctx;
  void *m_dec;
  MMALState m_state;
  bool m_input;
  MMAL_POOL_T *m_mmal_pool;
  MMAL_COMPONENT_T *m_component;
  CCriticalSection m_section;
  std::deque<CGPUMEM *> m_freeBuffers;
  bool m_closing;
  bool m_software;
  CProcessInfo *m_processInfo;
};

class CMMALRenderer : public CBaseRenderer, public CThread, public IRunnable
{
public:
  CMMALRenderer();
  ~CMMALRenderer();

  void Process();
  virtual void Update();

  bool RenderCapture(CRenderCapture* capture);

  // Player functions
  virtual bool         Configure(const VideoPicture &picture, float fps, unsigned flags, unsigned int orientation) override;
  virtual void         ReleaseBuffer(int idx) override;
  virtual void         FlipPage(int source) override;
  virtual void         UnInit();
  virtual void         Reset() override; /* resets renderer after seek for example */
  virtual void         Flush() override;
  virtual bool         IsConfigured() override { return m_bConfigured; }
  virtual void         AddVideoPicture(const VideoPicture& pic, int index) override;
  virtual bool         IsPictureHW(const VideoPicture &picture) override { return false; };
  virtual CRenderInfo GetRenderInfo() override;

  virtual bool         SupportsMultiPassRendering() override { return false; };
  virtual bool         Supports(ERENDERFEATURE feature) override;
  virtual bool         Supports(ESCALINGMETHOD method) override;

  virtual void         RenderUpdate(bool clear, DWORD flags = 0, DWORD alpha = 255) override;

  virtual void SetVideoRect(const CRect& SrcRect, const CRect& DestRect);
  virtual bool         IsGuiLayer() override { return false; }
  virtual bool         ConfigChanged(const VideoPicture &picture) override { return false; }

  void vout_input_port_cb(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer);
  void deint_input_port_cb(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer);
  void deint_output_port_cb(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer);
protected:
  int m_iYV12RenderBuffer;

  CMMALBuffer         *m_buffers[NUM_BUFFERS];
  bool                 m_bConfigured;
  unsigned int         m_extended_format;
  int                  m_neededBuffers;

  CRect                     m_cachedSourceRect;
  CRect                     m_cachedDestRect;
  CRect                     m_src_rect;
  CRect                     m_dst_rect;
  RENDER_STEREO_MODE        m_video_stereo_mode;
  RENDER_STEREO_MODE        m_display_stereo_mode;
  bool                      m_StereoInvert;

  CCriticalSection m_sharedSection;
  MMAL_COMPONENT_T *m_vout;
  MMAL_PORT_T *m_vout_input;
  MMAL_QUEUE_T *m_queue_render;
  MMAL_QUEUE_T *m_queue_process;
  CThread m_processThread;
  MMAL_BUFFER_HEADER_T m_quitpacket;
  double m_error;
  double m_lastPts;
  double m_frameInterval;
  double m_frameIntervalDiff;
  uint32_t m_vout_width, m_vout_height, m_vout_aligned_width, m_vout_aligned_height;
  // deinterlace
  MMAL_COMPONENT_T *m_deint;
  MMAL_PORT_T *m_deint_input;
  MMAL_PORT_T *m_deint_output;
  std::shared_ptr<CMMALPool> m_deint_output_pool;
  MMAL_INTERLACETYPE_T m_interlace_mode;
  EINTERLACEMETHOD  m_interlace_method;
  uint32_t m_deint_width, m_deint_height, m_deint_aligned_width, m_deint_aligned_height;
  MMAL_FOURCC_T m_deinterlace_out_encoding;
  void DestroyDeinterlace();
  bool CheckConfigurationDeint(uint32_t width, uint32_t height, uint32_t aligned_width, uint32_t aligned_height, uint32_t encoding, EINTERLACEMETHOD interlace_method);

  bool CheckConfigurationVout(uint32_t width, uint32_t height, uint32_t aligned_width, uint32_t aligned_height, uint32_t encoding);
  uint32_t m_vsync_count;
  void ReleaseBuffers();
  void UnInitMMAL();
  void UpdateFramerateStats(double pts);
  virtual void Run() override;
};
