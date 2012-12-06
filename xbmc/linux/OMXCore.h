#pragma once
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

#if defined(HAVE_OMXLIB)

#include <string>
#include <queue>

// TODO: should this be in configure
#ifndef OMX_SKIP64BIT
#define OMX_SKIP64BIT
#endif

#include "DllOMX.h"

#include <semaphore.h>

////////////////////////////////////////////////////////////////////////////////////////////
// debug spew defines
#if 0
#define OMX_DEBUG_VERBOSE
#define OMX_DEBUG_EVENTHANDLER
#endif

#define OMX_INIT_STRUCTURE(a) \
  memset(&(a), 0, sizeof(a)); \
  (a).nSize = sizeof(a); \
  (a).nVersion.s.nVersionMajor = OMX_VERSION_MAJOR; \
  (a).nVersion.s.nVersionMinor = OMX_VERSION_MINOR; \
  (a).nVersion.s.nRevision = OMX_VERSION_REVISION; \
  (a).nVersion.s.nStep = OMX_VERSION_STEP

#include "DllAvFormat.h"

#define OMX_MAX_PORTS 10

typedef struct omx_event {
  OMX_EVENTTYPE eEvent;
  OMX_U32 nData1;
  OMX_U32 nData2;
} omx_event;

class DllLibOMXCore;
class COMXCore;
class COMXCoreComponent;
class COMXCoreTunel;
class COMXCoreClock;

class COMXCoreTunel
{
public:
  COMXCoreTunel();
  ~COMXCoreTunel();

  void Initialize(COMXCoreComponent *src_component, unsigned int src_port, COMXCoreComponent *dst_component, unsigned int dst_port);
  OMX_ERRORTYPE Flush();
  OMX_ERRORTYPE Deestablish(bool noWait = false);
  OMX_ERRORTYPE Establish(bool portSettingsChanged);
private:
  pthread_mutex_t   m_lock;
  bool              m_portSettingsChanged;
  COMXCoreComponent *m_src_component;
  COMXCoreComponent *m_dst_component;
  unsigned int      m_src_port;
  unsigned int      m_dst_port;
  DllOMX            *m_DllOMX;
  bool              m_DllOMXOpen;
  void              Lock();
  void              UnLock();
};

class COMXCoreComponent
{
public:
  COMXCoreComponent();
  ~COMXCoreComponent();

  OMX_HANDLETYPE    GetComponent()   { return m_handle;        };
  unsigned int      GetInputPort()   { return m_input_port;    };
  unsigned int      GetOutputPort()  { return m_output_port;   };
  std::string       GetName()        { return m_componentName; };

  OMX_ERRORTYPE DisableAllPorts();
  void          RemoveEvent(OMX_EVENTTYPE eEvent, OMX_U32 nData1, OMX_U32 nData2);
  OMX_ERRORTYPE AddEvent(OMX_EVENTTYPE eEvent, OMX_U32 nData1, OMX_U32 nData2);
  OMX_ERRORTYPE WaitForEvent(OMX_EVENTTYPE event, long timeout = 300);
  OMX_ERRORTYPE WaitForCommand(OMX_U32 command, OMX_U32 nData2, long timeout = 2000);
  OMX_ERRORTYPE SetStateForComponent(OMX_STATETYPE state);
  OMX_STATETYPE GetState();
  OMX_ERRORTYPE SetParameter(OMX_INDEXTYPE paramIndex, OMX_PTR paramStruct);
  OMX_ERRORTYPE GetParameter(OMX_INDEXTYPE paramIndex, OMX_PTR paramStruct);
  OMX_ERRORTYPE SetConfig(OMX_INDEXTYPE configIndex, OMX_PTR configStruct);
  OMX_ERRORTYPE GetConfig(OMX_INDEXTYPE configIndex, OMX_PTR configStruct);
  OMX_ERRORTYPE SendCommand(OMX_COMMANDTYPE cmd, OMX_U32 cmdParam, OMX_PTR cmdParamData);
  OMX_ERRORTYPE EnablePort(unsigned int port, bool wait = true);
  OMX_ERRORTYPE DisablePort(unsigned int port, bool wait = true);
  OMX_ERRORTYPE UseEGLImage(OMX_BUFFERHEADERTYPE** ppBufferHdr, OMX_U32 nPortIndex, OMX_PTR pAppPrivate, void* eglImage);

  bool          Initialize( const std::string &component_name, OMX_INDEXTYPE index);
  bool          Deinitialize();

  // OMXCore Decoder delegate callback routines.
  static OMX_ERRORTYPE DecoderEventHandlerCallback(OMX_HANDLETYPE hComponent, OMX_PTR pAppData,
    OMX_EVENTTYPE eEvent, OMX_U32 nData1, OMX_U32 nData2, OMX_PTR pEventData);
  static OMX_ERRORTYPE DecoderEmptyBufferDoneCallback(
    OMX_HANDLETYPE hComponent, OMX_PTR pAppData, OMX_BUFFERHEADERTYPE* pBuffer);
  static OMX_ERRORTYPE DecoderFillBufferDoneCallback(
    OMX_HANDLETYPE hComponent, OMX_PTR pAppData, OMX_BUFFERHEADERTYPE* pBufferHeader);

  // OMXCore decoder callback routines.
  OMX_ERRORTYPE DecoderEventHandler(OMX_HANDLETYPE hComponent, OMX_PTR pAppData,
    OMX_EVENTTYPE eEvent, OMX_U32 nData1, OMX_U32 nData2, OMX_PTR pEventData);
  OMX_ERRORTYPE DecoderEmptyBufferDone(
    OMX_HANDLETYPE hComponent, OMX_PTR pAppData, OMX_BUFFERHEADERTYPE* pBuffer);
  OMX_ERRORTYPE DecoderFillBufferDone(
    OMX_HANDLETYPE hComponent, OMX_PTR pAppData, OMX_BUFFERHEADERTYPE* pBuffer);

  void TransitionToStateLoaded();

  OMX_ERRORTYPE EmptyThisBuffer(OMX_BUFFERHEADERTYPE *omx_buffer);
  OMX_ERRORTYPE FillThisBuffer(OMX_BUFFERHEADERTYPE *omx_buffer);
  OMX_ERRORTYPE FreeOutputBuffer(OMX_BUFFERHEADERTYPE *omx_buffer);

  unsigned int GetInputBufferSize();
  unsigned int GetOutputBufferSize();

  unsigned int GetInputBufferSpace();
  unsigned int GetOutputBufferSpace();

  void FlushAll();
  void FlushInput();
  void FlushOutput();

  OMX_BUFFERHEADERTYPE *GetInputBuffer(long timeout=200);
  OMX_BUFFERHEADERTYPE *GetOutputBuffer();

  OMX_ERRORTYPE AllocInputBuffers(bool use_buffers = false);
  OMX_ERRORTYPE AllocOutputBuffers(bool use_buffers = false);

  OMX_ERRORTYPE FreeInputBuffers();
  OMX_ERRORTYPE FreeOutputBuffers();

  bool IsEOS() { return m_eos; };

private:
  OMX_HANDLETYPE m_handle;
  unsigned int   m_input_port;
  unsigned int   m_output_port;
  std::string    m_componentName;
  pthread_mutex_t   m_omx_event_mutex;
  pthread_mutex_t   m_lock;
  std::vector<omx_event> m_omx_events;

  OMX_CALLBACKTYPE  m_callbacks;

  // OMXCore input buffers (demuxer packets)
  pthread_mutex_t   m_omx_input_mutex;
  std::queue<OMX_BUFFERHEADERTYPE*> m_omx_input_avaliable;
  std::vector<OMX_BUFFERHEADERTYPE*> m_omx_input_buffers;
  unsigned int  m_input_alignment;
  unsigned int  m_input_buffer_size;
  unsigned int  m_input_buffer_count;
  bool          m_omx_input_use_buffers;

  // OMXCore output buffers (video frames)
  pthread_mutex_t   m_omx_output_mutex;
  std::queue<OMX_BUFFERHEADERTYPE*> m_omx_output_available;
  std::vector<OMX_BUFFERHEADERTYPE*> m_omx_output_buffers;
  unsigned int  m_output_alignment;
  unsigned int  m_output_buffer_size;
  unsigned int  m_output_buffer_count;
  bool          m_omx_output_use_buffers;

  bool          m_exit;
  DllOMX        *m_DllOMX;
  bool          m_DllOMXOpen;
  pthread_cond_t    m_input_buffer_cond;
  pthread_cond_t    m_output_buffer_cond;
  pthread_cond_t    m_omx_event_cond;
  bool          m_eos;
  bool          m_flush_input;
  bool          m_flush_output;
  void              Lock();
  void              UnLock();
};

class COMXCore
{
public:
  COMXCore();
  ~COMXCore();

  // initialize OMXCore and get decoder component
  bool Initialize();
  void Deinitialize();

protected:
  bool              m_is_open;
  bool              m_Initialized;
  DllOMX            *m_DllOMX;
};

#endif

