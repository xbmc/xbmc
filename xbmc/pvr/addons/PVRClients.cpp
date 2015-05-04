/*
 *      Copyright (C) 2012-2013 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "PVRClients.h"

#include "Application.h"
#include "ApplicationMessenger.h"
#include "GUIUserMessages.h"
#include "dialogs/GUIDialogExtendedProgressBar.h"
#include "dialogs/GUIDialogOK.h"
#include "dialogs/GUIDialogSelect.h"
#include "pvr/PVRManager.h"
#include "guilib/GUIWindowManager.h"
#include "settings/Settings.h"
#include "pvr/channels/PVRChannelGroups.h"
#include "pvr/channels/PVRChannelGroupInternal.h"
#include "pvr/recordings/PVRRecordings.h"
#include "pvr/timers/PVRTimers.h"

#include <assert.h>

using namespace ADDON;
using namespace PVR;
using namespace EPG;

/** number of iterations when scanning for add-ons. don't use a timer because the user may block in the dialog */
#define PVR_CLIENT_AVAHI_SCAN_ITERATIONS   (20)
/** sleep time in milliseconds when no auto-configured add-ons were found */
#define PVR_CLIENT_AVAHI_SLEEP_TIME_MS     (250)

CPVRClients::CPVRClients(void) :
    CThread("PVRClient"),
    m_bChannelScanRunning(false),
    m_bIsSwitchingChannels(false),
    m_playingClientId(-EINVAL),
    m_bIsPlayingLiveTV(false),
    m_bIsPlayingRecording(false),
    m_scanStart(0),
    m_bNoAddonWarningDisplayed(false),
    m_bRestartManagerOnAddonDisabled(false)
{
}

CPVRClients::~CPVRClients(void)
{
  Unload();
}

bool CPVRClients::IsInUse(const std::string& strAddonId) const
{
  CSingleLock lock(m_critSection);

  for (PVR_CLIENTMAP_CITR itr = m_clientMap.begin(); itr != m_clientMap.end(); itr++)
    if (itr->second->Enabled() && itr->second->ID() == strAddonId)
      return true;
  return false;
}

void CPVRClients::Start(void)
{
  Stop();

  Create();
  SetPriority(-1);
}

void CPVRClients::Stop(void)
{
  StopThread();
}

bool CPVRClients::IsConnectedClient(int iClientId) const
{
  PVR_CLIENT client;
  return GetConnectedClient(iClientId, client);
}

bool CPVRClients::IsConnectedClient(const AddonPtr addon)
{
  CSingleLock lock(m_critSection);

  for (PVR_CLIENTMAP_CITR itr = m_clientMap.begin(); itr != m_clientMap.end(); itr++)
    if (itr->second->ID() == addon->ID())
      return itr->second->ReadyToUse();
  return false;
}

int CPVRClients::GetClientId(const AddonPtr client) const
{
  CSingleLock lock(m_critSection);

  for (PVR_CLIENTMAP_CITR itr = m_clientMap.begin(); itr != m_clientMap.end(); itr++)
    if (itr->second->ID() == client->ID())
      return itr->first;

  return -1;
}

int CPVRClients::GetClientId(const std::string& strId) const
{
  CSingleLock lock(m_critSection);
  std::map<std::string, int>::const_iterator it = m_addonNameIds.find(strId);
  return it != m_addonNameIds.end() ?
      it->second :
      -1;
}

bool CPVRClients::GetClient(int iClientId, PVR_CLIENT &addon) const
{
  bool bReturn(false);
  if (iClientId <= PVR_INVALID_CLIENT_ID)
    return bReturn;

  CSingleLock lock(m_critSection);

  PVR_CLIENTMAP_CITR itr = m_clientMap.find(iClientId);
  if (itr != m_clientMap.end())
  {
    addon = itr->second;
    bReturn = true;
  }

  return bReturn;
}

bool CPVRClients::GetConnectedClient(int iClientId, PVR_CLIENT &addon) const
{
  if (GetClient(iClientId, addon))
    return addon->ReadyToUse();
  return false;
}

bool CPVRClients::RequestRestart(AddonPtr addon, bool bDataChanged)
{
  return StopClient(addon, true);
}

bool CPVRClients::RequestRemoval(AddonPtr addon)
{
  return StopClient(addon, false);
}

void CPVRClients::Unload(void)
{
  Stop();

  CSingleLock lock(m_critSection);

  /* destroy all clients */
  for (PVR_CLIENTMAP_ITR itr = m_clientMap.begin(); itr != m_clientMap.end(); itr++)
    itr->second->Destroy();

  /* reset class properties */
  m_bChannelScanRunning  = false;
  m_bIsPlayingLiveTV     = false;
  m_bIsPlayingRecording  = false;
  m_strPlayingClientName = "";

  m_clientMap.clear();
}

int CPVRClients::GetFirstConnectedClientID(void)
{
  CSingleLock lock(m_critSection);

  for (PVR_CLIENTMAP_CITR itr = m_clientMap.begin(); itr != m_clientMap.end(); itr++)
    if (itr->second->ReadyToUse())
      return itr->second->GetID();

  return -1;
}

int CPVRClients::EnabledClientAmount(void) const
{
  int iReturn(0);
  CSingleLock lock(m_critSection);

  for (PVR_CLIENTMAP_CITR itr = m_clientMap.begin(); itr != m_clientMap.end(); itr++)
    if (!CAddonMgr::Get().IsAddonDisabled(itr->second->ID()))
      ++iReturn;

  return iReturn;
}

bool CPVRClients::HasEnabledClients(void) const
{
  for (PVR_CLIENTMAP_CITR itr = m_clientMap.begin(); itr != m_clientMap.end(); itr++)
    if (!CAddonMgr::Get().IsAddonDisabled(itr->second->ID()))
      return true;
  return false;
}

bool CPVRClients::StopClient(AddonPtr client, bool bRestart)
{
  CSingleLock lock(m_critSection);
  int iId = GetClientId(client);
  PVR_CLIENT mappedClient;
  if (GetClient(iId, mappedClient))
  {
    if (bRestart)
      mappedClient->ReCreate();
    else
      mappedClient->Destroy();

    return true;
  }

  return false;
}

int CPVRClients::ConnectedClientAmount(void) const
{
  int iReturn(0);
  CSingleLock lock(m_critSection);

  for (PVR_CLIENTMAP_CITR itr = m_clientMap.begin(); itr != m_clientMap.end(); itr++)
    if (itr->second->ReadyToUse())
      ++iReturn;

  return iReturn;
}

bool CPVRClients::HasConnectedClients(void) const
{
  CSingleLock lock(m_critSection);

  for (PVR_CLIENTMAP_CITR itr = m_clientMap.begin(); itr != m_clientMap.end(); itr++)
    if (itr->second->ReadyToUse())
      return true;

  return false;
}

bool CPVRClients::GetClientName(int iClientId, std::string &strName) const
{
  bool bReturn(false);
  PVR_CLIENT client;
  if ((bReturn = GetConnectedClient(iClientId, client)) == true)
    strName = client->GetFriendlyName();

  return bReturn;
}

std::vector<SBackend> CPVRClients::GetBackendProperties() const
{
  std::vector<SBackend> backendProperties;
  CSingleLock lock(m_critSection);

  for (const auto &entry : m_clientMap)
  {
    const auto &client = entry.second;
    
    if (!client->ReadyToUse())
      continue;

    SBackend properties;

    if (client->GetDriveSpace(&properties.diskTotal, &properties.diskUsed) == PVR_ERROR_NO_ERROR)
    {
      properties.diskTotal *= 1024;  
      properties.diskUsed *= 1024;
    }

    properties.numChannels = client->GetChannelsAmount();
    properties.numTimers = client->GetTimersAmount();
    properties.numRecordings = client->GetRecordingsAmount(false);
    properties.numDeletedRecordings = client->GetRecordingsAmount(true);
    properties.name = client->GetBackendName();
    properties.version = client->GetBackendVersion();
    properties.host = client->GetConnectionString();

    backendProperties.push_back(properties);
  }

  return backendProperties;
}

std::string CPVRClients::GetClientAddonId(int iClientId) const
{
  PVR_CLIENT client;
  return GetClient(iClientId, client) ?
      client->ID() :
      "";
}

int CPVRClients::GetConnectedClients(PVR_CLIENTMAP &clients) const
{
  int iReturn(0);
  CSingleLock lock(m_critSection);

  for (PVR_CLIENTMAP_CITR itr = m_clientMap.begin(); itr != m_clientMap.end(); itr++)
  {
    if (itr->second->ReadyToUse())
    {
      clients.insert(std::make_pair(itr->second->GetID(), itr->second));
      ++iReturn;
    }
  }

  return iReturn;
}

int CPVRClients::GetPlayingClientID(void) const
{
  CSingleLock lock(m_critSection);

  if (m_bIsPlayingLiveTV || m_bIsPlayingRecording)
    return m_playingClientId;
  return -EINVAL;
}

const std::string CPVRClients::GetPlayingClientName(void) const
{
  CSingleLock lock(m_critSection);
  return m_strPlayingClientName;
}

std::string CPVRClients::GetStreamURL(const CPVRChannelPtr &channel)
{
  assert(channel.get());

  std::string strReturn;
  PVR_CLIENT client;
  if (GetConnectedClient(channel->ClientID(), client))
    strReturn = client->GetLiveStreamURL(channel);
  else
    CLog::Log(LOGERROR, "PVR - %s - cannot find client %d",__FUNCTION__, channel->ClientID());

  return strReturn;
}

bool CPVRClients::SwitchChannel(const CPVRChannelPtr &channel)
{
  assert(channel.get());

  {
    CSingleLock lock(m_critSection);
    if (m_bIsSwitchingChannels)
    {
      CLog::Log(LOGDEBUG, "PVRClients - %s - can't switch to channel '%s'. waiting for the previous switch to complete", __FUNCTION__, channel->ChannelName().c_str());
      return false;
    }
    m_bIsSwitchingChannels = true;
  }

  bool bSwitchSuccessful(false);
  CPVRChannelPtr currentChannel(GetPlayingChannel());
  if (// no channel is currently playing
      !currentChannel ||
      // different backend
      currentChannel->ClientID() != channel->ClientID() ||
      // stream URL should always be opened as a new file
      !channel->StreamURL().empty() || !currentChannel->StreamURL().empty())
  {
    if (channel->StreamURL().empty())
    {
      CloseStream();
      bSwitchSuccessful = OpenStream(channel, true);
    }
    else
    {
      CFileItem m_currentFile(channel);
      CApplicationMessenger::Get().PlayFile(m_currentFile, false);
      bSwitchSuccessful = true;
    }
  }
  // same channel
  else if (currentChannel && currentChannel == channel)
  {
    bSwitchSuccessful = true;
  }
  else
  {
    PVR_CLIENT client;
    if (GetConnectedClient(channel->ClientID(), client))
      bSwitchSuccessful = client->SwitchChannel(channel);
  }

  {
    CSingleLock lock(m_critSection);
    m_bIsSwitchingChannels = false;
  }

  if (!bSwitchSuccessful)
    CLog::Log(LOGERROR, "PVR - %s - cannot switch to channel '%s' on client '%d'",__FUNCTION__, channel->ChannelName().c_str(), channel->ClientID());

  return bSwitchSuccessful;
}

CPVRChannelPtr CPVRClients::GetPlayingChannel() const
{
  PVR_CLIENT client;
  if (GetPlayingClient(client))
    return client->GetPlayingChannel();

  return CPVRChannelPtr();
}

CPVRRecordingPtr CPVRClients::GetPlayingRecording(void) const
{
  PVR_CLIENT client;
  return GetPlayingClient(client) ? client->GetPlayingRecording() : CPVRRecordingPtr();
}

bool CPVRClients::HasTimerSupport(int iClientId)
{
  PVR_CLIENT client;
  if (GetConnectedClient(iClientId, client))
    return client->SupportsTimers();

  return false;
}

PVR_ERROR CPVRClients::GetTimers(CPVRTimers *timers)
{
  PVR_ERROR error(PVR_ERROR_NO_ERROR);
  PVR_CLIENTMAP clients;
  GetConnectedClients(clients);

  /* get the timer list from each client */
  for (PVR_CLIENTMAP_CITR itrClients = clients.begin(); itrClients != clients.end(); itrClients++)
  {
    PVR_ERROR currentError = (*itrClients).second->GetTimers(timers);
    if (currentError != PVR_ERROR_NOT_IMPLEMENTED &&
        currentError != PVR_ERROR_NO_ERROR)
    {
      CLog::Log(LOGERROR, "PVR - %s - cannot get timers from client '%d': %s",__FUNCTION__, (*itrClients).first, CPVRClient::ToString(currentError));
      error = currentError;
    }
  }

  return error;
}

PVR_ERROR CPVRClients::AddTimer(const CPVRTimerInfoTag &timer)
{
  PVR_ERROR error(PVR_ERROR_UNKNOWN);

  PVR_CLIENT client;
  if (GetConnectedClient(timer.m_iClientId, client))
    error = client->AddTimer(timer);

  if (error != PVR_ERROR_NO_ERROR)
    CLog::Log(LOGERROR, "PVR - %s - cannot add timer to client '%d': %s",__FUNCTION__, timer.m_iClientId, CPVRClient::ToString(error));

  return error;
}

PVR_ERROR CPVRClients::UpdateTimer(const CPVRTimerInfoTag &timer)
{
  PVR_ERROR error(PVR_ERROR_UNKNOWN);

  PVR_CLIENT client;
  if (GetConnectedClient(timer.m_iClientId, client))
    error = client->UpdateTimer(timer);

  if (error != PVR_ERROR_NO_ERROR)
    CLog::Log(LOGERROR, "PVR - %s - cannot update timer on client '%d': %s",__FUNCTION__, timer.m_iClientId, CPVRClient::ToString(error));

  return error;
}

PVR_ERROR CPVRClients::DeleteTimer(const CPVRTimerInfoTag &timer, bool bForce, bool bDeleteSchedule)
{
  PVR_ERROR error(PVR_ERROR_UNKNOWN);
  PVR_CLIENT client;

  if (GetConnectedClient(timer.m_iClientId, client))
    error = client->DeleteTimer(timer, bForce, bDeleteSchedule);

  return error;
}

PVR_ERROR CPVRClients::RenameTimer(const CPVRTimerInfoTag &timer, const std::string &strNewName)
{
  PVR_ERROR error(PVR_ERROR_UNKNOWN);

  PVR_CLIENT client;
  if (GetConnectedClient(timer.m_iClientId, client))
    error = client->RenameTimer(timer, strNewName);

  if (error != PVR_ERROR_NO_ERROR)
    CLog::Log(LOGERROR, "PVR - %s - cannot rename timer on client '%d': %s",__FUNCTION__, timer.m_iClientId, CPVRClient::ToString(error));

  return error;
}

PVR_ERROR CPVRClients::GetTimerTypes(CPVRTimerTypes& results) const
{
  PVR_ERROR error(PVR_ERROR_NO_ERROR);

  PVR_CLIENTMAP clients;
  GetConnectedClients(clients);

  for (const auto &clientEntry : clients)
  {
    CPVRTimerTypes types;
    PVR_ERROR currentError = clientEntry.second->GetTimerTypes(types);
    if (currentError != PVR_ERROR_NOT_IMPLEMENTED &&
        currentError != PVR_ERROR_NO_ERROR)
    {
      CLog::Log(LOGERROR, "PVR - %s - cannot get timer types from client '%d': %s",__FUNCTION__, clientEntry.first, CPVRClient::ToString(currentError));
      error = currentError;
    }
    else
    {
      for (const auto &typesEntry : types)
        results.push_back(typesEntry);
    }
  }

  return error;
}

PVR_ERROR CPVRClients::GetTimerTypes(CPVRTimerTypes& results, int iClientId) const
{
  PVR_ERROR error(PVR_ERROR_UNKNOWN);

  PVR_CLIENT client;
  if (GetConnectedClient(iClientId, client))
    error = client->GetTimerTypes(results);

  if (error != PVR_ERROR_NO_ERROR)
    CLog::Log(LOGERROR, "PVR - %s - cannot get timer types from client '%d': %s",__FUNCTION__, iClientId, CPVRClient::ToString(error));

  return error;
}

PVR_ERROR CPVRClients::GetRecordings(CPVRRecordings *recordings, bool deleted)
{
  PVR_ERROR error(PVR_ERROR_NO_ERROR);
  PVR_CLIENTMAP clients;
  GetConnectedClients(clients);

  for (PVR_CLIENTMAP_CITR itrClients = clients.begin(); itrClients != clients.end(); itrClients++)
  {
    PVR_ERROR currentError = (*itrClients).second->GetRecordings(recordings, deleted);
    if (currentError != PVR_ERROR_NOT_IMPLEMENTED &&
        currentError != PVR_ERROR_NO_ERROR)
    {
      CLog::Log(LOGERROR, "PVR - %s - cannot get recordings from client '%d': %s",__FUNCTION__, (*itrClients).first, CPVRClient::ToString(currentError));
      error = currentError;
    }
  }

  return error;
}

PVR_ERROR CPVRClients::RenameRecording(const CPVRRecording &recording)
{
  PVR_ERROR error(PVR_ERROR_UNKNOWN);

  PVR_CLIENT client;
  if (GetConnectedClient(recording.m_iClientId, client))
    error = client->RenameRecording(recording);

  if (error != PVR_ERROR_NO_ERROR)
    CLog::Log(LOGERROR, "PVR - %s - cannot rename recording on client '%d': %s",__FUNCTION__, recording.m_iClientId, CPVRClient::ToString(error));

  return error;
}

PVR_ERROR CPVRClients::DeleteRecording(const CPVRRecording &recording)
{
  PVR_ERROR error(PVR_ERROR_UNKNOWN);

  PVR_CLIENT client;
  if (GetConnectedClient(recording.m_iClientId, client))
    error = client->DeleteRecording(recording);

  if (error != PVR_ERROR_NO_ERROR)
    CLog::Log(LOGERROR, "PVR - %s - cannot delete recording from client '%d': %s",__FUNCTION__, recording.m_iClientId, CPVRClient::ToString(error));

  return error;
}

PVR_ERROR CPVRClients::UndeleteRecording(const CPVRRecording &recording)
{
  PVR_ERROR error(PVR_ERROR_UNKNOWN);

  if (!recording.IsDeleted())
    return error;

  PVR_CLIENT client;
  if (GetConnectedClient(recording.m_iClientId, client))
    error = client->UndeleteRecording(recording);

  if (error != PVR_ERROR_NO_ERROR)
    CLog::Log(LOGERROR, "PVR - %s - cannot undelete recording from client '%d': %s",__FUNCTION__, recording.m_iClientId, CPVRClient::ToString(error));

  return error;
}

PVR_ERROR CPVRClients::DeleteAllRecordingsFromTrash()
{
  PVR_ERROR error(PVR_ERROR_NO_ERROR);
  PVR_CLIENTMAP clients;
  GetConnectedClients(clients);

  std::vector<PVR_CLIENT> suppClients;
  for (PVR_CLIENTMAP_CITR itrClients = clients.begin(); itrClients != clients.end(); ++itrClients)
  {
    if (itrClients->second->SupportsRecordingsUndelete() && itrClients->second->GetRecordingsAmount(true) > 0)
      suppClients.push_back(itrClients->second);
  }

  int selection = 0;
  if (suppClients.size() > 1)
  {
    // have user select client
    CGUIDialogSelect* pDialog = (CGUIDialogSelect*)g_windowManager.GetWindow(WINDOW_DIALOG_SELECT);
    pDialog->Reset();
    pDialog->SetHeading(19292);                 /* Delete all permanently */
    pDialog->Add(g_localizeStrings.Get(24032)); /* All Add-ons */

    PVR_CLIENTMAP_CITR itrClients;
    for (itrClients = clients.begin(); itrClients != clients.end(); ++itrClients)
    {
      if (itrClients->second->SupportsRecordingsUndelete() && itrClients->second->GetRecordingsAmount(true) > 0)
        pDialog->Add(itrClients->second->GetBackendName());
    }
    pDialog->DoModal();
    selection = pDialog->GetSelectedLabel();
  }

  if (selection == 0)
  {
    typedef std::vector<PVR_CLIENT>::const_iterator suppClientsCITR;
    for (suppClientsCITR itrSuppClients = suppClients.begin(); itrSuppClients != suppClients.end(); ++itrSuppClients)
    {
      PVR_ERROR currentError = (*itrSuppClients)->DeleteAllRecordingsFromTrash();
      if (currentError != PVR_ERROR_NO_ERROR)
      {
        CLog::Log(LOGERROR, "PVR - %s - cannot delete all recordings from client '%d': %s",__FUNCTION__, (*itrSuppClients)->GetID(), CPVRClient::ToString(currentError));
        error = currentError;
      }
    }
  }
  else if (selection >= 1 && selection <= (int)suppClients.size())
  {
    PVR_ERROR currentError = suppClients[selection-1]->DeleteAllRecordingsFromTrash();
    if (currentError != PVR_ERROR_NO_ERROR)
    {
      CLog::Log(LOGERROR, "PVR - %s - cannot delete all recordings from client '%d': %s",__FUNCTION__, suppClients[selection-1]->GetID(), CPVRClient::ToString(currentError));
      error = currentError;
    }
  }

  return error;
}

bool CPVRClients::SetRecordingLastPlayedPosition(const CPVRRecording &recording, int lastplayedposition, PVR_ERROR *error)
{
  *error = PVR_ERROR_UNKNOWN;
  PVR_CLIENT client;
  if (GetConnectedClient(recording.m_iClientId, client) && client->SupportsRecordings())
    *error = client->SetRecordingLastPlayedPosition(recording, lastplayedposition);
  else
    CLog::Log(LOGERROR, "PVR - %s - client %d does not support recordings",__FUNCTION__, recording.m_iClientId);

  return *error == PVR_ERROR_NO_ERROR;
}

int CPVRClients::GetRecordingLastPlayedPosition(const CPVRRecording &recording)
{
  int rc = 0;

  PVR_CLIENT client;
  if (GetConnectedClient(recording.m_iClientId, client) && client->SupportsRecordings())
    rc = client->GetRecordingLastPlayedPosition(recording);
  else
    CLog::Log(LOGERROR, "PVR - %s - client %d does not support recordings",__FUNCTION__, recording.m_iClientId);

  return rc;
}

bool CPVRClients::SetRecordingPlayCount(const CPVRRecording &recording, int count, PVR_ERROR *error)
{
  *error = PVR_ERROR_UNKNOWN;
  PVR_CLIENT client;
  if (GetConnectedClient(recording.m_iClientId, client) && client->SupportsRecordingPlayCount())
    *error = client->SetRecordingPlayCount(recording, count);
  else
    CLog::Log(LOGERROR, "PVR - %s - client %d does not support setting recording's play count",__FUNCTION__, recording.m_iClientId);

  return *error == PVR_ERROR_NO_ERROR;
}

std::vector<PVR_EDL_ENTRY> CPVRClients::GetRecordingEdl(const CPVRRecording &recording)
{
  PVR_CLIENT client;
  if (GetConnectedClient(recording.m_iClientId, client) && client->SupportsRecordingEdl())
    return client->GetRecordingEdl(recording);
  else
    CLog::Log(LOGERROR, "PVR - %s - client %d does not support getting Edl", __FUNCTION__, recording.m_iClientId);

  return std::vector<PVR_EDL_ENTRY>();
}

bool CPVRClients::IsRecordingOnPlayingChannel(void) const
{
  CPVRChannelPtr currentChannel(GetPlayingChannel());
  return currentChannel && currentChannel->IsRecording();
}

bool CPVRClients::CanRecordInstantly(void)
{
  CPVRChannelPtr currentChannel(GetPlayingChannel());
  return currentChannel && currentChannel->CanRecord();
}

bool CPVRClients::CanPauseStream(void) const
{
  PVR_CLIENT client;

  if (GetPlayingClient(client))
  {
    return m_bIsPlayingRecording || client->CanPauseStream();
  }

  return false;
}

bool CPVRClients::CanSeekStream(void) const
{
  PVR_CLIENT client;

  if (GetPlayingClient(client))
  {
    return m_bIsPlayingRecording || client->CanSeekStream();
  }

  return false;
}

PVR_ERROR CPVRClients::GetEPGForChannel(const CPVRChannelPtr &channel, CEpg *epg, time_t start, time_t end)
{
  assert(channel.get());

  PVR_ERROR error(PVR_ERROR_UNKNOWN);
  PVR_CLIENT client;
  if (GetConnectedClient(channel->ClientID(), client))
    error = client->GetEPGForChannel(channel, epg, start, end);

  if (error != PVR_ERROR_NO_ERROR)
    CLog::Log(LOGERROR, "PVR - %s - cannot get EPG for channel '%s' from client '%d': %s",__FUNCTION__, channel->ChannelName().c_str(), channel->ClientID(), CPVRClient::ToString(error));
  return error;
}

PVR_ERROR CPVRClients::GetChannels(CPVRChannelGroupInternal *group)
{
  PVR_ERROR error(PVR_ERROR_NO_ERROR);
  PVR_CLIENTMAP clients;
  GetConnectedClients(clients);

  /* get the channel list from each client */
  for (PVR_CLIENTMAP_CITR itrClients = clients.begin(); itrClients != clients.end(); itrClients++)
  {
    PVR_ERROR currentError = (*itrClients).second->GetChannels(*group, group->IsRadio());
    if (currentError != PVR_ERROR_NOT_IMPLEMENTED &&
        currentError != PVR_ERROR_NO_ERROR)
    {
      error = currentError;
      CLog::Log(LOGERROR, "PVR - %s - cannot get channels from client '%d': %s",__FUNCTION__, (*itrClients).first, CPVRClient::ToString(error));
    }
  }

  return error;
}

PVR_ERROR CPVRClients::GetChannelGroups(CPVRChannelGroups *groups)
{
  PVR_ERROR error(PVR_ERROR_NO_ERROR);
  PVR_CLIENTMAP clients;
  GetConnectedClients(clients);

  for (PVR_CLIENTMAP_CITR itrClients = clients.begin(); itrClients != clients.end(); itrClients++)
  {
    PVR_ERROR currentError = (*itrClients).second->GetChannelGroups(groups);
    if (currentError != PVR_ERROR_NOT_IMPLEMENTED &&
        currentError != PVR_ERROR_NO_ERROR)
    {
      error = currentError;
      CLog::Log(LOGERROR, "PVR - %s - cannot get groups from client '%d': %s",__FUNCTION__, (*itrClients).first, CPVRClient::ToString(error));
    }
  }

  return error;
}

PVR_ERROR CPVRClients::GetChannelGroupMembers(CPVRChannelGroup *group)
{
  PVR_ERROR error(PVR_ERROR_NO_ERROR);
  PVR_CLIENTMAP clients;
  GetConnectedClients(clients);

  /* get the member list from each client */
  for (PVR_CLIENTMAP_CITR itrClients = clients.begin(); itrClients != clients.end(); itrClients++)
  {
    PVR_ERROR currentError = (*itrClients).second->GetChannelGroupMembers(group);
    if (currentError != PVR_ERROR_NOT_IMPLEMENTED &&
        currentError != PVR_ERROR_NO_ERROR)
    {
      error = currentError;
      CLog::Log(LOGERROR, "PVR - %s - cannot get group members from client '%d': %s",__FUNCTION__, (*itrClients).first, CPVRClient::ToString(error));
    }
  }

  return error;
}

bool CPVRClients::HasMenuHooks(int iClientID, PVR_MENUHOOK_CAT cat)
{
  if (iClientID < 0)
    iClientID = GetPlayingClientID();

  PVR_CLIENT client;
  return (GetConnectedClient(iClientID, client) &&
      client->HaveMenuHooks(cat));
}

bool CPVRClients::GetMenuHooks(int iClientID, PVR_MENUHOOK_CAT cat, PVR_MENUHOOKS *hooks)
{
  bool bReturn(false);

  if (iClientID < 0)
    iClientID = GetPlayingClientID();

  PVR_CLIENT client;
  if (GetConnectedClient(iClientID, client) && client->HaveMenuHooks(cat))
  {
    *hooks = *(client->GetMenuHooks());
    bReturn = true;
  }

  return bReturn;
}

void CPVRClients::ProcessMenuHooks(int iClientID, PVR_MENUHOOK_CAT cat, const CFileItem *item)
{
  // get client id
  if (iClientID < 0 && cat == PVR_MENUHOOK_SETTING)
  {
    PVR_CLIENTMAP clients;
    GetConnectedClients(clients);

    if (clients.size() == 1)
    {
      iClientID = clients.begin()->first;
    }
    else if (clients.size() > 1)
    {
      // have user select client
      CGUIDialogSelect* pDialog = (CGUIDialogSelect*)g_windowManager.GetWindow(WINDOW_DIALOG_SELECT);
      pDialog->Reset();
      pDialog->SetHeading(19196);

      PVR_CLIENTMAP_CITR itrClients;
      for (itrClients = clients.begin(); itrClients != clients.end(); itrClients++)
      {
        pDialog->Add(itrClients->second->GetBackendName());
      }
      pDialog->DoModal();

      int selection = pDialog->GetSelectedLabel();
      if (selection >= 0)
      {
        itrClients = clients.begin();
        for (int i = 0; i < selection; i++)
          itrClients++;
        iClientID = itrClients->first;
      }
    }
  }

  if (iClientID < 0)
    iClientID = GetPlayingClientID();

  PVR_CLIENT client;
  if (GetConnectedClient(iClientID, client) && client->HaveMenuHooks(cat))
  {
    PVR_MENUHOOKS *hooks = client->GetMenuHooks();
    std::vector<int> hookIDs;
    int selection = 0;

    CGUIDialogSelect* pDialog = (CGUIDialogSelect*)g_windowManager.GetWindow(WINDOW_DIALOG_SELECT);
    pDialog->Reset();
    pDialog->SetHeading(19196);
    for (unsigned int i = 0; i < hooks->size(); i++)
      if (hooks->at(i).category == cat || hooks->at(i).category == PVR_MENUHOOK_ALL)
      {
        pDialog->Add(client->GetString(hooks->at(i).iLocalizedStringId));
        hookIDs.push_back(i);
      }
    if (hookIDs.size() > 1)
    {
      pDialog->DoModal();
      selection = pDialog->GetSelectedLabel();
    }
    if (selection >= 0)
      client->CallMenuHook(hooks->at(hookIDs.at(selection)), item);
  }
}

bool CPVRClients::IsRunningChannelScan(void) const
{
  CSingleLock lock(m_critSection);
  return m_bChannelScanRunning;
}

std::vector<PVR_CLIENT> CPVRClients::GetClientsSupportingChannelScan(void) const
{
  std::vector<PVR_CLIENT> possibleScanClients;
  CSingleLock lock(m_critSection);

  /* get clients that support channel scanning */
  for (PVR_CLIENTMAP_CITR itr = m_clientMap.begin(); itr != m_clientMap.end(); itr++)
  {
    if (itr->second->ReadyToUse() && itr->second->SupportsChannelScan())
      possibleScanClients.push_back(itr->second);
  }

  return possibleScanClients;
}

void CPVRClients::StartChannelScan(void)
{
  PVR_CLIENT scanClient;
  CSingleLock lock(m_critSection);
  std::vector<PVR_CLIENT> possibleScanClients = GetClientsSupportingChannelScan();
  m_bChannelScanRunning = true;

  /* multiple clients found */
  if (possibleScanClients.size() > 1)
  {
    CGUIDialogSelect* pDialog= (CGUIDialogSelect*)g_windowManager.GetWindow(WINDOW_DIALOG_SELECT);

    pDialog->Reset();
    pDialog->SetHeading(19119);

    for (unsigned int i = 0; i < possibleScanClients.size(); i++)
      pDialog->Add(possibleScanClients[i]->GetFriendlyName());

    pDialog->DoModal();

    int selection = pDialog->GetSelectedLabel();
    if (selection >= 0)
      scanClient = possibleScanClients[selection];
  }
  /* one client found */
  else if (possibleScanClients.size() == 1)
  {
    scanClient = possibleScanClients[0];
  }
  /* no clients found */
  else if (!scanClient)
  {
    CGUIDialogOK::ShowAndGetInput(19033, 19192);
    return;
  }

  /* start the channel scan */
  CLog::Log(LOGNOTICE,"PVR - %s - starting to scan for channels on client %s",
      __FUNCTION__, scanClient->GetFriendlyName().c_str());
  long perfCnt = XbmcThreads::SystemClockMillis();

  /* stop the supervisor thread */
  g_PVRManager.StopUpdateThreads();

  /* do the scan */
  if (scanClient->StartChannelScan() != PVR_ERROR_NO_ERROR)
    /* an error occured */
    CGUIDialogOK::ShowAndGetInput(19111, 19193);

  /* restart the supervisor thread */
  g_PVRManager.StartUpdateThreads();

  CLog::Log(LOGNOTICE, "PVRManager - %s - channel scan finished after %li.%li seconds",
      __FUNCTION__, (XbmcThreads::SystemClockMillis()-perfCnt)/1000, (XbmcThreads::SystemClockMillis()-perfCnt)%1000);
  m_bChannelScanRunning = false;
}

std::vector<PVR_CLIENT> CPVRClients::GetClientsSupportingChannelSettings(bool bRadio) const
{
  std::vector<PVR_CLIENT> possibleSettingsClients;
  CSingleLock lock(m_critSection);

  /* get clients that support channel settings */
  for (PVR_CLIENTMAP_CITR itr = m_clientMap.begin(); itr != m_clientMap.end(); itr++)
  {
    if (itr->second->ReadyToUse() && itr->second->SupportsChannelSettings() &&
         ((bRadio && itr->second->SupportsRadio()) || (!bRadio && itr->second->SupportsTV())))
      possibleSettingsClients.push_back(itr->second);
  }

  return possibleSettingsClients;
}


bool CPVRClients::OpenDialogChannelAdd(const CPVRChannelPtr &channel)
{
  PVR_ERROR error = PVR_ERROR_UNKNOWN;

  PVR_CLIENT client;
  if (GetConnectedClient(channel->ClientID(), client))
    error = client->OpenDialogChannelAdd(channel);
  else
    CLog::Log(LOGERROR, "PVR - %s - cannot find client %d",__FUNCTION__, channel->ClientID());

  if (error == PVR_ERROR_NOT_IMPLEMENTED)
  {
    CGUIDialogOK::ShowAndGetInput(19033, 19038);
    return true;
  }

  return error == PVR_ERROR_NO_ERROR;
}

bool CPVRClients::OpenDialogChannelSettings(const CPVRChannelPtr &channel)
{
  PVR_ERROR error = PVR_ERROR_UNKNOWN;

  PVR_CLIENT client;
  if (GetConnectedClient(channel->ClientID(), client))
    error = client->OpenDialogChannelSettings(channel);
  else
    CLog::Log(LOGERROR, "PVR - %s - cannot find client %d",__FUNCTION__, channel->ClientID());

  if (error == PVR_ERROR_NOT_IMPLEMENTED)
  {
    CGUIDialogOK::ShowAndGetInput(19033, 19038);
    return true;
  }

  return error == PVR_ERROR_NO_ERROR;
}

bool CPVRClients::DeleteChannel(const CPVRChannelPtr &channel)
{
  PVR_ERROR error = PVR_ERROR_UNKNOWN;

  PVR_CLIENT client;
  if (GetConnectedClient(channel->ClientID(), client))
    error = client->DeleteChannel(channel);
  else
    CLog::Log(LOGERROR, "PVR - %s - cannot find client %d",__FUNCTION__, channel->ClientID());

  if (error == PVR_ERROR_NOT_IMPLEMENTED)
  {
    CGUIDialogOK::ShowAndGetInput(19033, 19038);
    return true;
  }

  return error == PVR_ERROR_NO_ERROR;
}

bool CPVRClients::RenameChannel(const CPVRChannelPtr &channel)
{
  PVR_ERROR error = PVR_ERROR_UNKNOWN;

  PVR_CLIENT client;
  if (GetConnectedClient(channel->ClientID(), client))
    error = client->RenameChannel(channel);
  else
    CLog::Log(LOGERROR, "PVR - %s - cannot find client %d",__FUNCTION__, channel->ClientID());

  return (error == PVR_ERROR_NO_ERROR || error == PVR_ERROR_NOT_IMPLEMENTED);
}

bool CPVRClients::IsKnownClient(const AddonPtr client) const
{
  // database IDs start at 1
  return GetClientId(client) > 0;
}

int CPVRClients::RegisterClient(AddonPtr client)
{
  int iClientId(-1);
  CAddonDatabase database;
  PVR_CLIENT addon;

  if (!client->Enabled() || !database.Open())
    return -1;

  CLog::Log(LOGDEBUG, "%s - registering add-on '%s'", __FUNCTION__, client->Name().c_str());

  // check whether we already know this client
  iClientId = database.GetAddonId(client); //database->GetClientId(client->ID());

  // try to register the new client in the db
  if (iClientId <= 0)
    iClientId = database.AddAddon(client, 0);

  if (iClientId > 0)
  // load and initialise the client libraries
  {
    CSingleLock lock(m_critSection);
    PVR_CLIENTMAP_CITR existingClient = m_clientMap.find(iClientId);
    if (existingClient != m_clientMap.end())
    {
      // return existing client
      addon = existingClient->second;
    }
    else
    {
      // create a new client instance
      addon = std::dynamic_pointer_cast<CPVRClient>(client);
      m_clientMap.insert(std::make_pair(iClientId, addon));
      m_addonNameIds.insert(make_pair(addon->ID(), iClientId));
    }
  }

  if (iClientId <= 0)
    CLog::Log(LOGERROR, "PVR - %s - can't register add-on '%s'", __FUNCTION__, client->Name().c_str());

  return iClientId;
}

bool CPVRClients::UpdateAndInitialiseClients(bool bInitialiseAllClients /* = false */)
{
  bool bReturn(true);
  VECADDONS map;
  VECADDONS disableAddons;
  {
    CSingleLock lock(m_critSection);
    map = m_addons;
  }

  if (map.empty())
    return false;

  for (VECADDONS::iterator it = map.begin(); it != map.end(); ++it)
  {
    bool bEnabled = (*it)->Enabled() &&
        !CAddonMgr::Get().IsAddonDisabled((*it)->ID());

    if (!bEnabled && IsKnownClient(*it))
    {
      CSingleLock lock(m_critSection);
      /* stop the client and remove it from the db */
      StopClient(*it, false);
      disableAddons.push_back(*it);

    }
    else if (bEnabled && (bInitialiseAllClients || !IsKnownClient(*it) || !IsConnectedClient(*it)))
    {
      bool bDisabled(false);

      // register the add-on in the pvr db, and create the CPVRClient instance
      int iClientId = RegisterClient(*it);
      if (iClientId <= 0)
      {
        // failed to register or create the add-on, disable it
        CLog::Log(LOGWARNING, "%s - failed to register add-on %s, disabling it", __FUNCTION__, (*it)->Name().c_str());
        disableAddons.push_back(*it);
        bDisabled = true;
      }
      else
      {
        ADDON_STATUS status(ADDON_STATUS_UNKNOWN);
        PVR_CLIENT addon;
        {
          CSingleLock lock(m_critSection);
          if (!GetClient(iClientId, addon))
          {
            CLog::Log(LOGWARNING, "%s - failed to find add-on %s, disabling it", __FUNCTION__, (*it)->Name().c_str());
            disableAddons.push_back(*it);
            bDisabled = true;
          }
        }

        // throttle connection attempts, no more than 1 attempt per 5 seconds
        if (!bDisabled && addon->Enabled())
        {
          time_t now;
          CDateTime::GetCurrentDateTime().GetAsTime(now);
          std::map<int, time_t>::iterator it = m_connectionAttempts.find(iClientId);
          if (it != m_connectionAttempts.end() && now < it->second)
            continue;
          m_connectionAttempts[iClientId] = now + 5;
        }

        // re-check the enabled status. newly installed clients get disabled when they're added to the db
        if (!bDisabled && addon->Enabled() && (status = addon->Create(iClientId)) != ADDON_STATUS_OK)
        {
          CLog::Log(LOGWARNING, "%s - failed to create add-on %s, status = %d", __FUNCTION__, (*it)->Name().c_str(), status);
          if (!addon.get() || !addon->DllLoaded() || status == ADDON_STATUS_PERMANENT_FAILURE)
          {
            // failed to load the dll of this add-on, disable it
            CLog::Log(LOGWARNING, "%s - failed to load the dll for add-on %s, disabling it", __FUNCTION__, (*it)->Name().c_str());
            disableAddons.push_back(*it);
            bDisabled = true;
          }
        }
      }

      if (bDisabled && (g_PVRManager.IsStarted() || g_PVRManager.IsInitialising()))
        CGUIDialogOK::ShowAndGetInput(24070, 16029);
    }
  }

  // disable add-ons that failed to initialise
  if (!disableAddons.empty())
  {
    CSingleLock lock(m_critSection);
    for (VECADDONS::iterator it = disableAddons.begin(); it != disableAddons.end(); ++it)
    {
      // disable in the add-on db
      CAddonMgr::Get().DisableAddon((*it)->ID());

      // remove from the pvr add-on list
      VECADDONS::iterator addonPtr = std::find(m_addons.begin(), m_addons.end(), *it);
      if (addonPtr != m_addons.end())
        m_addons.erase(addonPtr);
    }
  }

  return bReturn;
}

void CPVRClients::Process(void)
{
  bool bCheckedEnabledClientsOnStartup(false);

  CAddonMgr::Get().RegisterAddonMgrCallback(ADDON_PVRDLL, this);
  CAddonMgr::Get().RegisterObserver(this);

  UpdateAddons();

  while (!g_application.m_bStop && !m_bStop)
  {
    UpdateAndInitialiseClients();

    if (!bCheckedEnabledClientsOnStartup)
    {
      bCheckedEnabledClientsOnStartup = true;
      if (!HasEnabledClients() && !m_bNoAddonWarningDisplayed)
      {
        if (AutoconfigureClients())
          m_bNoAddonWarningDisplayed = true;
        else
          ShowDialogNoClientsEnabled();
      }
      m_bRestartManagerOnAddonDisabled = true;
    }
    else
    {
      Sleep(1000);
    }
  }
}

bool CPVRClients::AutoconfigureClients(void)
{
  bool bReturn(false);
  std::vector<PVR_CLIENT> autoConfigAddons;
  PVR_CLIENT addon;
  VECADDONS map;
  CAddonMgr::Get().GetAddons(ADDON_PVRDLL, map, false);

  /** get the auto-configurable add-ons */
  for (VECADDONS::iterator it = map.begin(); it != map.end(); ++it)
  {
    if (CAddonMgr::Get().IsAddonDisabled((*it)->ID()))
    {
      addon = std::dynamic_pointer_cast<CPVRClient>(*it);
      if (addon->CanAutoconfigure())
        autoConfigAddons.push_back(addon);
    }
  }

  /** no configurable add-ons found */
  if (autoConfigAddons.size() == 0)
    return bReturn;

  /** display a progress bar while trying to auto-configure add-ons */
  CGUIDialogExtendedProgressBar *loadingProgressDialog = (CGUIDialogExtendedProgressBar *)g_windowManager.GetWindow(WINDOW_DIALOG_EXT_PROGRESS);
  CGUIDialogProgressBarHandle* progressHandle = loadingProgressDialog->GetHandle(g_localizeStrings.Get(19688)); // Scanning for PVR services
  progressHandle->SetPercentage(0);
  progressHandle->SetText(g_localizeStrings.Get(19688)); //Scanning for PVR services

  /** start zeroconf and wait a second to get some responses */
  CZeroconfBrowser::GetInstance()->Start();
  for (std::vector<PVR_CLIENT>::iterator it = autoConfigAddons.begin(); !bReturn && it != autoConfigAddons.end(); ++it)
    (*it)->AutoconfigureRegisterType();

  unsigned iIterations(0);
  float percentage(0.0f);
  float percentageStep(100.0f / PVR_CLIENT_AVAHI_SCAN_ITERATIONS);
  progressHandle->SetPercentage(percentage);

  /** while no add-ons were configured within 20 iterations */
  while (!bReturn && iIterations++ < PVR_CLIENT_AVAHI_SCAN_ITERATIONS)
  {
    /** check each disabled add-on */
    for (std::vector<PVR_CLIENT>::iterator it = autoConfigAddons.begin(); !bReturn && it != autoConfigAddons.end(); ++it)
    {
      if (addon->Autoconfigure())
      {
        progressHandle->SetPercentage(100.0f);
        progressHandle->MarkFinished();

        /** enable the add-on */
        CAddonMgr::Get().EnableAddon((*it)->ID());
        CSingleLock lock(m_critSection);
        m_addons.push_back(*it);
        bReturn = true;
      }
    }

    /** wait a while and try again */
    if (!bReturn)
    {
      percentage += percentageStep;
      progressHandle->SetPercentage(percentage);
      Sleep(PVR_CLIENT_AVAHI_SLEEP_TIME_MS);
    }
  }

  progressHandle->SetPercentage(100.0f);
  progressHandle->MarkFinished();
  return bReturn;
}

void CPVRClients::ShowDialogNoClientsEnabled(void)
{
  if (!g_PVRManager.IsStarted() && !g_PVRManager.IsInitialising())
    return;

  CGUIDialogOK::ShowAndGetInput(19240, 19241);

  std::vector<std::string> params;
  params.push_back("addons://disabled/xbmc.pvrclient");
  params.push_back("return");
  g_windowManager.ActivateWindow(WINDOW_ADDON_BROWSER, params);
}

bool CPVRClients::UpdateAddons(void)
{
  VECADDONS addons;
  PVR_CLIENT addon;
  bool bReturn(CAddonMgr::Get().GetAddons(ADDON_PVRDLL, addons, true));
  size_t usableClients;
  bool bDisable(false);

  if (bReturn)
  {
    CSingleLock lock(m_critSection);
    m_addons = addons;
  }

  usableClients = m_addons.size();

  // handle "new" addons which aren't yet in the db - these have to be added first
  for (VECADDONS::const_iterator it = addons.begin(); it != addons.end(); ++it)
  {
    if (RegisterClient(*it) < 0)
      bDisable = true;
    else
    {
      addon = std::dynamic_pointer_cast<CPVRClient>(*it);
      bDisable = addon &&
          addon->NeedsConfiguration() &&
          addon->HasSettings() &&
          !addon->HasUserSettings();
    }

    if (bDisable)
    {
      CLog::Log(LOGDEBUG, "%s - disabling add-on '%s'", __FUNCTION__, (*it)->Name().c_str());
      CAddonMgr::Get().DisableAddon((*it)->ID());
      usableClients--;
    }
  }

  if ((!bReturn || usableClients == 0) && !m_bNoAddonWarningDisplayed &&
      !CAddonMgr::Get().HasAddons(ADDON_PVRDLL, false) &&
      (g_PVRManager.IsStarted() || g_PVRManager.IsInitialising()))
  {
    // No PVR add-ons could be found
    // You need a tuner, backend software, and an add-on for the backend to be able to use PVR.
    // Please visit http://kodi.wiki/view/PVR to learn more.
    m_bNoAddonWarningDisplayed = true;
    CGUIDialogOK::ShowAndGetInput(19271, 19272);
    CSettings::Get().SetBool("pvrmanager.enabled", false);
    CGUIMessage msg(GUI_MSG_UPDATE, WINDOW_SETTINGS_MYPVR, 0);
    g_windowManager.SendThreadMessage(msg, WINDOW_SETTINGS_MYPVR);
  }

  return bReturn;
}

void CPVRClients::Notify(const Observable &obs, const ObservableMessage msg)
{
  if (msg == ObservableMessageAddons)
    UpdateAddons();
}

bool CPVRClients::GetClient(const std::string &strId, AddonPtr &addon) const
{
  CSingleLock lock(m_critSection);
  for (PVR_CLIENTMAP_CITR itr = m_clientMap.begin(); itr != m_clientMap.end(); itr++)
  {
    if (itr->second->ID() == strId)
    {
      addon = itr->second;
      return true;
    }
  }
  return false;
}

bool CPVRClients::SupportsChannelGroups(int iClientId) const
{
  PVR_CLIENT client;
  return GetConnectedClient(iClientId, client) && client->SupportsChannelGroups();
}

bool CPVRClients::SupportsChannelScan(int iClientId) const
{
  PVR_CLIENT client;
  return GetConnectedClient(iClientId, client) && client->SupportsChannelScan();
}

bool CPVRClients::SupportsChannelSettings(int iClientId) const
{
  PVR_CLIENT client;
  return GetConnectedClient(iClientId, client) && client->SupportsChannelSettings();
}

bool CPVRClients::SupportsEPG(int iClientId) const
{
  PVR_CLIENT client;
  return GetConnectedClient(iClientId, client) && client->SupportsEPG();
}

bool CPVRClients::SupportsLastPlayedPosition(int iClientId) const
{
  PVR_CLIENT client;
  return GetConnectedClient(iClientId, client) && client->SupportsLastPlayedPosition();
}

bool CPVRClients::SupportsRadio(int iClientId) const
{
  PVR_CLIENT client;
  return GetConnectedClient(iClientId, client) && client->SupportsRadio();
}

bool CPVRClients::SupportsRecordings(int iClientId) const
{
  PVR_CLIENT client;
  return GetConnectedClient(iClientId, client) && client->SupportsRecordings();
}

bool CPVRClients::SupportsRecordingsUndelete(int iClientId) const
{
  PVR_CLIENT client;
  return GetConnectedClient(iClientId, client) && client->SupportsRecordingsUndelete();
}

bool CPVRClients::SupportsRecordingPlayCount(int iClientId) const
{
  PVR_CLIENT client;
  return GetConnectedClient(iClientId, client) && client->SupportsRecordingPlayCount();
}

bool CPVRClients::SupportsRecordingEdl(int iClientId) const
{
  PVR_CLIENT client;
  return GetConnectedClient(iClientId, client) && client->SupportsRecordingEdl();
}

bool CPVRClients::SupportsTimers(int iClientId) const
{
  PVR_CLIENT client;
  return GetConnectedClient(iClientId, client) && client->SupportsTimers();
}

bool CPVRClients::SupportsTV(int iClientId) const
{
  PVR_CLIENT client;
  return GetConnectedClient(iClientId, client) && client->SupportsTV();
}

bool CPVRClients::HandlesDemuxing(int iClientId) const
{
  PVR_CLIENT client;
  return GetConnectedClient(iClientId, client) && client->HandlesDemuxing();
}

bool CPVRClients::HandlesInputStream(int iClientId) const
{
  PVR_CLIENT client;
  return GetConnectedClient(iClientId, client) && client->HandlesInputStream();
}

bool CPVRClients::GetPlayingClient(PVR_CLIENT &client) const
{
  return GetConnectedClient(GetPlayingClientID(), client);
}

bool CPVRClients::OpenStream(const CPVRChannelPtr &channel, bool bIsSwitchingChannel)
{
  assert(channel.get());

  bool bReturn(false);
  CloseStream();

  /* try to open the stream on the client */
  PVR_CLIENT client;
  if (GetConnectedClient(channel->ClientID(), client) &&
      client->OpenStream(channel, bIsSwitchingChannel))
  {
    CSingleLock lock(m_critSection);
    m_playingClientId = channel->ClientID();
    m_bIsPlayingLiveTV = true;

    if (client.get())
      m_strPlayingClientName = client->GetFriendlyName();
    else
      m_strPlayingClientName = g_localizeStrings.Get(13205);

    bReturn = true;
  }

  return bReturn;
}

bool CPVRClients::OpenStream(const CPVRRecordingPtr &channel)
{
  assert(channel.get());

  bool bReturn(false);
  CloseStream();

  /* try to open the recording stream on the client */
  PVR_CLIENT client;
  if (GetConnectedClient(channel->m_iClientId, client) &&
      client->OpenStream(channel))
  {
    CSingleLock lock(m_critSection);
    m_playingClientId = channel->m_iClientId;
    m_bIsPlayingRecording = true;
    m_strPlayingClientName = client->GetFriendlyName();
    bReturn = true;
  }

  return bReturn;
}

void CPVRClients::CloseStream(void)
{
  PVR_CLIENT playingClient;
  if (GetPlayingClient(playingClient))
    playingClient->CloseStream();

  CSingleLock lock(m_critSection);
  m_bIsPlayingLiveTV     = false;
  m_bIsPlayingRecording  = false;
  m_playingClientId      = PVR_INVALID_CLIENT_ID;
  m_strPlayingClientName = "";
}

int CPVRClients::ReadStream(void* lpBuf, int64_t uiBufSize)
{
  PVR_CLIENT client;
  if (GetPlayingClient(client))
    return client->ReadStream(lpBuf, uiBufSize);
  return -EINVAL;
}

int64_t CPVRClients::GetStreamLength(void)
{
  PVR_CLIENT client;
  if (GetPlayingClient(client))
    return client->GetStreamLength();
  return -EINVAL;
}

int64_t CPVRClients::SeekStream(int64_t iFilePosition, int iWhence/* = SEEK_SET*/)
{
  PVR_CLIENT client;
  if (GetPlayingClient(client))
    return client->SeekStream(iFilePosition, iWhence);
  return -EINVAL;
}

int64_t CPVRClients::GetStreamPosition(void)
{
  PVR_CLIENT client;
  if (GetPlayingClient(client))
    return client->GetStreamPosition();
  return -EINVAL;
}

void CPVRClients::PauseStream(bool bPaused)
{
  PVR_CLIENT client;
  if (GetPlayingClient(client))
    client->PauseStream(bPaused);
}

std::string CPVRClients::GetCurrentInputFormat(void) const
{
  std::string strReturn;
  CPVRChannelPtr currentChannel(GetPlayingChannel());
  if (currentChannel)
    strReturn = currentChannel->InputFormat();

  return strReturn;
}

bool CPVRClients::IsPlaying(void) const
{
  CSingleLock lock(m_critSection);
  return m_bIsPlayingRecording || m_bIsPlayingLiveTV;
}

bool CPVRClients::IsPlayingRadio(void) const
{
  PVR_CLIENT client;
  if (GetPlayingClient(client))
    return client->IsPlayingLiveRadio();
  return false;
}

bool CPVRClients::IsPlayingTV(void) const
{
  PVR_CLIENT client;
  if (GetPlayingClient(client))
    return client->IsPlayingLiveTV();
  return false;
}

bool CPVRClients::IsPlayingRecording(void) const
{
  CSingleLock lock(m_critSection);
  return m_bIsPlayingRecording;
}

bool CPVRClients::IsReadingLiveStream(void) const
{
  CSingleLock lock(m_critSection);
  return m_bIsPlayingLiveTV;
}

bool CPVRClients::IsEncrypted(void) const
{
  PVR_CLIENT client;
  if (GetPlayingClient(client))
    return client->IsPlayingEncryptedChannel();
  return false;
}

std::string CPVRClients::GetBackendHostnameByClientId(int iClientId) const
{
  PVR_CLIENT client;
  std::string name;

  if (GetConnectedClient(iClientId, client))
  {
    name = client->GetBackendHostname();
  }

  return name;
}

time_t CPVRClients::GetPlayingTime() const
{
  PVR_CLIENT client;
  time_t time = 0;

  if (GetPlayingClient(client))
  {
     time = client->GetPlayingTime();
  }

  return time;
}

time_t CPVRClients::GetBufferTimeStart() const
{
  PVR_CLIENT client;
  time_t time = 0;

  if (GetPlayingClient(client))
  {
    time = client->GetBufferTimeStart();
  }

  return time;
}

time_t CPVRClients::GetBufferTimeEnd() const
{
  PVR_CLIENT client;
  time_t time = 0;

  if (GetPlayingClient(client))
  {
    time = client->GetBufferTimeEnd();
  }

  return time;
}

