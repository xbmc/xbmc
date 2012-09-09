#pragma once

/*
 *      Copyright (C) 2005-2012 Team XBMC
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

#include "DynamicDll.h"

#include <librtmp/log.h>
#include <librtmp/rtmp.h>

class DllLibRTMPInterface
{
public:
  virtual ~DllLibRTMPInterface() {}
  virtual void LogSetLevel(int level)=0;
  virtual void LogSetCallback(RTMP_LogCallback* cb)=0;
  virtual RTMP *Alloc(void)=0;
  virtual void Free(RTMP* r)=0;
  virtual void Init(RTMP* r)=0;
  virtual void Close(RTMP* r)=0;
  virtual bool SetupURL(RTMP* r, char* url)=0;
  virtual bool SetOpt(RTMP* r, const AVal* opt, AVal* arg)=0;
  virtual bool Connect(RTMP* r, RTMPPacket* cp)=0;
  virtual bool ConnectStream(RTMP* r, int seekTime)=0;
  virtual int Read(RTMP* r, char* buf, int size)=0;
  virtual bool SendSeek(RTMP*r, int dTime)=0;
  virtual bool Pause(RTMP* r, bool DoPause)=0;
};

class DllLibRTMP : public DllDynamic, DllLibRTMPInterface
{
  DECLARE_DLL_WRAPPER(DllLibRTMP, DLL_PATH_LIBRTMP)
  DEFINE_METHOD1(void, LogSetLevel,    (int p1))
  DEFINE_METHOD1(void, LogSetCallback, (RTMP_LogCallback* p1))
  DEFINE_METHOD0(RTMP *, Alloc         )
  DEFINE_METHOD1(void, Free,           (RTMP* p1))
  DEFINE_METHOD1(void, Init,           (RTMP* p1))
  DEFINE_METHOD1(void, Close,          (RTMP* p1))
  DEFINE_METHOD2(bool, SetupURL,       (RTMP* p1, char* p2))
  DEFINE_METHOD3(bool, SetOpt,         (RTMP* p1, const AVal* p2, AVal* p3))
  DEFINE_METHOD2(bool, Connect,        (RTMP* p1, RTMPPacket* p2))
  DEFINE_METHOD2(bool, ConnectStream,  (RTMP* p1, int p2))
  DEFINE_METHOD3(int,  Read,           (RTMP* p1, char* p2, int p3))
  DEFINE_METHOD2(bool, SendSeek,       (RTMP* p1, int p2))
  DEFINE_METHOD2(bool, Pause,          (RTMP* p1, bool p2))
  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD_RENAME(RTMP_LogSetLevel,LogSetLevel)
    RESOLVE_METHOD_RENAME(RTMP_LogSetCallback,LogSetCallback)
    RESOLVE_METHOD_RENAME(RTMP_Alloc,Alloc)
    RESOLVE_METHOD_RENAME(RTMP_Free,Free)
    RESOLVE_METHOD_RENAME(RTMP_Init,Init)
    RESOLVE_METHOD_RENAME(RTMP_Close,Close)
    RESOLVE_METHOD_RENAME(RTMP_SetupURL,SetupURL)
    RESOLVE_METHOD_RENAME(RTMP_SetOpt,SetOpt)
    RESOLVE_METHOD_RENAME(RTMP_Connect,Connect)
    RESOLVE_METHOD_RENAME(RTMP_ConnectStream,ConnectStream)
    RESOLVE_METHOD_RENAME(RTMP_Read,Read)
    RESOLVE_METHOD_RENAME(RTMP_SendSeek,SendSeek)
    RESOLVE_METHOD_RENAME(RTMP_Pause,Pause)
  END_METHOD_RESOLVE()
};

