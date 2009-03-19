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
#include "../../pvrclients/PVRClientTypes.h"
#include "../utils/log.h"

CPVRClient::CPVRClient(long clientID, struct PVRClient* pClient, DllPVRClient* pDll,
                       const CStdString& strPVRClientName, IPVRClientCallback* cb)
                              : IPVRClient(clientID, cb)
                              , m_clientID(clientID)
                              , m_pClient(pClient)
                              , m_pDll(pDll)
                              , m_clientName(strPVRClientName)
                              , m_manager(cb)
{}

CPVRClient::~CPVRClient()
{
  // tell the plugin to disconnect and prepare for destruction
  Disconnect();
  /*DeInit();*/
}

bool CPVRClient::Init()
{
  PVRCallbacks *callbacks = new PVRCallbacks;
  callbacks->userData=this;
  callbacks->Event=PVREventCallback;
  callbacks->Log=PVRLogCallback;

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
  return GetDriveSpace(total, used);
}

int CPVRClient::GetNumBouquets()
{
  return 0;
}

PVR_ERROR CPVRClient::GetBouquetInfo(const unsigned int number, PVR_BOUQUET& info)
{
  return PVR_ERROR_NO_ERROR;
}

int CPVRClient::GetNumChannels()
{
  return m_pClient->GetNumChannels();
}

PVR_ERROR CPVRClient::GetChannelList(VECCHANNELS &channels)
{
  PVR_ERROR ret;
  PVR_CHANLIST clientChans;
  ret = m_pClient->GetChannelList(&clientChans);
  if (ret != PVR_ERROR_NO_ERROR)
    return ret;

  ConvertChannels(clientChans, channels);
  return PVR_ERROR_NO_ERROR;
}

void CPVRClient::ConvertChannels(PVR_CHANLIST in, VECCHANNELS &out)
{
  out.clear();

  for (int i = 0; i < in.length; i++)
  {

  }

}

PVR_ERROR CPVRClient::GetEPGForChannel(const unsigned int number, PVR_PROGLIST *epg, time_t start, time_t end)
{
  return m_pClient->GetEPGForChannel(number, epg, start, end);
}

PVR_ERROR CPVRClient::GetEPGNowInfo(const unsigned int number, PVR_PROGINFO *result)
{
  return m_pClient->GetEPGNowInfo(number, result);
}

PVR_ERROR CPVRClient::GetEPGNextInfo(const unsigned int number, PVR_PROGINFO *result)
{
  return m_pClient->GetEPGNextInfo(number, result);
}

PVR_ERROR CPVRClient::GetEPGDataEnd(time_t end)
{
  return PVR_ERROR_NO_ERROR;
}

void CPVRClient::GetSettings(std::vector<DllSetting> **vecSettings)
{
  /*if (vecSettings) *vecSettings = NULL;
  if (m_pClient->GetSettings)
    m_pClient->GetSettings(vecSettings);*/
}

void CPVRClient::UpdateSetting(int num)
{
  /*if (m_pClient->UpdateSetting)
    m_pClient->UpdateSetting(num);*/
}


// Callbacks /////////////////////////////////////////////////////////////////
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
  message.Format("PVR: %s/%s:", client->m_clientName, client->m_hostName);

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
