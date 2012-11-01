/*
 *      Copyright (C) 2010-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#if (defined HAVE_CONFIG_H) && (!defined WIN32)
  #include "config.h"
#elif defined(_WIN32)
#include "system.h"
#endif

#include "OpenMaxVideo.h"

#include "DVDClock.h"
#include "DVDStreamInfo.h"
#include "windowing/WindowingFactory.h"
#include "DVDVideoCodec.h"
#include "utils/log.h"
#include "utils/TimeUtils.h"
#include "ApplicationMessenger.h"
#include "Application.h"

#include <OMX_Core.h>
#include <OMX_Component.h>
#include <OMX_Index.h>
#include <OMX_Image.h>


#define CLASSNAME "COpenMaxVideo"

// TODO: These are Nvidia Tegra2 dependent, need to dynamiclly find the
// right codec matched to video format.
#define OMX_H264BASE_DECODER    "OMX.Nvidia.h264.decode"
// OMX.Nvidia.h264ext.decode segfaults, not sure why.
//#define OMX_H264MAIN_DECODER  "OMX.Nvidia.h264ext.decode"
#define OMX_H264MAIN_DECODER    "OMX.Nvidia.h264.decode"
#define OMX_H264HIGH_DECODER    "OMX.Nvidia.h264ext.decode"
#define OMX_MPEG4_DECODER       "OMX.Nvidia.mp4.decode"
#define OMX_MPEG4EXT_DECODER    "OMX.Nvidia.mp4ext.decode"
#define OMX_MPEG2V_DECODER      "OMX.Nvidia.mpeg2v.decode"
#define OMX_VC1_DECODER         "OMX.Nvidia.vc1.decode"

// EGL extension functions
static PFNEGLCREATEIMAGEKHRPROC eglCreateImageKHR;
static PFNEGLDESTROYIMAGEKHRPROC eglDestroyImageKHR;
#define GETEXTENSION(type, ext) \
do \
{ \
    ext = (type) eglGetProcAddress(#ext); \
    if (!ext) \
    { \
        CLog::Log(LOGERROR, "%s::%s - ERROR getting proc addr of " #ext "\n", CLASSNAME, __func__); \
    } \
} while (0);

#define OMX_INIT_STRUCTURE(a) \
  memset(&(a), 0, sizeof(a)); \
  (a).nSize = sizeof(a); \
  (a).nVersion.s.nVersionMajor = OMX_VERSION_MAJOR; \
  (a).nVersion.s.nVersionMinor = OMX_VERSION_MINOR; \
  (a).nVersion.s.nRevision = OMX_VERSION_REVISION; \
  (a).nVersion.s.nStep = OMX_VERSION_STEP


COpenMaxVideo::COpenMaxVideo()
{
  m_portChanging = false;

  pthread_mutex_init(&m_omx_input_mutex, NULL);
  pthread_mutex_init(&m_omx_output_mutex, NULL);

  m_omx_decoder_state_change = (sem_t*)malloc(sizeof(sem_t));
  sem_init(m_omx_decoder_state_change, 0, 0);
}

COpenMaxVideo::~COpenMaxVideo()
{
  #if defined(OMX_DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, "%s::%s\n", CLASSNAME, __func__);
  #endif
  if (m_is_open)
    Close();
  pthread_mutex_destroy(&m_omx_input_mutex);
  pthread_mutex_destroy(&m_omx_output_mutex);
  sem_destroy(m_omx_decoder_state_change);
  free(m_omx_decoder_state_change);
}

bool COpenMaxVideo::Open(CDVDStreamInfo &hints)
{
  #if defined(OMX_DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, "%s::%s\n", CLASSNAME, __func__);
  #endif

  OMX_ERRORTYPE omx_err = OMX_ErrorNone;
  std::string decoder_name;

  m_decoded_width  = hints.width;
  m_decoded_height = hints.height;

  switch (hints.codec)
  {
    case CODEC_ID_H264:
    {
      switch(hints.profile)
      {
        case FF_PROFILE_H264_BASELINE:
          // (role name) video_decoder.avc
          // H.264 Baseline profile
          decoder_name = OMX_H264BASE_DECODER;
        break;
        case FF_PROFILE_H264_MAIN:
          // (role name) video_decoder.avc
          // H.264 Main profile
          decoder_name = OMX_H264MAIN_DECODER;
        break;
        case FF_PROFILE_H264_HIGH:
          // (role name) video_decoder.avc
          // H.264 Main profile
          decoder_name = OMX_H264HIGH_DECODER;
        break;
        default:
          return false;
        break;
      }
    }
    break;
    case CODEC_ID_MPEG4:
      // (role name) video_decoder.mpeg4
      // MPEG-4, DivX 4/5 and Xvid compatible
      decoder_name = OMX_MPEG4_DECODER;
    break;
    /*
    TODO: what mpeg4 formats are "ext" ????
    case NvxStreamType_MPEG4Ext:
      // (role name) video_decoder.mpeg4
      // MPEG-4, DivX 4/5 and Xvid compatible
      decoder_name = OMX_MPEG4EXT_DECODER;
      m_pFormatName = "omx-mpeg4";
    break;
    */
    case CODEC_ID_MPEG2VIDEO:
      // (role name) video_decoder.mpeg2
      // MPEG-2
      decoder_name = OMX_MPEG2V_DECODER;
    break;
    case CODEC_ID_VC1:
      // (role name) video_decoder.vc1
      // VC-1, WMV9
      decoder_name = OMX_VC1_DECODER;
    break;
    default:
      return false;
    break;
  }

  // initialize OpenMAX.
  if (!Initialize(decoder_name))
  {
    return false;
  }

  // TODO: Find component from role name.
  // Get the port information. This will obtain information about the
  // number of ports and index of the first port.
  OMX_PORT_PARAM_TYPE port_param;
  OMX_INIT_STRUCTURE(port_param);
  omx_err = OMX_GetParameter(m_omx_decoder, OMX_IndexParamVideoInit, &port_param);
  if (omx_err)
  {
    Deinitialize();
    return false;
  }
  m_omx_input_port = port_param.nStartPortNumber;
  m_omx_output_port = m_omx_input_port + 1;
  #if defined(OMX_DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG,
    "%s::%s - decoder_component(0x%p), input_port(0x%x), output_port(0x%x)\n",
    CLASSNAME, __func__, m_omx_decoder, m_omx_input_port, m_omx_output_port);
  #endif

  // TODO: Set role for the component because components could have multiple roles.
  //QueryCodec();

  // Component will be in OMX_StateLoaded now so we can alloc omx input/output buffers.
  // we can only alloc them in OMX_StateLoaded state or if the component port is disabled
  // Alloc buffers for the omx input port.
  omx_err = AllocOMXInputBuffers();
  if (omx_err)
  {
    Deinitialize();
    return false;
  }
  // Alloc buffers for the omx output port.
  m_egl_display = g_Windowing.GetEGLDisplay();
  m_egl_context = g_Windowing.GetEGLContext();
  omx_err = AllocOMXOutputBuffers();
  if (omx_err)
  {
    FreeOMXInputBuffers(false);
    Deinitialize();
    return false;
  }

  m_is_open = true;
  m_drop_state = false;
  m_videoplayback_done = false;

  // crank it up.
  StartDecoder();

  return true;
}

void COpenMaxVideo::Close()
{
  #if defined(OMX_DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, "%s::%s\n", CLASSNAME, __func__);
  #endif
  if (m_omx_decoder)
  {
    if (m_omx_decoder_state != OMX_StateLoaded)
      StopDecoder();
    Deinitialize();
  }
  m_is_open = false;
}

void COpenMaxVideo::SetDropState(bool bDrop)
{
  m_drop_state = bDrop;

  if (m_drop_state)
  {
    OMX_ERRORTYPE omx_err;

    // blow all but the last ready video frame
    pthread_mutex_lock(&m_omx_output_mutex);
    while (m_omx_output_ready.size() > 1)
    {
      m_dts_queue.pop();
      OMX_BUFFERHEADERTYPE *omx_buffer = m_omx_output_ready.front()->omx_buffer;
      m_omx_output_ready.pop();
      // return the omx buffer back to OpenMax to fill.
      omx_err = OMX_FillThisBuffer(m_omx_decoder, omx_buffer);
      if (omx_err)
        CLog::Log(LOGERROR, "%s::%s - OMX_FillThisBuffer, omx_err(0x%x)\n",
          CLASSNAME, __func__, omx_err);
    }
    pthread_mutex_unlock(&m_omx_output_mutex);

    #if defined(OMX_DEBUG_VERBOSE)
    CLog::Log(LOGDEBUG, "%s::%s - m_drop_state(%d)\n",
      CLASSNAME, __func__, m_drop_state);
    #endif
  }
}

int COpenMaxVideo::Decode(BYTE* pData, int iSize, double dts, double pts)
{
  if (pData)
  {
    int demuxer_bytes = iSize;
    uint8_t *demuxer_content = pData;

    // we need to queue then de-queue the demux packet, seems silly but
    // omx might not have a omx input buffer avaliable when we are called
    // and we must store the demuxer packet and try again later.
    omx_demux_packet demux_packet;
    demux_packet.dts = dts;
    demux_packet.pts = pts;

    demux_packet.size = demuxer_bytes;
    demux_packet.buff = new OMX_U8[demuxer_bytes];
    memcpy(demux_packet.buff, demuxer_content, demuxer_bytes);
    m_demux_queue.push(demux_packet);

    // we can look at m_omx_input_avaliable.empty without needing to lock/unlock
    // try to send any/all demux packets to omx decoder.
    while (!m_omx_input_avaliable.empty() && !m_demux_queue.empty() )
    {
      OMX_ERRORTYPE omx_err;
      OMX_BUFFERHEADERTYPE* omx_buffer;

      demux_packet = m_demux_queue.front();
      m_demux_queue.pop();
      // need to lock here to retreve an input buffer and pop the queue
      pthread_mutex_lock(&m_omx_input_mutex);
      omx_buffer = m_omx_input_avaliable.front();
      m_omx_input_avaliable.pop();
      pthread_mutex_unlock(&m_omx_input_mutex);

      // delete the previous demuxer buffer
      delete [] omx_buffer->pBuffer;
      // setup a new omx_buffer.
      omx_buffer->nFlags  = m_omx_input_eos ? OMX_BUFFERFLAG_EOS : 0;
      omx_buffer->nOffset = 0;
      omx_buffer->pBuffer = demux_packet.buff;
      omx_buffer->nAllocLen  = demux_packet.size;
      omx_buffer->nFilledLen = demux_packet.size;
      omx_buffer->nTimeStamp = (demux_packet.pts == DVD_NOPTS_VALUE) ? 0 : demux_packet.pts * 1000.0; // in microseconds;
      omx_buffer->pAppPrivate = omx_buffer;
      omx_buffer->nInputPortIndex = m_omx_input_port;

      #if defined(OMX_DEBUG_EMPTYBUFFERDONE)
      CLog::Log(LOGDEBUG,
        "%s::%s - feeding decoder, omx_buffer->pBuffer(0x%p), demuxer_bytes(%d)\n",
        CLASSNAME, __func__, omx_buffer->pBuffer, demuxer_bytes);
      #endif
      // Give this omx_buffer to OpenMax to be decoded.
      omx_err = OMX_EmptyThisBuffer(m_omx_decoder, omx_buffer);
      if (omx_err)
      {
        CLog::Log(LOGDEBUG,
          "%s::%s - OMX_EmptyThisBuffer() failed with result(0x%x)\n",
          CLASSNAME, __func__, omx_err);
        return VC_ERROR;
      }
      // only push if we are successful with feeding OMX_EmptyThisBuffer
      m_dts_queue.push(demux_packet.dts);

      // if m_omx_input_avaliable and/or demux_queue are now empty,
      // wait up to 20ms for OpenMax to consume a demux packet
      if (m_omx_input_avaliable.empty() || m_demux_queue.empty())
        m_input_consumed_event.WaitMSec(1);
    }
    if (m_omx_input_avaliable.empty() && !m_demux_queue.empty())
      m_input_consumed_event.WaitMSec(1);

    #if defined(OMX_DEBUG_VERBOSE)
    if (m_omx_input_avaliable.empty())
      CLog::Log(LOGDEBUG,
        "%s::%s - buffering demux, m_demux_queue_size(%d), demuxer_bytes(%d)\n",
        CLASSNAME, __func__, m_demux_queue.size(), demuxer_bytes);
    #endif
  }

  if (m_omx_output_ready.empty())
    return VC_BUFFER;

  return VC_PICTURE | VC_BUFFER;
}

void COpenMaxVideo::Reset(void)
{
  #if defined(OMX_DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, "%s::%s\n", CLASSNAME, __func__);
  #endif
/*
  // only reset OpenMax decoder if it's running
  if (m_omx_decoder_state == OMX_StateExecuting)
  {
    OMX_ERRORTYPE omx_err;

    omx_err = StopDecoder();
    // Alloc OpenMax input buffers.
    omx_err = AllocOMXInputBuffers();
    // Alloc OpenMax output buffers.
    omx_err = AllocOMXOutputBuffers();

    omx_err = StartDecoder();
  }
*/
  ::Sleep(100);
}

bool COpenMaxVideo::GetPicture(DVDVideoPicture* pDvdVideoPicture)
{
  while (m_omx_output_busy.size() > 1)
  {
    // fetch a output buffer and pop it off the busy list
    pthread_mutex_lock(&m_omx_output_mutex);
    OpenMaxVideoBuffer *buffer = m_omx_output_busy.front();
    m_omx_output_busy.pop();
    pthread_mutex_unlock(&m_omx_output_mutex);

    bool done = buffer->omx_buffer->nFlags & OMX_BUFFERFLAG_EOS;
    if (!done)
    {
      // return the omx buffer back to OpenMax to fill.
      OMX_ERRORTYPE omx_err = OMX_FillThisBuffer(m_omx_decoder, buffer->omx_buffer);
      if (omx_err)
        CLog::Log(LOGERROR, "%s::%s - OMX_FillThisBuffer, omx_err(0x%x)\n",
          CLASSNAME, __func__, omx_err);
    }
  }

  if (!m_omx_output_ready.empty())
  {
    OpenMaxVideoBuffer *buffer;
    // fetch a output buffer and pop it off the ready list
    pthread_mutex_lock(&m_omx_output_mutex);
    buffer = m_omx_output_ready.front();
    m_omx_output_ready.pop();
    m_omx_output_busy.push(buffer);
    pthread_mutex_unlock(&m_omx_output_mutex);

    pDvdVideoPicture->dts = DVD_NOPTS_VALUE;
    pDvdVideoPicture->pts = DVD_NOPTS_VALUE;
    pDvdVideoPicture->format = RENDER_FMT_OMXEGL;
    pDvdVideoPicture->openMax = this;
    pDvdVideoPicture->openMaxBuffer = buffer;

    if (!m_dts_queue.empty())
    {
      pDvdVideoPicture->dts = m_dts_queue.front();
      m_dts_queue.pop();
    }
    // nTimeStamp is in microseconds
    pDvdVideoPicture->pts = (buffer->omx_buffer->nTimeStamp == 0) ? DVD_NOPTS_VALUE : (double)buffer->omx_buffer->nTimeStamp / 1000.0;
  }
  #if defined(OMX_DEBUG_VERBOSE)
  else
  {
    CLog::Log(LOGDEBUG, "%s::%s - called but m_omx_output_ready is empty\n",
      CLASSNAME, __func__);
  }
  #endif

  pDvdVideoPicture->iFlags  = DVP_FLAG_ALLOCATED;
  pDvdVideoPicture->iFlags |= m_drop_state ? DVP_FLAG_DROPPED : 0;

  return true;
}


// DecoderEmptyBufferDone -- OpenMax input buffer has been emptied
OMX_ERRORTYPE COpenMaxVideo::DecoderEmptyBufferDone(
  OMX_HANDLETYPE hComponent,
  OMX_PTR pAppData,
  OMX_BUFFERHEADERTYPE* pBuffer)
{
  COpenMaxVideo *ctx = static_cast<COpenMaxVideo*>(pAppData);
/*
  #if defined(OMX_DEBUG_EMPTYBUFFERDONE)
  CLog::Log(LOGDEBUG, "%s::%s - buffer_size(%lu), timestamp(%f)\n",
    CLASSNAME, __func__, pBuffer->nFilledLen, (double)pBuffer->nTimeStamp / 1000.0);
  #endif
*/
  // queue free input buffer to avaliable list.
  pthread_mutex_lock(&ctx->m_omx_input_mutex);
  ctx->m_omx_input_avaliable.push(pBuffer);
  ctx->m_input_consumed_event.Set();
  pthread_mutex_unlock(&ctx->m_omx_input_mutex);

  return OMX_ErrorNone;
}

// DecoderFillBufferDone -- OpenMax output buffer has been filled
OMX_ERRORTYPE COpenMaxVideo::DecoderFillBufferDone(
  OMX_HANDLETYPE hComponent,
  OMX_PTR pAppData,
  OMX_BUFFERHEADERTYPE* pBuffer)
{
  COpenMaxVideo *ctx = static_cast<COpenMaxVideo*>(pAppData);
  OpenMaxVideoBuffer *buffer = (OpenMaxVideoBuffer*)pBuffer->pAppPrivate;

  #if defined(OMX_DEBUG_FILLBUFFERDONE)
  CLog::Log(LOGDEBUG, "%s::%s - buffer_size(%lu), timestamp(%f)\n",
    CLASSNAME, __func__, pBuffer->nFilledLen, (double)pBuffer->nTimeStamp / 1000.0);
  #endif

  if (!ctx->m_portChanging)
  {
    // queue output omx buffer to ready list.
    pthread_mutex_lock(&ctx->m_omx_output_mutex);
    ctx->m_omx_output_ready.push(buffer);
    pthread_mutex_unlock(&ctx->m_omx_output_mutex);
  }

  return OMX_ErrorNone;
}

void COpenMaxVideo::QueryCodec(void)
{
  OMX_ERRORTYPE omx_err = OMX_ErrorNone;
  OMX_VIDEO_PARAM_PROFILELEVELTYPE port_param;
  OMX_INIT_STRUCTURE(port_param);

  port_param.nPortIndex = m_omx_input_port;

  for (port_param.nProfileIndex = 0;; port_param.nProfileIndex++)
  {
    omx_err = OMX_GetParameter(m_omx_decoder,
      OMX_IndexParamVideoProfileLevelQuerySupported, &port_param);
    if (omx_err)
      break;

    omx_codec_capability omx_capability;
    omx_capability.level = port_param.eLevel;
    omx_capability.profile = port_param.eProfile;
    m_omx_decoder_capabilities.push_back(omx_capability);
  }
}

OMX_ERRORTYPE COpenMaxVideo::PrimeFillBuffers(void)
{
  OMX_ERRORTYPE omx_err = OMX_ErrorNone;
  OpenMaxVideoBuffer *buffer;

  #if defined(OMX_DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, "%s::%s\n", CLASSNAME, __func__);
  #endif
  // tell OpenMax to start filling output buffers
  for (size_t i = 0; i < m_omx_output_buffers.size(); i++)
  {
    buffer = m_omx_output_buffers[i];
    // always set the port index.
    buffer->omx_buffer->nOutputPortIndex = m_omx_output_port;
    // Need to clear the EOS flag.
    buffer->omx_buffer->nFlags &= ~OMX_BUFFERFLAG_EOS;
    buffer->omx_buffer->pAppPrivate = buffer;

    omx_err = OMX_FillThisBuffer(m_omx_decoder, buffer->omx_buffer);
    if (omx_err)
      CLog::Log(LOGERROR, "%s::%s - OMX_FillThisBuffer failed with omx_err(0x%x)\n",
        CLASSNAME, __func__, omx_err);
  }

  return omx_err;
}

OMX_ERRORTYPE COpenMaxVideo::AllocOMXInputBuffers(void)
{
  OMX_ERRORTYPE omx_err = OMX_ErrorNone;

  // Obtain the information about the decoder input port.
  OMX_PARAM_PORTDEFINITIONTYPE port_format;
  OMX_INIT_STRUCTURE(port_format);
  port_format.nPortIndex = m_omx_input_port;
  OMX_GetParameter(m_omx_decoder, OMX_IndexParamPortDefinition, &port_format);

  #if defined(OMX_DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG,
    "%s::%s - iport(%d), nBufferCountMin(%lu), nBufferSize(%lu)\n",
    CLASSNAME, __func__, m_omx_input_port, port_format.nBufferCountMin, port_format.nBufferSize);
  #endif
  for (size_t i = 0; i < port_format.nBufferCountMin; i++)
  {
    OMX_BUFFERHEADERTYPE *buffer = NULL;
    // use an external buffer that's sized according to actual demux
    // packet size, start at internal's buffer size, will get deleted when
    // we start pulling demuxer packets and using demux packet sized buffers.
    OMX_U8* data = new OMX_U8[port_format.nBufferSize];
    omx_err = OMX_UseBuffer(m_omx_decoder, &buffer, m_omx_input_port, NULL, port_format.nBufferSize, data);
    if (omx_err)
    {
      CLog::Log(LOGERROR, "%s::%s - OMX_UseBuffer failed with omx_err(0x%x)\n",
        CLASSNAME, __func__, omx_err);
      return(omx_err);
    }
    m_omx_input_buffers.push_back(buffer);
    // don't have to lock/unlock here, we are not decoding
    m_omx_input_avaliable.push(buffer);
  }
  m_omx_input_eos = false;

  return(omx_err);
}
OMX_ERRORTYPE COpenMaxVideo::FreeOMXInputBuffers(bool wait)
{
  OMX_ERRORTYPE omx_err = OMX_ErrorNone;

  /*
  omx_err = OMX_SendCommand(m_omx_decoder, OMX_CommandFlush, m_omx_input_port, 0);
  if (omx_err)
    CLog::Log(LOGERROR, "%s::%s - OMX_CommandFlush failed with omx_err(0x%x)\n",
      CLASSNAME, __func__, omx_err);
  else if (wait)
    sem_wait(m_omx_flush_input);
  */

  // free omx input port buffers.
  for (size_t i = 0; i < m_omx_input_buffers.size(); i++)
  {
    // using external buffers (OMX_UseBuffer), free our external buffers
    //  before calling OMX_FreeBuffer which frees the omx buffer.
    delete [] m_omx_input_buffers[i]->pBuffer;
    m_omx_input_buffers[i]->pBuffer = NULL;
    omx_err = OMX_FreeBuffer(m_omx_decoder, m_omx_input_port, m_omx_input_buffers[i]);
  }
  m_omx_input_buffers.clear();

  // empty input buffer queue. not decoding so don't need lock/unlock.
  while (!m_omx_input_avaliable.empty())
    m_omx_input_avaliable.pop();
  while (!m_demux_queue.empty())
    m_demux_queue.pop();
  while (!m_dts_queue.empty())
    m_dts_queue.pop();

  return(omx_err);
}

void COpenMaxVideo::CallbackAllocOMXEGLTextures(void *userdata)
{
  COpenMaxVideo *omx = static_cast<COpenMaxVideo*>(userdata);
  omx->AllocOMXOutputEGLTextures();
}

void COpenMaxVideo::CallbackFreeOMXEGLTextures(void *userdata)
{
  COpenMaxVideo *omx = static_cast<COpenMaxVideo*>(userdata);
  omx->FreeOMXOutputEGLTextures(true);
}

OMX_ERRORTYPE COpenMaxVideo::AllocOMXOutputBuffers(void)
{
  OMX_ERRORTYPE omx_err;

  if ( g_application.IsCurrentThread() )
  {
    omx_err = AllocOMXOutputEGLTextures();
  }
  else
  {
    ThreadMessageCallback callbackData;
    callbackData.callback = &CallbackAllocOMXEGLTextures;
    callbackData.userptr = (void *)this;

    ThreadMessage tMsg;
    tMsg.dwMessage = TMSG_CALLBACK;
    tMsg.lpVoid = (void*)&callbackData;

    g_application.getApplicationMessenger().SendMessage(tMsg, true);

    omx_err = OMX_ErrorNone;
  }

  return omx_err;
}

OMX_ERRORTYPE COpenMaxVideo::FreeOMXOutputBuffers(bool wait)
{
  OMX_ERRORTYPE omx_err = FreeOMXOutputEGLTextures(wait);

  return omx_err;
}

OMX_ERRORTYPE COpenMaxVideo::AllocOMXOutputEGLTextures(void)
{
  OMX_ERRORTYPE omx_err;

  if (!eglCreateImageKHR)
  {
    GETEXTENSION(PFNEGLCREATEIMAGEKHRPROC,  eglCreateImageKHR);
  }

  EGLint attrib = EGL_NONE;
  OpenMaxVideoBuffer *egl_buffer;

  // Obtain the information about the output port.
  OMX_PARAM_PORTDEFINITIONTYPE port_format;
  OMX_INIT_STRUCTURE(port_format);
  port_format.nPortIndex = m_omx_output_port;
  omx_err = OMX_GetParameter(m_omx_decoder, OMX_IndexParamPortDefinition, &port_format);

  #if defined(OMX_DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG,
    "%s::%s (1) - oport(%d), nFrameWidth(%lu), nFrameHeight(%lu), nStride(%lx), nBufferCountMin(%lu), nBufferSize(%lu)\n",
    CLASSNAME, __func__, m_omx_output_port,
    port_format.format.video.nFrameWidth, port_format.format.video.nFrameHeight,port_format.format.video.nStride,
    port_format.nBufferCountMin, port_format.nBufferSize);
  #endif

  glActiveTexture(GL_TEXTURE0);

  for (size_t i = 0; i < port_format.nBufferCountMin; i++)
  {
    egl_buffer = new OpenMaxVideoBuffer;
    memset(egl_buffer, 0, sizeof(*egl_buffer));
    egl_buffer->width  = m_decoded_width;
    egl_buffer->height = m_decoded_height;

    glGenTextures(1, &egl_buffer->texture_id);
    glBindTexture(GL_TEXTURE_2D, egl_buffer->texture_id);

    // create space for buffer with a texture
    glTexImage2D(
      GL_TEXTURE_2D,      // target
      0,                  // level
      GL_RGBA,            // internal format
      m_decoded_width,    // width
      m_decoded_height,   // height
      0,                  // border
      GL_RGBA,            // format
      GL_UNSIGNED_BYTE,   // type
      NULL);              // pixels -- will be provided later
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // create EGLImage from texture
    egl_buffer->egl_image = eglCreateImageKHR(
      m_egl_display,
      m_egl_context,
      EGL_GL_TEXTURE_2D_KHR,
      (EGLClientBuffer)(egl_buffer->texture_id),
      &attrib);
    if (!egl_buffer->egl_image)
    {
      CLog::Log(LOGERROR, "%s::%s - ERROR creating EglImage\n", CLASSNAME, __func__);
      return(OMX_ErrorUndefined);
    }
    egl_buffer->index = i;

    // tell decoder output port that it will be using EGLImage
    omx_err = OMX_UseEGLImage(
      m_omx_decoder, &egl_buffer->omx_buffer, m_omx_output_port, egl_buffer, egl_buffer->egl_image);
    if (omx_err)
    {
      CLog::Log(LOGERROR, "%s::%s - OMX_UseEGLImage failed with omx_err(0x%x)\n",
        CLASSNAME, __func__, omx_err);
      return(omx_err);
    }
    m_omx_output_buffers.push_back(egl_buffer);

    CLog::Log(LOGDEBUG, "%s::%s - Texture %p Width %d Height %d\n",
      CLASSNAME, __func__, egl_buffer->egl_image, egl_buffer->width, egl_buffer->height);
  }
  m_omx_output_eos = false;
  while (!m_omx_output_busy.empty())
    m_omx_output_busy.pop();
  while (!m_omx_output_ready.empty())
    m_omx_output_ready.pop();

  return omx_err;
}

OMX_ERRORTYPE COpenMaxVideo::FreeOMXOutputEGLTextures(bool wait)
{
  OMX_ERRORTYPE omx_err = OMX_ErrorNone;
  OpenMaxVideoBuffer *egl_buffer;

  if (!eglDestroyImageKHR)
  {
    GETEXTENSION(PFNEGLDESTROYIMAGEKHRPROC, eglDestroyImageKHR);
  }

  for (size_t i = 0; i < m_omx_output_buffers.size(); i++)
  {
    egl_buffer = m_omx_output_buffers[i];
    // tell decoder output port to stop using the EGLImage
    omx_err = OMX_FreeBuffer(m_omx_decoder, m_omx_output_port, egl_buffer->omx_buffer);
    // destroy egl_image
    eglDestroyImageKHR(m_egl_display, egl_buffer->egl_image);
    // free texture
    glDeleteTextures(1, &egl_buffer->texture_id);
    delete egl_buffer;
  }
  m_omx_output_buffers.clear();

  return omx_err;
}


////////////////////////////////////////////////////////////////////////////////////////////
// DecoderEventHandler -- OMX event callback
OMX_ERRORTYPE COpenMaxVideo::DecoderEventHandler(
  OMX_HANDLETYPE hComponent,
  OMX_PTR pAppData,
  OMX_EVENTTYPE eEvent,
  OMX_U32 nData1,
  OMX_U32 nData2,
  OMX_PTR pEventData)
{
  OMX_ERRORTYPE omx_err;
  COpenMaxVideo *ctx = static_cast<COpenMaxVideo*>(pAppData);

/*
  #if defined(OMX_DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG,
    "COpenMax::%s - hComponent(0x%p), eEvent(0x%x), nData1(0x%lx), nData2(0x%lx), pEventData(0x%p)\n",
    __func__, hComponent, eEvent, nData1, nData2, pEventData);
  #endif
*/

  switch (eEvent)
  {
    case OMX_EventCmdComplete:
      switch(nData1)
      {
        case OMX_CommandStateSet:
          ctx->m_omx_decoder_state = (int)nData2;
          switch (ctx->m_omx_decoder_state)
          {
            case OMX_StateInvalid:
              CLog::Log(LOGDEBUG, "%s::%s - OMX_StateInvalid\n", CLASSNAME, __func__);
            break;
            case OMX_StateLoaded:
              CLog::Log(LOGDEBUG, "%s::%s - OMX_StateLoaded\n", CLASSNAME, __func__);
            break;
            case OMX_StateIdle:
              CLog::Log(LOGDEBUG, "%s::%s - OMX_StateIdle\n", CLASSNAME, __func__);
            break;
            case OMX_StateExecuting:
              CLog::Log(LOGDEBUG, "%s::%s - OMX_StateExecuting\n", CLASSNAME, __func__);
            break;
            case OMX_StatePause:
              CLog::Log(LOGDEBUG, "%s::%s - OMX_StatePause\n", CLASSNAME, __func__);
            break;
            case OMX_StateWaitForResources:
              CLog::Log(LOGDEBUG, "%s::%s - OMX_StateWaitForResources\n", CLASSNAME, __func__);
            break;
            default:
              CLog::Log(LOGDEBUG,
                "%s::%s - Unknown OMX_Statexxxxx, state(%d)\n",
                CLASSNAME, __func__, ctx->m_omx_decoder_state);
            break;
          }
          sem_post(ctx->m_omx_decoder_state_change);
        break;
        case OMX_CommandFlush:
          /*
          if (OMX_ALL == (int)nData2)
          {
            sem_post(ctx->m_omx_flush_input);
            sem_post(ctx->m_omx_flush_output);
            CLog::Log(LOGDEBUG, "COpenMax::%s - OMX_CommandFlush input/output\n",__func__);
          }
          else if (ctx->m_omx_input_port == (int)nData2)
          {
            sem_post(ctx->m_omx_flush_input);
            CLog::Log(LOGDEBUG, "COpenMax::%s - OMX_CommandFlush input\n",__func__);
          }
          else if (ctx->m_omx_output_port == (int)nData2)
          {
            sem_post(ctx->m_omx_flush_output);
            CLog::Log(LOGDEBUG, "COpenMax::%s - OMX_CommandFlush ouput\n",__func__);
          }
          else
          */
          {
            #if defined(OMX_DEBUG_EVENTHANDLER)
            CLog::Log(LOGDEBUG,
              "%s::%s - OMX_CommandFlush, nData2(0x%lx)\n",
              CLASSNAME, __func__, nData2);
            #endif
          }
        break;
        case OMX_CommandPortDisable:
          #if defined(OMX_DEBUG_EVENTHANDLER)
          CLog::Log(LOGDEBUG,
            "%s::%s - OMX_CommandPortDisable, nData1(0x%lx), nData2(0x%lx)\n",
            CLASSNAME, __func__, nData1, nData2);
          #endif
          if (ctx->m_omx_output_port == (int)nData2)
          {
            // Got OMX_CommandPortDisable event, alloc new buffers for the output port.
            omx_err = ctx->AllocOMXOutputBuffers();
            omx_err = OMX_SendCommand(ctx->m_omx_decoder, OMX_CommandPortEnable, ctx->m_omx_output_port, NULL);
          }
        break;
        case OMX_CommandPortEnable:
          #if defined(OMX_DEBUG_EVENTHANDLER)
          CLog::Log(LOGDEBUG,
            "%s::%s - OMX_CommandPortEnable, nData1(0x%lx), nData2(0x%lx)\n",
            CLASSNAME, __func__, nData1, nData2);
          #endif
          if (ctx->m_omx_output_port == (int)nData2)
          {
            // Got OMX_CommandPortEnable event.
            // OMX_CommandPortDisable will have re-alloced new ones so re-prime
            ctx->PrimeFillBuffers();
          }
          ctx->m_portChanging = false;
        break;
        #if defined(OMX_DEBUG_EVENTHANDLER)
        case OMX_CommandMarkBuffer:
          CLog::Log(LOGDEBUG,
            "%s::%s - OMX_CommandMarkBuffer, nData1(0x%lx), nData2(0x%lx)\n",
            CLASSNAME, __func__, nData1, nData2);
        break;
        #endif
      }
    break;
    case OMX_EventBufferFlag:
      if (ctx->m_omx_decoder == hComponent && (nData2 & OMX_BUFFERFLAG_EOS)) {
        #if defined(OMX_DEBUG_EVENTHANDLER)
        if(ctx->m_omx_input_port  == (int)nData1)
            CLog::Log(LOGDEBUG, "%s::%s - OMX_EventBufferFlag(input)\n",
            CLASSNAME, __func__);
        #endif
        if(ctx->m_omx_output_port == (int)nData1)
        {
            ctx->m_videoplayback_done = true;
            #if defined(OMX_DEBUG_EVENTHANDLER)
            CLog::Log(LOGDEBUG, "%s::%s - OMX_EventBufferFlag(output)\n",
            CLASSNAME, __func__);
            #endif
        }
      }
    break;
    case OMX_EventPortSettingsChanged:
      #if defined(OMX_DEBUG_EVENTHANDLER)
      CLog::Log(LOGDEBUG,
        "%s::%s - OMX_EventPortSettingsChanged(output)\n", CLASSNAME, __func__);
      #endif
      // not sure nData2 is the input/output ports in this call, docs don't say
      if (ctx->m_omx_output_port == (int)nData2)
      {
        // free the current OpenMax output buffers, you must do this before sending
        // OMX_CommandPortDisable to component as it expects output buffers
        // to be freed before it will issue a OMX_CommandPortDisable event.
        ctx->m_portChanging = true;
        omx_err = OMX_SendCommand(ctx->m_omx_decoder, OMX_CommandPortDisable, ctx->m_omx_output_port, NULL);
        omx_err = ctx->FreeOMXOutputBuffers(false);
      }
    break;
    #if defined(OMX_DEBUG_EVENTHANDLER)
    case OMX_EventMark:
      CLog::Log(LOGDEBUG, "%s::%s - OMX_EventMark\n", CLASSNAME, __func__);
    break;
    case OMX_EventResourcesAcquired:
      CLog::Log(LOGDEBUG, "%s::%s - OMX_EventResourcesAcquired\n", CLASSNAME, __func__);
    break;
    #endif
    case OMX_EventError:
      switch((OMX_S32)nData1)
      {
        case OMX_ErrorInsufficientResources:
          CLog::Log(LOGERROR, "%s::%s - OMX_EventError, insufficient resources\n",
            CLASSNAME, __func__);
          // we are so frack'ed
          //exit(0);
        break;
        case OMX_ErrorFormatNotDetected:
          CLog::Log(LOGERROR, "%s::%s - OMX_EventError, cannot parse input stream\n",
            CLASSNAME, __func__);
        break;
        case OMX_ErrorPortUnpopulated:
          // silently ignore these. We can get them when setting OMX_CommandPortDisable
          // on the output port and the component flushes the output buffers.
        break;
        case OMX_ErrorStreamCorrupt:
          CLog::Log(LOGERROR, "%s::%s - OMX_EventError, Bitstream corrupt\n",
            CLASSNAME, __func__);
          ctx->m_videoplayback_done = true;
        break;
        default:
          CLog::Log(LOGERROR, "%s::%s - OMX_EventError detected, nData1(0x%lx), nData2(0x%lx)\n",
            CLASSNAME, __func__, nData1, nData2);
        break;
      }
      // do this so we don't hang on errors
      /*
      sem_post(ctx->m_omx_flush_input);
      sem_post(ctx->m_omx_flush_output);
      */
      sem_post(ctx->m_omx_decoder_state_change);
    break;
    default:
      CLog::Log(LOGWARNING,
        "%s::%s - Unknown eEvent(0x%x), nData1(0x%lx), nData2(0x%lx)\n",
        CLASSNAME, __func__, eEvent, nData1, nData2);
    break;
  }

  return OMX_ErrorNone;
}

// StartPlayback -- Kick off video playback.
OMX_ERRORTYPE COpenMaxVideo::StartDecoder(void)
{
  OMX_ERRORTYPE omx_err;

  #if defined(OMX_DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, "%s::%s\n", CLASSNAME, __func__);
  #endif

  // transition decoder component to IDLE state
  omx_err = SetStateForComponent(OMX_StateIdle);
  if (omx_err)
  {
    CLog::Log(LOGERROR, "%s::%s - setting OMX_StateIdle failed with omx_err(0x%x)\n",
      CLASSNAME, __func__, omx_err);
    return omx_err;
  }

  // transition decoder component to executing state
  omx_err = SetStateForComponent(OMX_StateExecuting);
  if (omx_err)
  {
    CLog::Log(LOGERROR, "%s::%s - setting OMX_StateExecuting failed with omx_err(0x%x)\n",
      CLASSNAME, __func__, omx_err);
    return omx_err;
  }

  //prime the omx output buffers.
  PrimeFillBuffers();

  return omx_err;
}

// StopPlayback -- Stop video playback
OMX_ERRORTYPE COpenMaxVideo::StopDecoder(void)
{
  OMX_ERRORTYPE omx_err;

  #if defined(OMX_DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, "%s::%s\n", CLASSNAME, __func__);
  #endif
  // transition decoder component from executing to idle
  omx_err = SetStateForComponent(OMX_StateIdle);
  if (omx_err)
  {
    CLog::Log(LOGERROR, "%s::%s - setting OMX_StateIdle failed with omx_err(0x%x)\n",
      CLASSNAME, __func__, omx_err);
    return omx_err;
  }

  // we can free our allocated port buffers in OMX_StateIdle state.
  // free OpenMax input buffers.
  omx_err = FreeOMXInputBuffers(true);
  // free OpenMax output buffers.
  omx_err = FreeOMXOutputBuffers(true);

  // transition decoder component from idle to loaded
  omx_err = SetStateForComponent(OMX_StateLoaded);
  if (omx_err)
    CLog::Log(LOGERROR,
      "%s::%s - setting OMX_StateLoaded failed with omx_err(0x%x)\n",
      CLASSNAME, __func__, omx_err);

  return omx_err;
}

