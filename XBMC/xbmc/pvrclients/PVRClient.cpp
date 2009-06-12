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

#include "stdafx.h"
#include <vector>

#include "PVRClient.h"
#include "pvrclients/PVRClientTypes.h"
#include "../utils/CharsetConverter.h"
#include "../utils/log.h"
#include "LocalizeStrings.h"

using namespace std;
using namespace ADDON;

CPVRClient::CPVRClient(long clientID, struct PVRClient* pClient, DllPVRClient* pDll,
                       const ADDON::CAddon& addon, IPVRClientCallback* pvrCB)
                              : IPVRClient(clientID, pvrCB)
                              , m_clientID(clientID)
                              , m_pClient(pClient)
                              , m_pDll(pDll)
                              , m_manager(pvrCB)
{

}

CPVRClient::~CPVRClient()
{
  // tell the plugin to disconnect and prepare for destruction
  Disconnect();
}

bool CPVRClient::Init()
{
  PVRCallbacks *callbacks = new PVRCallbacks;
  callbacks->userData     = this;
  callbacks->Event        = PVREventCallback;
  callbacks->Log          = PVRLogCallback;
  callbacks->CharConv     = PVRUnknownToUTF8;
  callbacks->LocStrings   = PVRLocStrings;

  m_pClient->Create(callbacks);
  return true;
}

long CPVRClient::GetID()
{
  return m_clientID;
}

PVR_ERROR CPVRClient::GetProperties(PVR_SERVERPROPS *props)
{
  // get info from client
  return m_pClient->GetProperties(props);
}

PVR_ERROR CPVRClient::SetUserSetting(const char *settingName, const void *settingValue)
{
  return m_pClient->SetUserSetting(settingName, settingValue);
}

PVR_ERROR CPVRClient::Connect()
{
  return m_pClient->Connect();
}

void CPVRClient::Disconnect()
{
  m_pClient->Disconnect();
}

bool CPVRClient::IsUp()
{
  return m_pClient->IsUp();
}

const std::string CPVRClient::GetBackendName()
{
  return m_pClient->GetBackendName();
}

const std::string CPVRClient::GetBackendVersion()
{
  return m_pClient->GetBackendVersion();
}

PVR_ERROR CPVRClient::GetDriveSpace(long long *total, long long *used)
{
  return m_pClient->GetDriveSpace(total, used);
}

PVR_ERROR CPVRClient::GetEPGForChannel(unsigned int number, EPG_DATA &epg, time_t start, time_t end)
{
  return m_pClient->GetEPGForChannel(number, epg, start, end);
}

PVR_ERROR CPVRClient::GetEPGNowInfo(unsigned int number, CTVEPGInfoTag *result)
{
  return m_pClient->GetEPGNowInfo(number, result);
}

PVR_ERROR CPVRClient::GetEPGNextInfo(unsigned int number, CTVEPGInfoTag *result)
{
  return m_pClient->GetEPGNextInfo(number, result);
}

int CPVRClient::GetNumChannels()
{
  return m_pClient->GetNumChannels();
}

PVR_ERROR CPVRClient::GetChannelList(VECCHANNELS &channels, bool radio)
{
  return m_pClient->GetChannelList(&channels, radio);
}

PVR_ERROR CPVRClient::GetChannelSettings(CTVChannelInfoTag *result)
{
  return m_pClient->GetChannelSettings(result);
}

PVR_ERROR CPVRClient::UpdateChannelSettings(const CTVChannelInfoTag &chaninfo)
{
  return m_pClient->UpdateChannelSettings(chaninfo);
}

PVR_ERROR CPVRClient::AddChannel(const CTVChannelInfoTag &info)
{
  return m_pClient->AddChannel(info);
}

PVR_ERROR CPVRClient::DeleteChannel(unsigned int number)
{
  return m_pClient->DeleteChannel(number);
}

PVR_ERROR CPVRClient::RenameChannel(unsigned int number, CStdString &newname)
{
  return m_pClient->RenameChannel(number, newname);
}

PVR_ERROR CPVRClient::MoveChannel(unsigned int number, unsigned int newnumber)
{
  return m_pClient->MoveChannel(number, newnumber);
}

int CPVRClient::GetNumRecordings(void)
{
  return m_pClient->GetNumRecordings();
}

PVR_ERROR CPVRClient::GetAllRecordings(VECRECORDINGS *results)
{
  return m_pClient->GetAllRecordings(results);
}

PVR_ERROR CPVRClient::DeleteRecording(const CTVRecordingInfoTag &recinfo)
{
  return m_pClient->DeleteRecording(recinfo);
}

PVR_ERROR CPVRClient::RenameRecording(const CTVRecordingInfoTag &recinfo, CStdString &newname)
{
  return m_pClient->RenameRecording(recinfo, newname);
}

int CPVRClient::GetNumTimers(void)
{
  return m_pClient->GetNumTimers();
}

PVR_ERROR CPVRClient::GetAllTimers(VECTVTIMERS *results)
{
  return m_pClient->GetAllTimers(results);
}

PVR_ERROR CPVRClient::AddTimer(const CTVTimerInfoTag &timerinfo)
{
  return m_pClient->AddTimer(timerinfo);
}

PVR_ERROR CPVRClient::DeleteTimer(const CTVTimerInfoTag &timerinfo, bool force)
{
  return m_pClient->DeleteTimer(timerinfo, force);
}

PVR_ERROR CPVRClient::RenameTimer(const CTVTimerInfoTag &timerinfo, CStdString &newname)
{
  return m_pClient->RenameTimer(timerinfo, newname);
}

PVR_ERROR CPVRClient::UpdateTimer(const CTVTimerInfoTag &timerinfo)
{
  return m_pClient->UpdateTimer(timerinfo);
}

bool CPVRClient::OpenLiveStream(unsigned int channel)
{
  return m_pClient->OpenLiveStream(channel);
}

void CPVRClient::CloseLiveStream()
{
  return m_pClient->CloseLiveStream();
}

int CPVRClient::ReadLiveStream(BYTE* buf, int buf_size)
{
  return m_pClient->ReadLiveStream(buf, buf_size);
}

int CPVRClient::GetCurrentClientChannel()
{
  return m_pClient->GetCurrentClientChannel();
}

bool CPVRClient::SwitchChannel(unsigned int channel)
{
  return m_pClient->SwitchChannel(channel);
}

bool CPVRClient::OpenRecordedStream(const CTVRecordingInfoTag &recinfo)
{
  return m_pClient->OpenRecordedStream(recinfo);
}

void CPVRClient::CloseRecordedStream(void)
{
  return m_pClient->CloseRecordedStream();
}

int CPVRClient::ReadRecordedStream(BYTE* buf, int buf_size)
{
  return m_pClient->ReadRecordedStream(buf, buf_size);
}

__int64 CPVRClient::SeekRecordedStream(__int64 pos, int whence)
{
  return m_pClient->SeekRecordedStream(pos, whence);
}

__int64 CPVRClient::LengthRecordedStream(void)
{
  return m_pClient->LengthRecordedStream();
}

/**
* XBMC callbacks
*/
void CPVRClient::PVREventCallback(void *userData, const PVR_EVENT pvrevent, const char *msg)
{
  CPVRClient* client=(CPVRClient*) userData;
  if (!client)
    return;

  client->m_manager->OnClientMessage(client->m_clientID, pvrevent, msg);
}

void CPVRClient::PVRLogCallback(void *userData, const PVR_LOG loglevel, const char *format, ... )
{
  CPVRClient* client=(CPVRClient*) userData;
  if (!client)
    return;

  CStdString message;
  message.reserve(16384);
  message.Format("PVR: %s:", client->m_hostName);

  va_list va;
  va_start(va, format);
  message.FormatV(format, va);
  va_end(va);

  int xbmclog;
  switch (loglevel)
  {
    case LOG_ERROR:
      xbmclog = LOGERROR;
      break;
    case LOG_INFO:
      xbmclog = LOGINFO;
      break;
    case LOG_DEBUG:
    default:
      xbmclog = LOGDEBUG;
      break;
  }

  CLog::Log(xbmclog, message);
}

void CPVRClient::PVRUnknownToUTF8(CStdStringA &sourceDest)
{
  g_charsetConverter.unknownToUTF8(sourceDest);
}

const char* CPVRClient::PVRLocStrings(DWORD dwCode)
{
  return g_localizeStrings.Get(dwCode).c_str();
}
