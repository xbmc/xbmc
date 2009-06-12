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
#ifdef HAS_XBOX_HARDWARE
#include <xtl.h>
#endif

#include "pvrclient-vdr_os.h"
#include "../../addons/include/xbmc_pvr.h"
#include "pvrclient-vdr.h"

// global callback for logging via XBMC
PVRClientVDR *g_client;
static PVREventCallback OnEvent;
static PVRLogCallback OnLog;
static void *Wrapper;
static bool created = false;

//////////////////////////////////////////////////////////////////////////////
extern "C" PVR_ERROR Create(PVRCallbacks *callbacks)
{
  OnEvent   = callbacks->Event;
  OnLog     = callbacks->Log;
  Wrapper   = callbacks->userData;

  g_client  = new PVRClientVDR(callbacks);
  created   = true;

  return PVR_ERROR_NO_ERROR;
}

extern "C" PVR_ERROR GetProperties(PVR_SERVERPROPS* pProps)
{
  return g_client->GetProperties(pProps);
}

extern "C" PVR_ERROR SetUserSetting(const char *settingName, const void *settingValue)
{
  return g_client->SetUserSetting(settingName, settingValue);
}

extern "C" PVR_ERROR Connect()
{
  return g_client->Connect();
}

extern "C" void Disconnect()
{
  return g_client->Disconnect();
}

extern "C" bool IsUp()
{
  return g_client->IsUp();
}

extern "C" const char* GetBackendName()
{
  return g_client->GetBackendName();
}

extern "C" const char* GetBackendVersion()
{
  return g_client->GetBackendVersion();
}

extern "C" PVR_ERROR GetDriveSpace(long long *total, long long *used)
{
  return g_client->GetDriveSpace(total, used);
}

extern "C" PVR_ERROR GetEPGForChannel(unsigned int number, EPG_DATA &epg, time_t start, time_t end)
{
  return g_client->GetEPGForChannel(number, epg, start, end);
}

extern "C" PVR_ERROR GetEPGNowInfo(unsigned int number, CTVEPGInfoTag *result)
{
  return g_client->GetEPGNowInfo(number, result);
}

extern "C" PVR_ERROR GetEPGNextInfo(unsigned int number, CTVEPGInfoTag *result)
{
  return g_client->GetEPGNextInfo(number, result);
}

extern "C" int GetNumChannels()
{
  return g_client->GetNumChannels();
}

extern "C" PVR_ERROR GetChannelList(VECCHANNELS *channels, bool radio)
{
  return g_client->GetChannelList(channels, radio);
}

extern "C" PVR_ERROR GetChannelSettings(CTVChannelInfoTag *result)
{
  return g_client->GetChannelSettings(result);
}

extern "C" PVR_ERROR UpdateChannelSettings(const CTVChannelInfoTag &chaninfo)
{
  return g_client->UpdateChannelSettings(chaninfo);
}

extern "C" PVR_ERROR AddChannel(const CTVChannelInfoTag &info)
{
  return g_client->AddChannel(info);
}

extern "C" PVR_ERROR DeleteChannel(unsigned int number)
{
  return g_client->DeleteChannel(number);
}

extern "C" PVR_ERROR RenameChannel(unsigned int number, CStdString &newname)
{
  return g_client->RenameChannel(number, newname);
}

extern "C" PVR_ERROR MoveChannel(unsigned int number, unsigned int newnumber)
{
  return g_client->MoveChannel(number, newnumber);
}

extern "C" int GetNumRecordings(void)
{
  return g_client->GetNumRecordings();
}

extern "C" PVR_ERROR GetAllRecordings(VECRECORDINGS *results)
{
  return g_client->GetAllRecordings(results);
}

extern "C" PVR_ERROR DeleteRecording(const CTVRecordingInfoTag &recinfo)
{
  return g_client->DeleteRecording(recinfo);
}

extern "C" PVR_ERROR RenameRecording(const CTVRecordingInfoTag &recinfo, CStdString &newname)
{
  return g_client->RenameRecording(recinfo, newname);
}

extern "C" int GetNumTimers(void)
{
  return g_client->GetNumTimers();
}

extern "C" PVR_ERROR GetAllTimers(VECTVTIMERS *results)
{
  return g_client->GetAllTimers(results);
}

extern "C" PVR_ERROR AddTimer(const CTVTimerInfoTag &timerinfo)
{
  return g_client->AddTimer(timerinfo);
}

extern "C" PVR_ERROR DeleteTimer(const CTVTimerInfoTag &timerinfo, bool force)
{
  return g_client->DeleteTimer(timerinfo, force);
}

extern "C" PVR_ERROR RenameTimer(const CTVTimerInfoTag &timerinfo, CStdString &newname)
{
  return g_client->RenameTimer(timerinfo, newname);
}

extern "C" PVR_ERROR UpdateTimer(const CTVTimerInfoTag &timerinfo)
{
  return g_client->UpdateTimer(timerinfo);
}

extern "C" bool OpenLiveStream(unsigned int channel)
{
  return g_client->OpenLiveStream(channel);
}

extern "C" void CloseLiveStream()
{
  return g_client->CloseLiveStream();
}

extern "C" int ReadLiveStream(BYTE* buf, int buf_size)
{
  return g_client->ReadLiveStream(buf, buf_size);
}

extern "C" int GetCurrentClientChannel()
{
  return g_client->GetCurrentClientChannel();
}

extern "C" bool SwitchChannel(unsigned int channel)
{
  return g_client->SwitchChannel(channel);
}

extern "C" bool OpenRecordedStream(const CTVRecordingInfoTag &recinfo)
{
  return g_client->OpenRecordedStream(recinfo);
}

extern "C" void CloseRecordedStream(void)
{
  return g_client->CloseRecordedStream();
}

extern "C" int ReadRecordedStream(BYTE* buf, int buf_size)
{
  return g_client->ReadRecordedStream(buf, buf_size);
}

extern "C" __int64 SeekRecordedStream(__int64 pos, int whence)
{
  return g_client->SeekRecordedStream(pos, whence);
}

extern "C" __int64 LengthRecordedStream(void)
{
  return g_client->LengthRecordedStream();
}

