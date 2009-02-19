#pragma once
/*
*      Copyright (C) 2005-2009 Team XBMC
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
#include "DynamicDll.h"
#include "../../pvrclients/PVRClientTypes.h"

class DllPVRClientInterface
{
public:
  void GetPlugin(struct PVRClient *pClient);
};

//  long GetID()=0;
//  PVR_ERROR GetProperties(PVR_SERVERPROPS *props)=0;
//  PVR_ERROR Connect()=0;
//  void Disconnect();
//  bool IsUp()=0;
//  const char* GetBackendName()=0;
//  const char* GetBackendVersion()=0;
//  PVR_ERROR GetDriveSpace(long long *total, long long *used)=0;
//  int GetNumBouquets()=0;
//  PVR_ERROR GetBouquetInfo(const unsigned number, PVR_BOUQUET *info)=0;
//  int GetNumChannels()=0;
//  PVR_ERROR GetChannelList(PVR_CHANLIST *channels)=0;
//  PVR_ERROR GetEPGForChannel(const unsigned channel, PVR_PROGLIST *epg, time_t start, time_t end)=0;
//  PVR_ERROR GetEPGNowInfo(const unsigned channel, PVR_PROGINFO *result)=0;
//  PVR_ERROR GetEPGNextInfo(const unsigned channel, PVR_PROGINFO *result)=0;
//  PVR_ERROR GetEPGDataEnd(time_t *end);
//};

class DllPVRClient : public DllDynamic, DllPVRClientInterface
{
  DECLARE_DLL_WRAPPER_TEMPLATE(DllPVRClient)
  DEFINE_METHOD1(void, GetPlugin, (struct PVRClient* p1))
  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD_RENAME(get_plugin,GetPlugin)
  END_METHOD_RESOLVE()

  /*DEFINE_METHOD0(long, GetID)
  DEFINE_METHOD1(PVR_ERROR, GetProperties, (PVR_SERVERPROPS *p1))
  DEFINE_METHOD0(PVR_ERROR, Connect)
  DEFINE_METHOD0(void, Disconnect)
  DEFINE_METHOD0(bool, IsUp)
  DEFINE_METHOD0(const char*, GetBackendName)
  DEFINE_METHOD0(const char*, GetBackendVersion)
  DEFINE_METHOD2(PVR_ERROR, GetDriveSpace, (long long *p1, long long *p2))
  DEFINE_METHOD0(int, GetNumBouquets)
  DEFINE_METHOD2(PVR_ERROR, GetBouquetInfo, (const unsigned p1, PVR_BOUQUET *p2))
  DEFINE_METHOD0(int, GetNumChannels)
  DEFINE_METHOD1(PVR_ERROR, GetChannelList, (PVR_CHANLIST *p1))
  DEFINE_METHOD4(PVR_ERROR, GetEPGForChannel, (const unsigned p1, PVR_PROGLIST *p2, time_t p3, time_t p4))
  DEFINE_METHOD2(PVR_ERROR, GetEPGNowInfo, (const unsigned p1, PVR_PROGINFO *p2))
  DEFINE_METHOD2(PVR_ERROR, GetEPGNextInfo, (const unsigned p1, PVR_PROGINFO *p2))
  DEFINE_METHOD1(PVR_ERROR, GetEPGDataEnd, (time_t *p1))
  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD(GetID)
    RESOLVE_METHOD(GetProperties)
    RESOLVE_METHOD(Connect)
    RESOLVE_METHOD(Disconnect)
    RESOLVE_METHOD(IsUp)
    RESOLVE_METHOD(GetBackendName)
    RESOLVE_METHOD(GetBackendVersion)
    RESOLVE_METHOD(GetDriveSpace)
    RESOLVE_METHOD(GetNumBouquets)
    RESOLVE_METHOD(GetBouquetInfo)
    RESOLVE_METHOD(GetNumChannels)
    RESOLVE_METHOD(GetChannelList)
    RESOLVE_METHOD(GetEPGForChannel)
    RESOLVE_METHOD(GetEPGNowInfo)
    RESOLVE_METHOD(GetEPGNextInfo)
    RESOLVE_METHOD(GetEPGDataEnd)
  END_METHOD_RESOLVE()*/
};

