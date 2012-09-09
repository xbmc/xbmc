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

#if defined(HAVE_LIBOPENMAX)
#include "OpenMax.h"
#include "DynamicDll.h"
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

#define CLASSNAME "COpenMax"


////////////////////////////////////////////////////////////////////////////////////////////
class DllLibOpenMaxInterface
{
public:
  virtual ~DllLibOpenMaxInterface() {}

  virtual OMX_ERRORTYPE OMX_Init(void) = 0;
  virtual OMX_ERRORTYPE OMX_Deinit(void) = 0;
  virtual OMX_ERRORTYPE OMX_GetHandle(
    OMX_HANDLETYPE *pHandle, OMX_STRING cComponentName, OMX_PTR pAppData, OMX_CALLBACKTYPE *pCallBacks) = 0;
  virtual OMX_ERRORTYPE OMX_FreeHandle(OMX_HANDLETYPE hComponent) = 0;
  virtual OMX_ERRORTYPE OMX_GetComponentsOfRole(OMX_STRING role, OMX_U32 *pNumComps, OMX_U8 **compNames) = 0;
  virtual OMX_ERRORTYPE OMX_GetRolesOfComponent(OMX_STRING compName, OMX_U32 *pNumRoles, OMX_U8 **roles) = 0;
  virtual OMX_ERRORTYPE OMX_ComponentNameEnum(OMX_STRING cComponentName, OMX_U32 nNameLength, OMX_U32 nIndex) = 0;
};

class DllLibOpenMax : public DllDynamic, DllLibOpenMaxInterface
{
  DECLARE_DLL_WRAPPER(DllLibOpenMax, "/usr/lib/libnvomx.so")

  DEFINE_METHOD0(OMX_ERRORTYPE, OMX_Init)
  DEFINE_METHOD0(OMX_ERRORTYPE, OMX_Deinit)
  DEFINE_METHOD4(OMX_ERRORTYPE, OMX_GetHandle, (OMX_HANDLETYPE *p1, OMX_STRING p2, OMX_PTR p3, OMX_CALLBACKTYPE *p4))
  DEFINE_METHOD1(OMX_ERRORTYPE, OMX_FreeHandle, (OMX_HANDLETYPE p1))
  DEFINE_METHOD3(OMX_ERRORTYPE, OMX_GetComponentsOfRole, (OMX_STRING p1, OMX_U32 *p2, OMX_U8 **p3))
  DEFINE_METHOD3(OMX_ERRORTYPE, OMX_GetRolesOfComponent, (OMX_STRING p1, OMX_U32 *p2, OMX_U8 **p3))
  DEFINE_METHOD3(OMX_ERRORTYPE, OMX_ComponentNameEnum, (OMX_STRING p1, OMX_U32 p2, OMX_U32 p3))
  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD(OMX_Init)
    RESOLVE_METHOD(OMX_Deinit)
    RESOLVE_METHOD(OMX_GetHandle)
    RESOLVE_METHOD(OMX_FreeHandle)
    RESOLVE_METHOD(OMX_GetComponentsOfRole)
    RESOLVE_METHOD(OMX_GetRolesOfComponent)
    RESOLVE_METHOD(OMX_ComponentNameEnum)
  END_METHOD_RESOLVE()
};

////////////////////////////////////////////////////////////////////////////////////////////
#define OMX_INIT_STRUCTURE(a) \
  memset(&(a), 0, sizeof(a)); \
  (a).nSize = sizeof(a); \
  (a).nVersion.s.nVersionMajor = OMX_VERSION_MAJOR; \
  (a).nVersion.s.nVersionMinor = OMX_VERSION_MINOR; \
  (a).nVersion.s.nRevision = OMX_VERSION_REVISION; \
  (a).nVersion.s.nStep = OMX_VERSION_STEP

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
COpenMax::COpenMax()
{
  #if defined(OMX_DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, "%s::%s\n", CLASSNAME, __func__);
  #endif
  m_dll = new DllLibOpenMax;
  m_dll->Load();
  m_is_open = false;

  m_omx_decoder = NULL;
  /*
  m_omx_flush_input  = (sem_t*)malloc(sizeof(sem_t));
  sem_init(m_omx_flush_input, 0, 0);
  m_omx_flush_output = (sem_t*)malloc(sizeof(sem_t));
  sem_init(m_omx_flush_output, 0, 0);
  */
}

COpenMax::~COpenMax()
{
  #if defined(OMX_DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, "%s::%s\n", CLASSNAME, __func__);
  #endif
  /*
  sem_destroy(m_omx_flush_input);
  free(m_omx_flush_input);
  sem_destroy(m_omx_flush_output);
  free(m_omx_flush_output);
  */
  delete m_dll;
}




////////////////////////////////////////////////////////////////////////////////////////////
// DecoderEventHandler -- OMX event callback
OMX_ERRORTYPE COpenMax::DecoderEventHandlerCallback(
  OMX_HANDLETYPE hComponent,
  OMX_PTR pAppData,
  OMX_EVENTTYPE eEvent,
  OMX_U32 nData1,
  OMX_U32 nData2,
  OMX_PTR pEventData)
{
  COpenMax *ctx = (COpenMax*)pAppData;
  return ctx->DecoderEventHandler(hComponent, pAppData, eEvent, nData1, nData2, pEventData);
}

// DecoderEmptyBufferDone -- OpenMax input buffer has been emptied
OMX_ERRORTYPE COpenMax::DecoderEmptyBufferDoneCallback(
  OMX_HANDLETYPE hComponent,
  OMX_PTR pAppData,
  OMX_BUFFERHEADERTYPE* pBuffer)
{
  COpenMax *ctx = (COpenMax*)pAppData;
  return ctx->DecoderEmptyBufferDone( hComponent, pAppData, pBuffer);
}

// DecoderFillBufferDone -- OpenMax output buffer has been filled
OMX_ERRORTYPE COpenMax::DecoderFillBufferDoneCallback(
  OMX_HANDLETYPE hComponent,
  OMX_PTR pAppData,
  OMX_BUFFERHEADERTYPE* pBuffer)
{
  COpenMax *ctx = (COpenMax*)pAppData;
  return ctx->DecoderFillBufferDone(hComponent, pAppData, pBuffer);
}



// Wait for a component to transition to the specified state
OMX_ERRORTYPE COpenMax::WaitForState(OMX_STATETYPE state)
{
  OMX_ERRORTYPE omx_error = OMX_ErrorNone;
  OMX_STATETYPE test_state;
  int tries = 0;
  struct timespec timeout;
  omx_error = OMX_GetState(m_omx_decoder, &test_state);

  #if defined(OMX_DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, "%s::%s - waiting for state(%d)\n", CLASSNAME, __func__, state);
  #endif
  while ((omx_error == OMX_ErrorNone) && (test_state != state)) 
  {
    clock_gettime(CLOCK_REALTIME, &timeout);
    timeout.tv_sec += 1;
    sem_timedwait(m_omx_decoder_state_change, &timeout);
    if (errno == ETIMEDOUT)
      tries++;
    if (tries > 5)
      return OMX_ErrorUndefined;

    omx_error = OMX_GetState(m_omx_decoder, &test_state);
  }

  return omx_error;
}

// SetStateForAllComponents
// Blocks until all state changes have completed
OMX_ERRORTYPE COpenMax::SetStateForComponent(OMX_STATETYPE state)
{
  OMX_ERRORTYPE omx_err;

  #if defined(OMX_DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, "%s::%s - state(%d)\n", CLASSNAME, __func__, state);
  #endif
  omx_err = OMX_SendCommand(m_omx_decoder, OMX_CommandStateSet, state, 0);
  if (omx_err)
    CLog::Log(LOGERROR, "%s::%s - OMX_CommandStateSet failed with omx_err(0x%x)\n",
      CLASSNAME, __func__, omx_err);
  else
    omx_err = WaitForState(state);

  return omx_err;
}

bool COpenMax::Initialize( const CStdString &decoder_name)
{
  OMX_ERRORTYPE omx_err = m_dll->OMX_Init();
  if (omx_err)
  {
    CLog::Log(LOGERROR,
      "%s::%s - OpenMax failed to init, status(%d), ", // codec(%d), profile(%d), level(%d)
      CLASSNAME, __func__, omx_err );//, hints.codec, hints.profile, hints.level);
    return false;
  }

  // Get video decoder handle setting up callbacks, component is in loaded state on return.
  static OMX_CALLBACKTYPE decoder_callbacks = {
    &DecoderEventHandlerCallback, &DecoderEmptyBufferDoneCallback, &DecoderFillBufferDoneCallback };
  omx_err = m_dll->OMX_GetHandle(&m_omx_decoder, (char*)decoder_name.c_str(), this, &decoder_callbacks);
  if (omx_err)
  {
    CLog::Log(LOGERROR,
      "%s::%s - could not get decoder handle\n", CLASSNAME, __func__);
    m_dll->OMX_Deinit();
    return false;
  }

  return true;
}

void COpenMax::Deinitialize()
{
  CLog::Log(LOGERROR,
    "%s::%s - failed to get component port parameter\n", CLASSNAME, __func__);
  m_dll->OMX_FreeHandle(m_omx_decoder);
  m_omx_decoder = NULL;
  m_dll->OMX_Deinit();
}

// OpenMax decoder callback routines.
OMX_ERRORTYPE COpenMax::DecoderEventHandler(OMX_HANDLETYPE hComponent, OMX_PTR pAppData,
  OMX_EVENTTYPE eEvent, OMX_U32 nData1, OMX_U32 nData2, OMX_PTR pEventData)
{
  return OMX_ErrorNone;
}

OMX_ERRORTYPE COpenMax::DecoderEmptyBufferDone(OMX_HANDLETYPE hComponent, OMX_PTR pAppData, OMX_BUFFERHEADERTYPE* pBuffer)
{
  return OMX_ErrorNone;
}

OMX_ERRORTYPE COpenMax::DecoderFillBufferDone(OMX_HANDLETYPE hComponent, OMX_PTR pAppData, OMX_BUFFERHEADERTYPE* pBufferHeader)
{
  return OMX_ErrorNone;
}

#endif

