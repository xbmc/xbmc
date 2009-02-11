#ifndef __XBMC_PVR_H__
#define __XBMC_PVR_H__

#include <vector>

#include "../xbmc/pvrclients/PVRClientTypes.h"

using namespace std;


extern "C"
{
  // the settings vector
  vector<PVRSetting> m_vecSettings;

  // Functions that your client must implement
  void GetProps(PVR_SERVERPROPS* pProps);
  void GetSettings(vector<PVRSetting> **vecSettings);
  void UpdateSetting(int num);

  // function to export the above structure to XBMC
  void __declspec(dllexport) get_module(struct PVRClient* pClient)
  {
    pClient->GetProps = GetProps;
    pClient->GetSettings = GetSettings;
    pClient->UpdateSetting = UpdateSetting;
  };
};

#endif
