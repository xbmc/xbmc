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

#include <interface/mmal/mmal.h>

#include "guilib/GraphicContext.h"
#include "../RenderFlags.h"
#include "../BaseRenderer.h"
#include "../RenderCapture.h"
#include "settings/VideoSettings.h"
#include "cores/VideoPlayer/DVDStreamInfo.h"
#include "guilib/Geometry.h"
#include "threads/Thread.h"

// worst case number of buffers. 12 for decoder. 8 for multi-threading in ffmpeg. NUM_BUFFERS for renderer.
// Note, generally these won't necessarily result in allocated pictures
#define MMAL_NUM_OUTPUT_BUFFERS (12 + 8 + NUM_BUFFERS)

struct VideoPicture;
class CProcessInfo;

namespace MMAL {

class CMMALBuffer;

enum MMALState { MMALStateNone, MMALStateHWDec, MMALStateFFDec, MMALStateDeint, };

class CMMALPool : public IVideoBufferPool
{
public:
  CMMALPool(const char *component_name, bool input, uint32_t num_buffers, uint32_t buffer_size, uint32_t encoding, MMALState state);
  ~CMMALPool();

  virtual CVideoBuffer* Get() override;
  virtual void Return(int id) override;
  virtual void Configure(AVPixelFormat format, int size) override;
  virtual bool IsConfigured() override;
  virtual bool IsCompatible(AVPixelFormat format, int size) override;

  void SetDimensions(int width, int height, int alignedWidth, int alignedHeight);
  MMAL_COMPONENT_T *GetComponent() { return m_component; }
  CMMALBuffer *GetBuffer(uint32_t timeout);
  void Prime();
  void SetProcessInfo(CProcessInfo *processInfo) { m_processInfo = processInfo; }
  void Configure(AVPixelFormat format, int width, int height, int alignedWidth, int alignedHeight, int size);
  bool IsSoftware() { return m_software; }
  void SetVideoDeintMethod(std::string method);
  static uint32_t TranslateFormat(AVPixelFormat pixfmt);
  virtual int Width() { return m_width; }
  virtual int Height() { return m_height; }
  virtual int AlignedWidth() { return m_geo.stride_y / m_geo.bytes_per_pixel; }
  virtual int AlignedHeight() { return m_geo.height_y; }
  virtual uint32_t &Encoding() { return m_mmal_format; }
  virtual int Size() { return m_size; }
  AVRpiZcFrameGeometry &GetGeometry() { return m_geo; }
  virtual void Released(CVideoBufferManager &videoBufferManager);

protected:
  int m_width = 0;
  int m_height = 0;
  bool m_configured = false;
  CCriticalSection m_critSection;

  std::vector<CMMALBuffer*> m_all;
  std::deque<int> m_used;
  std::deque<int> m_free;

  int m_size = 0;
  uint32_t m_mmal_format = 0;
  bool m_software = false;
  CProcessInfo *m_processInfo = nullptr;
  MMALState m_state;
  bool m_input;
  MMAL_POOL_T *m_mmal_pool;
  MMAL_COMPONENT_T *m_component;
  AVRpiZcFrameGeometry m_geo;
  struct MMALEncodingTable
  {
    AVPixelFormat pixfmt;
    uint32_t      encoding;
  };
  static std::vector<MMALEncodingTable> mmal_encoding_table;
};

// a generic mmal video frame. May be overridden as either software or hardware decoded buffer
class CMMALBuffer : public CVideoBuffer
{
public:
  CMMALBuffer(int id);
  virtual ~CMMALBuffer();
  MMAL_BUFFER_HEADER_T *mmal_buffer = nullptr;
  uint32_t m_encoding = MMAL_ENCODING_UNKNOWN;
  float m_aspect_ratio = 0.0f;
  MMALState m_state = MMALStateNone;
  bool m_rendered = false;
  bool m_stills = false;

  virtual void Unref();
  virtual std::shared_ptr<CMMALPool> Pool() { return std::dynamic_pointer_cast<CMMALPool>(m_pool); };
  virtual int Width() { return Pool()->Width(); }
  virtual int Height() { return Pool()->Height(); }
  virtual int AlignedWidth() { return Pool()->AlignedWidth(); }
  virtual int AlignedHeight() { return Pool()->AlignedHeight(); }
  virtual uint32_t &Encoding() { return Pool()->Encoding(); }
  virtual void Update();

  void SetVideoDeintMethod(std::string method);
  const char *GetStateName() {
    static const char *names[] = { "MMALStateNone", "MMALStateHWDec", "MMALStateFFDec", "MMALStateDeint", };
    if ((size_t)m_state < vcos_countof(names))
      return names[(size_t)m_state];
    else
      return "invalid";
  }
protected:
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
  virtual void         AddVideoPicture(const VideoPicture& pic, int index, double currentClock) override;
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

  static CBaseRenderer* Create(CVideoBuffer *buffer);
  static bool Register();

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

};
