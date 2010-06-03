#pragma once
/*
 *      Copyright (C) 2010 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#if defined(HAVE_LIBOPENMAX)

#include "OpenMax.h"
#include "DVDStreamInfo.h"
#include "DVDVideoCodec.h"
#include "utils/Event.h"

#include <queue>
#include <semaphore.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <OMX_Core.h>

typedef struct omx_codec_capability {
    // level is OMX_VIDEO_AVCPROFILETYPE, OMX_VIDEO_H263PROFILETYPE, 
    // or OMX_VIDEO_MPEG4PROFILETYPE depending on context.
    OMX_U32 level;
    // level is OMX_VIDEO_AVCLEVELTYPE, OMX_VIDEO_H263LEVELTYPE, 
    // or OMX_VIDEO_MPEG4PROFILETYPE depending on context.
    OMX_U32 profile;
} omx_codec_capability;

typedef struct omx_demux_packet {
  OMX_U8 *buff;
  int size;
  double dts;
  double pts;
} omx_demux_packet;

// an omx egl video frame
typedef struct omx_egl_buffer {
  EGLImageKHR egl_image;
  GLuint texture_id;
  OMX_BUFFERHEADERTYPE *omx_buffer;
  int width;
  int height;
  int index;
} omx_egl_buffer;


class DllLibOpenMax;
class COpenMax
{
public:
  COpenMax();
  virtual ~COpenMax();

  // Required overrides
  bool Open(CDVDStreamInfo &hints);
  void Close(void);
  int  Decode(BYTE *pData, int iSize, double dts, double pts);
  void Reset(void);
  bool GetPicture(DVDVideoPicture *pDvdVideoPicture);
  void SetDropState(bool bDrop);
  
protected:
  enum OMX_CLIENT_STATE {
      DEAD,
      LOADED,
      LOADED_TO_IDLE,
      IDLE_TO_EXECUTING,
      EXECUTING,
      EXECUTING_TO_IDLE,
      IDLE_TO_LOADED,
      RECONFIGURING,
      ERROR
  };

  // OpenMax Decoder Callback routines.
  static OMX_ERRORTYPE DecoderEventHandler(OMX_HANDLETYPE hComponent, OMX_PTR pAppData,
    OMX_EVENTTYPE eEvent, OMX_U32 nData1, OMX_U32 nData2, OMX_PTR pEventData);
  static OMX_ERRORTYPE DecoderEmptyBufferDone(
    OMX_HANDLETYPE hComponent, OMX_PTR pAppData, OMX_BUFFERHEADERTYPE* pBuffer);
  static OMX_ERRORTYPE DecoderFillBufferDone(
    OMX_HANDLETYPE hComponent, OMX_PTR pAppData, OMX_BUFFERHEADERTYPE* pBufferHeader);

  // OpenMax helper routines
  void QueryCodec(void);
  OMX_ERRORTYPE PrimeFillBuffers(void);
  OMX_ERRORTYPE AllocOMXInputBuffers(void);
  OMX_ERRORTYPE FreeOMXInputBuffers(bool wait);
  OMX_ERRORTYPE AllocOMXOutputBuffers(void);
  OMX_ERRORTYPE FreeOMXOutputBuffers(bool wait);
  OMX_ERRORTYPE AllocOMXOutputEGLTextures(void);
  OMX_ERRORTYPE FreeOMXOutputEGLTextures(bool wait);
  OMX_ERRORTYPE WaitForState(OMX_STATETYPE state);
  OMX_ERRORTYPE SetStateForComponent(OMX_STATETYPE state);
  OMX_ERRORTYPE StartDecoder(void);
  OMX_ERRORTYPE StopDecoder(void);

  DllLibOpenMax     *m_dll;
  bool              m_is_open;
  OMX_HANDLETYPE    m_omx_decoder;   // openmax decoder component reference
  DVDVideoPicture   m_videobuffer;
  bool              m_drop_state;
  int               m_decoded_width;
  int               m_decoded_height;

  std::queue<double> m_dts_queue;
  std::queue<omx_demux_packet> m_demux_queue;

  // OpenMax input buffers (demuxer packets)
  pthread_mutex_t   m_omx_avaliable_mutex;
  std::queue<OMX_BUFFERHEADERTYPE*> m_omx_input_avaliable;
  std::vector<OMX_BUFFERHEADERTYPE*> m_omx_input_buffers;
  bool              m_omx_input_eos;
  int               m_omx_input_port;
  //sem_t             *m_omx_flush_input;
  CEvent            m_input_consumed_event;

  // OpenMax output buffers (video frames)
  pthread_mutex_t   m_omx_ready_mutex;
  std::queue<OMX_BUFFERHEADERTYPE*> m_omx_output_ready;
  std::vector<OMX_BUFFERHEADERTYPE*> m_omx_output_buffers;
  bool              m_omx_output_eos;
  int               m_omx_output_port;
  //sem_t             *m_omx_flush_output;

  EGLDisplay        m_egl_display;
  EGLContext        m_egl_context;
  std::queue<omx_egl_buffer*> m_omx_egl_output_ready;
  std::vector<omx_egl_buffer*> m_omx_egl_output_buffers;


  // OpenMax state tracking
  OMX_CLIENT_STATE  m_omx_client_state;
  volatile int      m_omx_decoder_state;
  sem_t             *m_omx_decoder_state_change;
  std::vector<omx_codec_capability> m_omx_decoder_capabilities;

  volatile bool     m_videoplayback_done;
};

#endif
