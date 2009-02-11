#include "../../../pvrclients/xbmc_pvr.h"

//-- GetInfo ------------------------------------------------------------------
// Tell XBMC our requirements
//-----------------------------------------------------------------------------
extern "C" void GetProps(PVR_SERVERPROPS* pProps)
{
  pProps->Name = "VDR";
  pProps->SupportEPG = true;
  pProps->HasBouquets = true;
  pProps->HasUnique = true;
  pProps->SupportRadio = true;
  pProps->SupportRecordings = true;
  pProps->SupportTimers = true;
  pProps->DefaultHostname = "vdr";
  pProps->DefaultPort = 2004;
  pProps->DefaultUser = "vdr";
  pProps->DefaultPassword = "vdr";
}

//-- GetSettings --------------------------------------------------------------
// Return the settings for XBMC to display
//-----------------------------------------------------------------------------
extern "C" void GetSettings(vector<PVRSetting> **vecSettings)
{
  return;
}

//-- UpdateSetting ------------------------------------------------------------
// Handle setting change request from XBMC
//-----------------------------------------------------------------------------
extern "C" void UpdateSetting(int num)
{

}

