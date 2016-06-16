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

#include "cores/VideoPlayer/DVDStreamInfo.h"
#include "DVDVideoCodec.h"
#include "threads/Event.h"

#include <queue>
#include <string>
#include <vector>
#include <semaphore.h>
#include <OMX_Core.h>


// check for potentially undefined OpenMAX version numbers
#ifndef OMX_VERSION_MAJOR
#define OMX_VERSION_MAJOR 1
#endif
#ifndef OMX_VERSION_MINOR
#define OMX_VERSION_MINOR 1
#endif
#ifndef OMX_VERSION_REVISION
#define OMX_VERSION_REVISION 2
#endif
#ifndef OMX_VERSION_STEP
#define OMX_VERSION_STEP 0
#endif

////////////////////////////////////////////////////////////////////////////////////////////
// debug spew defines
#if 0
#define OMX_DEBUG_VERBOSE
#define OMX_DEBUG_EVENTHANDLER
#define OMX_DEBUG_FILLBUFFERDONE
#define OMX_DEBUG_EMPTYBUFFERDONE
#endif

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

class DllLibOpenMax;
class COpenMax
{
public:
  COpenMax();
  virtual ~COpenMax();

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

  // initialize OpenMax and get decoder component
  bool Initialize( const std::string &decoder_name);
  void Deinitialize();

  // OpenMax Decoder delegate callback routines.
  static OMX_ERRORTYPE DecoderEventHandlerCallback(OMX_HANDLETYPE hComponent, OMX_PTR pAppData,
    OMX_EVENTTYPE eEvent, OMX_U32 nData1, OMX_U32 nData2, OMX_PTR pEventData);
  static OMX_ERRORTYPE DecoderEmptyBufferDoneCallback(
    OMX_HANDLETYPE hComponent, OMX_PTR pAppData, OMX_BUFFERHEADERTYPE* pBuffer);
  static OMX_ERRORTYPE DecoderFillBufferDoneCallback(
    OMX_HANDLETYPE hComponent, OMX_PTR pAppData, OMX_BUFFERHEADERTYPE* pBufferHeader);

  // OpenMax decoder callback routines.
  virtual OMX_ERRORTYPE DecoderEventHandler(OMX_HANDLETYPE hComponent, OMX_PTR pAppData,
    OMX_EVENTTYPE eEvent, OMX_U32 nData1, OMX_U32 nData2, OMX_PTR pEventData);
  virtual OMX_ERRORTYPE DecoderEmptyBufferDone(
    OMX_HANDLETYPE hComponent, OMX_PTR pAppData, OMX_BUFFERHEADERTYPE* pBuffer);
  virtual OMX_ERRORTYPE DecoderFillBufferDone(
    OMX_HANDLETYPE hComponent, OMX_PTR pAppData, OMX_BUFFERHEADERTYPE* pBufferHeader);

  // OpenMax helper routines
  OMX_ERRORTYPE WaitForState(OMX_STATETYPE state);
  OMX_ERRORTYPE SetStateForComponent(OMX_STATETYPE state);

  DllLibOpenMax     *m_dll;
  bool              m_is_open;
  OMX_HANDLETYPE    m_omx_decoder;   // openmax decoder component reference

  // OpenMax state tracking
  OMX_CLIENT_STATE  m_omx_client_state;
  volatile int      m_omx_decoder_state;
  sem_t             m_omx_decoder_state_change;
  std::vector<omx_codec_capability> m_omx_decoder_capabilities;

private:
  COpenMax(const COpenMax& other);
  COpenMax& operator=(const COpenMax&);
};

#endif
