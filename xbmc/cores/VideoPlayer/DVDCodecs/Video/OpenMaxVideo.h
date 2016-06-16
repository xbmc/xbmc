#pragma once
/*
 *      Copyright (C) 2010-2013 Team XBMC
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

#if defined(HAVE_LIBOPENMAX)
#include <queue>
#include <vector>

#include "OpenMax.h"
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <DVDResource.h>

template<typename T> struct IDVDResourceCounted2
{
  IDVDResourceCounted2() : m_refs(1) {}
  virtual ~IDVDResourceCounted2() {}
  virtual T*   Acquire()
  {
    printf("Acquire %p %d\n", this, m_refs);
    ++m_refs;
    return (T*)this;
  }

  virtual long Release()
  {
    printf("Release %p %d\n", this, m_refs);
    --m_refs;
    assert(m_refs >= 0);
    if (m_refs == 0) delete (T*)this;
    return m_refs;
  }
  int m_refs;
};


// an omx egl video frame
struct OpenMaxVideoBuffer : public IDVDResourceCounted<OpenMaxVideoBuffer> {
  OpenMaxVideoBuffer();
  virtual ~OpenMaxVideoBuffer();

  OMX_BUFFERHEADERTYPE *omx_buffer;
  int width;
  int height;
  int index;

  // used for egl based rendering if active
  EGLImageKHR egl_image;
  GLuint texture_id;

#if defined(EGL_KHR_reusable_sync)
  EGLSyncKHR eglSync;
#endif
  EGLDisplay eglDisplay;

  bool done;
  void PassBackToRenderer();
  void ReleaseTexture();
  void SetOpenMaxVideo(COpenMaxVideo *openMaxVideo);

private:
  COpenMaxVideo *m_openMaxVideo;
};

class OpenMaxVideoBufferHolder : public IDVDResourceCounted<OpenMaxVideoBufferHolder> {
public:
  OpenMaxVideoBufferHolder(OpenMaxVideoBuffer *openMaxVideoBuffer);
  virtual ~OpenMaxVideoBufferHolder();

  OpenMaxVideoBuffer *m_openMaxVideoBuffer;
};

class COpenMaxVideo : public COpenMax, public IDVDResourceCounted<COpenMaxVideo>
{
public:
  COpenMaxVideo();
  virtual ~COpenMaxVideo();

  // Required overrides
  bool Open(CDVDStreamInfo &hints);
  void Close(void);
  int  Decode(uint8_t *pData, int iSize, double dts, double pts);
  void Reset(void);
  int GetPicture(DVDVideoPicture *pDvdVideoPicture);
  bool ClearPicture(DVDVideoPicture *pDvdVideoPicture);
  void ReleaseBuffer(OpenMaxVideoBuffer *buffer);
  void SetDropState(bool bDrop);
protected:
  int EnqueueDemuxPacket(omx_demux_packet demux_packet);

  void QueryCodec(void);
  OMX_ERRORTYPE PrimeFillBuffers(void);
  OMX_ERRORTYPE AllocOMXInputBuffers(void);
  OMX_ERRORTYPE FreeOMXInputBuffers(bool wait);
  OMX_ERRORTYPE AllocOMXOutputBuffers(void);
  OMX_ERRORTYPE FreeOMXOutputBuffers(bool wait);
  static void CallbackAllocOMXEGLTextures(void*);
  OMX_ERRORTYPE AllocOMXOutputEGLTextures(void);

  //! @todo Those should move into the base class. After start actions can be executed by callbacks.
  OMX_ERRORTYPE StartDecoder(void);
  OMX_ERRORTYPE StopDecoder(void);

  void ReleaseDemuxQueue();

  // OpenMax decoder callback routines.
  virtual OMX_ERRORTYPE DecoderEventHandler(OMX_HANDLETYPE hComponent, OMX_PTR pAppData,
    OMX_EVENTTYPE eEvent, OMX_U32 nData1, OMX_U32 nData2, OMX_PTR pEventData);
  virtual OMX_ERRORTYPE DecoderEmptyBufferDone(
    OMX_HANDLETYPE hComponent, OMX_PTR pAppData, OMX_BUFFERHEADERTYPE* pBuffer);
  virtual OMX_ERRORTYPE DecoderFillBufferDone(
    OMX_HANDLETYPE hComponent, OMX_PTR pAppData, OMX_BUFFERHEADERTYPE* pBufferHeader);

  // EGL Resources
  EGLDisplay        m_egl_display;
  EGLContext        m_egl_context;

  // Video format
  DVDVideoPicture   m_videobuffer;
  bool              m_drop_state;
  int               m_decoded_width;
  int               m_decoded_height;

  std::queue<double> m_dts_queue;
  std::queue<omx_demux_packet> m_demux_queue;

  // Synchronization
  //pthread_mutex_t   m_omx_queue_mutex;
  pthread_cond_t    m_omx_queue_available;

  // OpenMax input buffers (demuxer packets)
  std::queue<OMX_BUFFERHEADERTYPE*> m_omx_input_avaliable;
  std::vector<OMX_BUFFERHEADERTYPE*> m_omx_input_buffers;
  bool              m_omx_input_eos;
  int               m_omx_input_port;
  //sem_t             *m_omx_flush_input;
  CEvent            m_input_consumed_event;

  // OpenMax output buffers (video frames)
  std::queue<OpenMaxVideoBuffer*> m_omx_output_busy;
  std::queue<OpenMaxVideoBuffer*> m_omx_output_ready;
  std::vector<OpenMaxVideoBuffer*> m_omx_output_buffers;
  bool              m_omx_output_eos;
  int               m_omx_output_port;
  //sem_t             *m_omx_flush_output;

  bool              m_portChanging;

  volatile bool     m_videoplayback_done;
};

// defined(HAVE_LIBOPENMAX)
#endif
