#pragma once

/*
 *      Copyright (C) 2013 Team XBMC
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

#if (defined HAVE_CONFIG_H) && (!defined TARGET_WINDOWS)
#include "config.h"
#endif

#include "DynamicDll.h"
#include "DVDVideoCodec.h"

class CApplication;
class CWinSystemEGL;
class CAdvancedSettings;

namespace KODI
{
  namespace MESSAGING
  {
    class CApplicationMessenger;
  }
}

class DllLibStageFrightCodecInterface
{
public:
  virtual ~DllLibStageFrightCodecInterface() {}

  virtual void* create_stf(CApplication* application, KODI::MESSAGING::CApplicationMessenger* applicationMessenger, CWinSystemEGL* windowing, CAdvancedSettings* advsettings)=0;
  virtual void destroy_stf(void*)=0;
  
  virtual bool stf_Open(void*, CDVDStreamInfo &hints) = 0;
  virtual void stf_Dispose(void*) = 0;
  virtual int  stf_Decode(void*, uint8_t *pData, int iSize, double dts, double pts) = 0;
  virtual void stf_Reset(void*) = 0;
  virtual bool stf_GetPicture(void*, DVDVideoPicture *pDvdVideoPicture) = 0;
  virtual bool stf_ClearPicture(void*, DVDVideoPicture* pDvdVideoPicture) = 0;
  virtual void stf_SetDropState(void*, bool bDrop) = 0;
  virtual void stf_SetSpeed(void*, int iSpeed) = 0;

  virtual void stf_LockBuffer(void*, EGLImageKHR eglimg) = 0;
  virtual void stf_ReleaseBuffer(void*, EGLImageKHR eglimg) = 0;
};

class DllLibStageFrightCodec : public DllDynamic, DllLibStageFrightCodecInterface
{
  DECLARE_DLL_WRAPPER(DllLibStageFrightCodec, DLL_PATH_LIBSTAGEFRIGHTICS)
  DEFINE_METHOD4(void*, create_stf, (CApplication* p1, KODI::MESSAGING::CApplicationMessenger* p2, CWinSystemEGL* p3, CAdvancedSettings* p4))
  DEFINE_METHOD1(void, destroy_stf, (void* p1))
  DEFINE_METHOD2(bool, stf_Open, (void* p1, CDVDStreamInfo &p2))
  DEFINE_METHOD1(void, stf_Dispose, (void* p1))
  DEFINE_METHOD5(int, stf_Decode, (void* p1, uint8_t *p2, int p3, double p4, double p5))
  DEFINE_METHOD1(void, stf_Reset, (void* p1))
  DEFINE_METHOD2(bool, stf_GetPicture, (void* p1, DVDVideoPicture * p2))
  DEFINE_METHOD2(bool, stf_ClearPicture, (void* p1, DVDVideoPicture * p2))
  DEFINE_METHOD2(void, stf_SetDropState, (void* p1, bool p2))
  DEFINE_METHOD2(void, stf_SetSpeed, (void* p1, int p2))
  DEFINE_METHOD2(void, stf_LockBuffer, (void* p1, EGLImageKHR p2))
  DEFINE_METHOD2(void, stf_ReleaseBuffer, (void* p1, EGLImageKHR p2))
  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD(create_stf)
    RESOLVE_METHOD(destroy_stf)
    RESOLVE_METHOD(stf_Open)
    RESOLVE_METHOD(stf_Dispose)
    RESOLVE_METHOD(stf_Decode)
    RESOLVE_METHOD(stf_Reset)
    RESOLVE_METHOD(stf_GetPicture)
    RESOLVE_METHOD(stf_ClearPicture)
    RESOLVE_METHOD(stf_SetDropState)
    RESOLVE_METHOD(stf_SetSpeed)
    RESOLVE_METHOD(stf_LockBuffer)
    RESOLVE_METHOD(stf_ReleaseBuffer)
  END_METHOD_RESOLVE()
};
