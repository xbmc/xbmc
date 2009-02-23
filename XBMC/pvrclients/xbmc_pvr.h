#ifndef __XBMC_PVR_H__
#define __XBMC_PVR_H__

#ifndef _LINUX
#ifdef USE_DLL
#define PVRAPI __declspec(dllimport)
#else
#define PVRAPI __declspec(dllexport)
#endif
#else
#define PVRAPI

#if !defined(__int8)
#define __int8 char
#endif

#if !defined(__int32)
#define __int32 long
#endif
 
#if !defined(__int64)
#define __int64 long long
#endif

#include <time.h>
#endif
#include <vector>

#include "PVRClientTypes.h"

using namespace std;


extern "C"
{
  // the settings vector
  static vector<PVRSetting> m_vecSettings;

  // Functions that your client must implement
  PVR_ERROR Create(PVRCallbacks*);
  PVR_ERROR GetProperties(PVR_SERVERPROPS* pProps);
  void GetSettings(vector<PVRSetting> **vecSettings);
  void UpdateSetting(int num);
  PVR_ERROR GetProperties(PVR_SERVERPROPS *props);
  PVR_ERROR Connect();
  void Disconnect();
  bool IsUp();
  const char* GetBackendName();
  const char* GetBackendVersion();
  PVR_ERROR GetDriveSpace(long long *total, long long *used);
  int GetNumBouquets();
  PVR_ERROR GetBouquetInfo(const unsigned number, PVR_BOUQUET *info);
  int GetNumChannels();
  PVR_ERROR GetChannelList(PVR_CHANLIST *channels);
  PVR_ERROR GetEPGForChannel(const unsigned channel, PVR_PROGLIST *epg, time_t start, time_t end);
  PVR_ERROR GetEPGNowInfo(const unsigned channel, PVR_PROGINFO *result);
  PVR_ERROR GetEPGNextInfo(const unsigned channel, PVR_PROGINFO *result);
  PVR_ERROR GetEPGDataEnd(time_t *end);

  // function to export the above structure to XBMC
  void __declspec(dllexport) get_plugin(struct PVRClient* pClient)
  {
	  pClient->Create = Create;
    pClient->GetSettings = GetSettings;
    pClient->UpdateSetting = UpdateSetting;
    pClient->GetProperties = GetProperties;
    pClient->Connect = Connect;
    pClient->Disconnect = Disconnect;
    pClient->IsUp = IsUp;
    pClient->GetBackendName = GetBackendName;
    pClient->GetBackendVersion = GetBackendVersion;
    pClient->GetDriveSpace = GetDriveSpace;
    pClient->GetNumBouquets = GetNumBouquets;
    pClient->GetBouquetInfo = GetBouquetInfo;
    pClient->GetNumChannels = GetNumChannels;
    pClient->GetChannelList = GetChannelList;
    pClient->GetEPGForChannel = GetEPGForChannel;
    pClient->GetEPGNowInfo = GetEPGNowInfo;
    pClient->GetEPGNextInfo = GetEPGNextInfo;
    pClient->GetEPGDataEnd = GetEPGDataEnd;
  };
};

#endif
