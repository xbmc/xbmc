#ifndef __XBMC_PVR_H__
#define __XBMC_PVR_H__

#ifdef HAS_XBOX_HARDWARE
#include <xtl.h>
#else
#ifndef _LINUX
#include <windows.h>
#else
#define __cdecl
#define __declspec(x)
#include <time.h>
#endif
#endif

#include "xbmc_addon_dll.h"               /* Dll related functions available to all AddOn's */
#include "xbmc_pvr_types.h"

extern "C"
{
  // Functions that your PVR client must implement, also you must implement the functions from
  // xbmc_addon.h
  PVR_ERROR GetProperties(PVR_SERVERPROPS* pProps);
  const char* GetBackendName();
  const char* GetBackendVersion();
  const char* GetConnectionString();
  PVR_ERROR GetDriveSpace(long long *total, long long *used);
  int GetNumBouquets();
  int GetNumChannels();
  int GetNumRecordings();
  int GetNumTimers();
//  PVR_ERROR GetBouquetInfo(const unsigned number, PVR_BOUQUET *info);
  PVR_ERROR RequestChannelList(PVRHANDLE handle);
//  PVR_ERROR GetEPGForChannel(const unsigned channel, PVR_PROGLIST **epg, time_t start, time_t end);
//  PVR_ERROR GetEPGNowInfo(const unsigned channel, PVR_PROGINFO *result);
//  PVR_ERROR GetEPGNextInfo(const unsigned channel, PVR_PROGINFO *result);
//  PVR_ERROR GetEPGDataEnd(time_t *end);

  // function to export the above structure to XBMC
  void __declspec(dllexport) get_addon(struct PVRClient* pClient)
  {
    pClient->GetProperties = GetProperties;
    pClient->GetBackendName = GetBackendName;
    pClient->GetBackendVersion = GetBackendVersion;
    pClient->GetConnectionString = GetConnectionString;
    pClient->GetDriveSpace = GetDriveSpace;
    pClient->GetNumBouquets = GetNumBouquets;
    pClient->GetNumChannels = GetNumChannels;
    pClient->GetNumRecordings = GetNumRecordings;
    pClient->GetNumTimers = GetNumTimers;
    pClient->RequestChannelList = RequestChannelList;
//    pClient->GetBouquetInfo = GetBouquetInfo;
//    pClient->GetChannelList = GetChannelList;
//    pClient->GetEPGForChannel = GetEPGForChannel;
//    pClient->GetEPGNowInfo = GetEPGNowInfo;
//    pClient->GetEPGNextInfo = GetEPGNextInfo;
//    pClient->GetEPGDataEnd = GetEPGDataEnd;
  };
};

#endif
