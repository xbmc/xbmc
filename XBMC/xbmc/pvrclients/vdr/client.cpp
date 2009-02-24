/*
*      Copyright (C) 2005-2009 Team XBMC
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
*  along with XBMC; see the file COPYING.  If not, write to
*  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
*  http://www.gnu.org/copyleft/gpl.html
*
*/
#include "../../../pvrclients/xbmc_pvr.h"
#include "PVRClient-vdr.h"


// global callback for logging via XBMC
PVRClientVDR *g_client;
static PVREventCallback OnEvent;
static PVRLogCallback OnLog;
static void *Wrapper;
static bool created = false;

//////////////////////////////////////////////////////////////////////////////
extern "C" PVR_ERROR Create(PVRCallbacks *callbacks)
{ 
  OnEvent = callbacks->Event;
  OnLog = callbacks->Log;
  Wrapper = callbacks->userData;

  g_client = new PVRClientVDR(callbacks);
  created = true;

  return PVR_ERROR_NO_ERROR;
}

extern "C" void GetSettings(vector<PVRSetting> **vecSettings)
{
  return;
}

extern "C" void UpdateSetting(int num)
{

}

//-- GetProperties ------------------------------------------------------------------
// Tell XBMC our requirements
//-----------------------------------------------------------------------------
extern "C" PVR_ERROR GetProperties(PVR_SERVERPROPS* props)
{
return g_client->GetProperties(props);
}

extern "C" PVR_ERROR Connect()
{
  if (!created)
    return PVR_ERROR_UNKOWN;
  
  return g_client->Connect();
}

extern "C" void Disconnect()
{
  g_client->Disconnect();
}

extern "C" bool IsUp()
{
  return g_client->IsUp();
}

extern "C" const char * GetBackendName()
{
  return g_client->GetBackendName();
}

extern "C" const char * GetBackendVersion()
{
  return g_client->GetBackendVersion();
}

extern "C" PVR_ERROR GetDriveSpace(long long *total, long long *used)
{
  int percent;
  return g_client->GetDriveSpace(total, used, &percent);
}

extern "C" int GetNumBouquets()
{
  return 0;
}

extern "C" PVR_ERROR GetBouquetInfo(const unsigned number, PVR_BOUQUET *info)
{
  return PVR_ERROR_NO_ERROR;
}

extern "C" int GetNumChannels()
{
  return g_client->GetNumChannels();
}

extern "C" PVR_ERROR GetChannelList(PVR_CHANLIST *channels)
{
  return g_client->GetAllChannels(channels);
}

extern "C" PVR_ERROR GetEPGForChannel(const unsigned channel, PVR_PROGLIST *epg, time_t start, time_t end)
{
  return g_client->GetEPGForChannel(channel, epg, start, end);
}

extern "C" PVR_ERROR GetEPGNowInfo(const unsigned channel, PVR_PROGINFO *result)
{
  return PVR_ERROR_NO_ERROR; /*g_client->GetEPGNowInfo(channel, result);*/
}

extern "C" PVR_ERROR GetEPGNextInfo(const unsigned channel, PVR_PROGINFO *result)
{
  return PVR_ERROR_NO_ERROR; /*g_client->GetEPGNextInfo(channel, result);*/
}

extern "C" PVR_ERROR GetEPGDataEnd(time_t *end)
{
  /*return g_client->GetEPGDataEnd(end);*/
  return PVR_ERROR_NO_ERROR;
}
