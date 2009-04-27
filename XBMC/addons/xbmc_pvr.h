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


#include "PVRClientTypes.h"
#include <vector>



using namespace std;


extern "C"
{
  //TODO inherit these functions from xbmc_addon.h
  bool __declspec(dllexport) HasSettings();
  __declspec(dllexport) DllSettings* GetSettings();


  // Functions that your client must implement
  PVR_ERROR Create(PVRCallbacks*);
  PVR_ERROR GetProperties(PVR_SERVERPROPS* pProps);
  PVR_ERROR Connect();
  void Disconnect();
  bool IsUp();
  const char* GetBackendName();
  const char* GetBackendVersion();
  const char* GetConnectionString();
  PVR_ERROR GetDriveSpace(long long *total, long long *used);
//  int GetNumBouquets();
//  PVR_ERROR GetBouquetInfo(const unsigned number, PVR_BOUQUET *info);
//  int GetNumChannels();
//  unsigned int GetChannelList(PVR_CHANNEL ***channels);
//  PVR_ERROR GetEPGForChannel(const unsigned channel, PVR_PROGLIST **epg, time_t start, time_t end);
//  PVR_ERROR GetEPGNowInfo(const unsigned channel, PVR_PROGINFO *result);
//  PVR_ERROR GetEPGNextInfo(const unsigned channel, PVR_PROGINFO *result);
//  PVR_ERROR GetEPGDataEnd(time_t *end);

  // function to export the above structure to XBMC
  void __declspec(dllexport) get_addon(struct PVRClient* pClient)
  {
    pClient->Create = Create;
    pClient->GetProperties = GetProperties;
    pClient->Connect = Connect;
    pClient->Disconnect = Disconnect;
    pClient->IsUp = IsUp;
    pClient->GetBackendName = GetBackendName;
    pClient->GetBackendVersion = GetBackendVersion;
    pClient->GetConnectionString = GetConnectionString;
    pClient->GetDriveSpace = GetDriveSpace;
//    pClient->GetNumBouquets = GetNumBouquets;
//    pClient->GetBouquetInfo = GetBouquetInfo;
//    pClient->GetNumChannels = GetNumChannels;
//    pClient->GetChannelList = GetChannelList;
//    pClient->GetEPGForChannel = GetEPGForChannel;
//    pClient->GetEPGNowInfo = GetEPGNowInfo;
//    pClient->GetEPGNextInfo = GetEPGNextInfo;
//    pClient->GetEPGDataEnd = GetEPGDataEnd;
  };
};

#endif
