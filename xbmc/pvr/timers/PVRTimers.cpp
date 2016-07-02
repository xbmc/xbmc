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

#include "PVRTimers.h"

#include <cassert>
#include <cstdlib>
#include <utility>

#include "dialogs/GUIDialogKaiToast.h"
#include "dialogs/GUIDialogOK.h"
#include "epg/EpgContainer.h"
#include "events/EventLog.h"
#include "events/NotificationEvent.h"
#include "FileItem.h"
#include "pvr/addons/PVRClients.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/PVRManager.h"
#include "settings/Settings.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"

using namespace PVR;
using namespace EPG;

CPVRTimers::CPVRTimers(void)
{
  m_bIsUpdating = false;
  m_iLastId     = 0;
}

CPVRTimers::~CPVRTimers(void)
{
  Unload();
}

bool CPVRTimers::Load(void)
{
  // unload previous timers
  Unload();

  // (re)register observer
  g_EpgContainer.RegisterObserver(this);

  // update from clients
  return Update();
}

void CPVRTimers::Unload()
{
  // unregister observer
  g_EpgContainer.UnregisterObserver(this);

  // remove all tags
  CSingleLock lock(m_critSection);
  m_tags.clear();
}

bool CPVRTimers::Update(void)
{
  {
    CSingleLock lock(m_critSection);
    if (m_bIsUpdating)
      return false;
    m_bIsUpdating = true;
  }

  CLog::Log(LOGDEBUG, "CPVRTimers - %s - updating timers", __FUNCTION__);
  CPVRTimers newTimerList;
  std::vector<int> failedClients;
  g_PVRClients->GetTimers(&newTimerList, failedClients);
  return UpdateEntries(newTimerList, failedClients);
}

bool CPVRTimers::IsRecording(void) const
{
  CSingleLock lock(m_critSection);

  for (MapTags::const_iterator it = m_tags.begin(); it != m_tags.end(); ++it)
    for (VecTimerInfoTag::const_iterator timerIt = it->second->begin(); timerIt != it->second->end(); ++timerIt)
      if ((*timerIt)->IsRecording())
        return true;

  return false;
}

bool CPVRTimers::SetEpgTagTimer(const CPVRTimerInfoTagPtr &timer)
{
  if (timer->IsTimerRule() || timer->m_bStartAnyTime || timer->m_bEndAnyTime)
    return false;

  std::vector<CEpgInfoTagPtr> tags(g_EpgContainer.GetEpgTagsForTimer(timer));

  if (tags.empty())
    return false;

  // assign first matching epg tag to the timer.
  timer->SetEpgTag(tags.front());

  // assign timer to every matching epg tag.
  for (const auto &tag : tags)
    tag->SetTimer(timer);

  return true;
}

bool CPVRTimers::ClearEpgTagTimer(const CPVRTimerInfoTagPtr &timer)
{
  if (timer->IsTimerRule() || timer->m_bStartAnyTime || timer->m_bEndAnyTime)
    return false;

  std::vector<CEpgInfoTagPtr> tags(g_EpgContainer.GetEpgTagsForTimer(timer));

  if (tags.empty())
    return false;

  for (const auto &tag : tags)
    tag->ClearTimer();

  return true;
}

bool CPVRTimers::UpdateEntries(const CPVRTimers &timers, const std::vector<int> &failedClients)
{
  bool bChanged(false);
  bool bAddedOrDeleted(false);
  std::vector< std::pair< int, std::string> > timerNotifications;

  CSingleLock lock(m_critSection);

  /* go through the timer list and check for updated or new timers */
  for (MapTags::const_iterator it = timers.m_tags.begin(); it != timers.m_tags.end(); ++it)
  {
    for (VecTimerInfoTag::const_iterator timerIt = it->second->begin(); timerIt != it->second->end(); ++timerIt)
    {
      /* check if this timer is present in this container */
      CPVRTimerInfoTagPtr existingTimer = GetByClient((*timerIt)->m_iClientId, (*timerIt)->m_iClientIndex);
      if (existingTimer)
      {
        /* if it's present, update the current tag */
        bool bStateChanged(existingTimer->m_state != (*timerIt)->m_state);
        ClearEpgTagTimer(existingTimer);
        if (existingTimer->UpdateEntry(*timerIt))
        {
          SetEpgTagTimer(existingTimer);

          bChanged = true;
          existingTimer->ResetChildState();

          if (bStateChanged && g_PVRManager.IsStarted())
          {
            std::string strMessage;
            existingTimer->GetNotificationText(strMessage);
            timerNotifications.push_back(std::make_pair((*timerIt)->m_iClientId, strMessage));
          }

          CLog::Log(LOGDEBUG,"PVRTimers - %s - updated timer %d on client %d",
              __FUNCTION__, (*timerIt)->m_iClientIndex, (*timerIt)->m_iClientId);
        }
      }
      else
      {
        /* new timer */
        CPVRTimerInfoTagPtr newTimer = CPVRTimerInfoTagPtr(new CPVRTimerInfoTag);
        newTimer->UpdateEntry(*timerIt);

        VecTimerInfoTag* addEntry = NULL;
        MapTags::iterator itr = m_tags.find(newTimer->m_bStartAnyTime ? CDateTime() : newTimer->StartAsUTC());
        if (itr == m_tags.end())
        {
          addEntry = new VecTimerInfoTag;
          m_tags.insert(std::make_pair(newTimer->m_bStartAnyTime ? CDateTime() : newTimer->StartAsUTC(), addEntry));
        }
        else
        {
          addEntry = itr->second;
        }

        newTimer->m_iTimerId = ++m_iLastId;
        SetEpgTagTimer(newTimer);

        addEntry->push_back(newTimer);
        bChanged = true;
        bAddedOrDeleted = true;

        if (g_PVRManager.IsStarted())
        {
          std::string strMessage;
          newTimer->GetNotificationText(strMessage);
          timerNotifications.push_back(std::make_pair(newTimer->m_iClientId, strMessage));
        }

        CLog::Log(LOGDEBUG,"PVRTimers - %s - added timer %d on client %d",
            __FUNCTION__, (*timerIt)->m_iClientIndex, (*timerIt)->m_iClientId);
      }
    }
  }

  /* to collect timer with changed starting time */
  VecTimerInfoTag timersToMove;

  /* check for deleted timers */
  for (MapTags::iterator it = m_tags.begin(); it != m_tags.end();)
  {
    for (std::vector<CPVRTimerInfoTagPtr>::iterator it2 = it->second->begin(); it2 != it->second->end();)
    {
      CPVRTimerInfoTagPtr timer(*it2);
      if (!timers.GetByClient(timer->m_iClientId, timer->m_iClientIndex))
      {
        /* timer was not found */
        bool bIgnoreTimer(false);
        for (const auto &failedClient : failedClients)
        {
          if (failedClient == timer->m_iClientId)
          {
            bIgnoreTimer = true;
            break;
          }
        }

        if (bIgnoreTimer)
        {
          ++it2;
          continue;
        }

        CLog::Log(LOGDEBUG,"PVRTimers - %s - deleted timer %d on client %d",
            __FUNCTION__, timer->m_iClientIndex, timer->m_iClientId);

        if (g_PVRManager.IsStarted())
          timerNotifications.push_back(std::make_pair(timer->m_iClientId, timer->GetDeletedNotificationText()));

        ClearEpgTagTimer(timer);

        it2 = it->second->erase(it2);

        bChanged = true;
        bAddedOrDeleted = true;
      }
      else if ((timer->m_bStartAnyTime && it->first != CDateTime()) ||
               (!timer->m_bStartAnyTime && timer->StartAsUTC() != it->first))
      {
        /* timer start has changed */
        CLog::Log(LOGDEBUG,"PVRTimers - %s - changed start time timer %d on client %d",
            __FUNCTION__, timer->m_iClientIndex, timer->m_iClientId);

        ClearEpgTagTimer(timer);

        /* remember timer */
        timersToMove.push_back(timer);

        /* remove timer for now, reinsert later */
        it2 = it->second->erase(it2);

        bChanged = true;
        bAddedOrDeleted = true;
      }
      else
      {
        ++it2;
      }
    }
    if (it->second->empty())
      it = m_tags.erase(it);
    else
      ++it;
  }

  /* reinsert timers with changed timer start */
  for (VecTimerInfoTag::const_iterator timerIt = timersToMove.begin(); timerIt != timersToMove.end(); ++timerIt)
  {
    VecTimerInfoTag* addEntry = NULL;
    MapTags::const_iterator itr = m_tags.find((*timerIt)->m_bStartAnyTime ? CDateTime() : (*timerIt)->StartAsUTC());
    if (itr == m_tags.end())
    {
      addEntry = new VecTimerInfoTag;
      m_tags.insert(std::make_pair((*timerIt)->m_bStartAnyTime ? CDateTime() : (*timerIt)->StartAsUTC(), addEntry));
    }
    else
    {
      addEntry = itr->second;
    }

    SetEpgTagTimer(*timerIt);

    addEntry->push_back(*timerIt);
  }

  /* update child information for all parent timers */
  for (const auto &tagsEntry : m_tags)
  {
    for (const auto &timersEntry : *tagsEntry.second)
    {
      if (timersEntry->GetTimerRuleId() != PVR_TIMER_NO_PARENT)
      {
        const CPVRTimerInfoTagPtr parentTimer(GetByClient(timersEntry->m_iClientId, timersEntry->GetTimerRuleId()));
        if (parentTimer)
          parentTimer->UpdateChildState(timersEntry);
      }
    }
  }

  m_bIsUpdating = false;
  if (bChanged)
  {
    UpdateChannels();
    g_PVRManager.SetChanged();
    lock.Leave();

    g_PVRManager.NotifyObservers(bAddedOrDeleted ? ObservableMessageTimersReset : ObservableMessageTimers);

    /* queue notifications / fill eventlog */
    for (const auto &entry : timerNotifications)
    {
      if (CSettings::GetInstance().GetBool(CSettings::SETTING_PVRRECORD_TIMERNOTIFICATIONS))
        CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, g_localizeStrings.Get(19166), entry.second);

      std::string strName;
      g_PVRClients->GetClientAddonName(entry.first, strName);

      std::string strIcon;
      g_PVRClients->GetClientAddonIcon(entry.first, strIcon);

      CEventLog::GetInstance().Add(EventPtr(new CNotificationEvent(strName, entry.second, strIcon, EventLevel::Information)));
    }
  }

  return bChanged;
}

bool CPVRTimers::UpdateFromClient(const CPVRTimerInfoTagPtr &timer)
{
  CSingleLock lock(m_critSection);
  CPVRTimerInfoTagPtr tag = GetByClient(timer->m_iClientId, timer->m_iClientIndex);
  if (!tag)
  {
    tag = CPVRTimerInfoTagPtr(new CPVRTimerInfoTag());
    VecTimerInfoTag* addEntry = NULL;
    MapTags::iterator itr = m_tags.find(timer->m_bStartAnyTime ? CDateTime() : timer->StartAsUTC());
    if (itr == m_tags.end())
    {
      addEntry = new VecTimerInfoTag;
      m_tags.insert(std::make_pair(timer->m_bStartAnyTime ? CDateTime() : timer->StartAsUTC(), addEntry));
    }
    else
    {
      addEntry = itr->second;
    }
    tag->m_iTimerId = ++m_iLastId;
    addEntry->push_back(tag);
  }

  return tag->UpdateEntry(timer);
}

/********** getters **********/

CFileItemPtr CPVRTimers::GetNextActiveTimer(void) const
{
  CSingleLock lock(m_critSection);
  for (MapTags::const_iterator it = m_tags.begin(); it != m_tags.end(); ++it)
  {
    for (VecTimerInfoTag::const_iterator timerIt = it->second->begin(); timerIt != it->second->end(); ++timerIt)
    {
      CPVRTimerInfoTagPtr current = *timerIt;
      if (current->IsActive() && !current->IsRecording() && !current->IsTimerRule() && !current->IsBroken())
      {
        CFileItemPtr fileItem(new CFileItem(current));
        return fileItem;
      }
    }
  }

  CFileItemPtr fileItem;
  return fileItem;
}

std::vector<CFileItemPtr> CPVRTimers::GetActiveTimers(void) const
{
  std::vector<CFileItemPtr> tags;
  CSingleLock lock(m_critSection);

  for (MapTags::const_iterator it = m_tags.begin(); it != m_tags.end(); ++it)
  {
    for (VecTimerInfoTag::const_iterator timerIt = it->second->begin(); timerIt != it->second->end(); ++timerIt)
    {
      CPVRTimerInfoTagPtr current = *timerIt;
      if (current->IsActive() && !current->IsTimerRule())
      {
        CFileItemPtr fileItem(new CFileItem(current));
        tags.push_back(fileItem);
      }
    }
  }

  return tags;
}

int CPVRTimers::AmountActiveTimers(void) const
{
  int iReturn(0);
  CSingleLock lock(m_critSection);

  for (MapTags::const_iterator it = m_tags.begin(); it != m_tags.end(); ++it)
    for (VecTimerInfoTag::const_iterator timerIt = it->second->begin(); timerIt != it->second->end(); ++timerIt)
      if ((*timerIt)->IsActive() && !(*timerIt)->IsTimerRule())
        ++iReturn;

  return iReturn;
}

std::vector<CFileItemPtr> CPVRTimers::GetActiveRecordings(void) const
{
  std::vector<CFileItemPtr> tags;
  CSingleLock lock(m_critSection);

  for (MapTags::const_iterator it = m_tags.begin(); it != m_tags.end(); ++it)
  {
    for (VecTimerInfoTag::const_iterator timerIt = it->second->begin(); timerIt != it->second->end(); ++timerIt)
    {
      CPVRTimerInfoTagPtr current = *timerIt;
      if (current->IsRecording() && !current->IsTimerRule())
      {
        CFileItemPtr fileItem(new CFileItem(current));
        tags.push_back(fileItem);
      }
    }
  }

  return tags;
}

int CPVRTimers::AmountActiveRecordings(void) const
{
  int iReturn(0);
  CSingleLock lock(m_critSection);

  for (MapTags::const_iterator it = m_tags.begin(); it != m_tags.end(); ++it)
    for (VecTimerInfoTag::const_iterator timerIt = it->second->begin(); timerIt != it->second->end(); ++timerIt)
      if ((*timerIt)->IsRecording() && !(*timerIt)->IsTimerRule())
        ++iReturn;

  return iReturn;
}

bool CPVRTimers::HasActiveTimers(void) const
{
  CSingleLock lock(m_critSection);
  for (MapTags::const_iterator it = m_tags.begin(); it != m_tags.end(); ++it)
    for (VecTimerInfoTag::const_iterator timerIt = it->second->begin(); timerIt != it->second->end(); ++timerIt)
      if ((*timerIt)->IsActive() && !(*timerIt)->IsTimerRule())
        return true;

  return false;
}

bool CPVRTimers::GetRootDirectory(const CPVRTimersPath &path, CFileItemList &items) const
{
  CFileItemPtr item(new CFileItem(CPVRTimersPath::PATH_ADDTIMER, false));
  item->SetLabel(g_localizeStrings.Get(19026)); // "Add timer..."
  item->SetLabelPreformated(true);
  item->SetSpecialSort(SortSpecialOnTop);
  items.Add(item);

  bool bRadio = path.IsRadio();
  bool bRules = path.IsRules();

  bool bHideDisabled = CSettings::GetInstance().GetBool(CSettings::SETTING_PVRTIMERS_HIDEDISABLEDTIMERS);

  CSingleLock lock(m_critSection);
  for (const auto &tagsEntry : m_tags)
  {
    for (const auto &timer : *tagsEntry.second)
    {
      if ((bRadio == timer->m_bIsRadio) &&
          (bRules == timer->IsTimerRule()) &&
          (!bHideDisabled || (timer->m_state != PVR_TIMER_STATE_DISABLED)))
      {
        item.reset(new CFileItem(timer));
        std::string strItemPath(
          CPVRTimersPath(path.GetPath(), timer->m_iClientId, timer->m_iClientIndex).GetPath());
        item->SetPath(strItemPath);
        items.Add(item);
      }
    }
  }
  return true;
}

bool CPVRTimers::GetSubDirectory(const CPVRTimersPath &path, CFileItemList &items) const
{
  bool         bRadio    = path.IsRadio();
  unsigned int iParentId = path.GetParentId();
  int          iClientId = path.GetClientId();

  bool bHideDisabled = CSettings::GetInstance().GetBool(CSettings::SETTING_PVRTIMERS_HIDEDISABLEDTIMERS);

  CFileItemPtr item;

  CSingleLock lock(m_critSection);
  for (const auto &tagsEntry : m_tags)
  {
    for (const auto &timer : *tagsEntry.second)
    {
      if ((timer->m_bIsRadio == bRadio) &&
          (timer->m_iParentClientIndex != PVR_TIMER_NO_PARENT) &&
          (timer->m_iClientId == iClientId) &&
          (timer->m_iParentClientIndex == iParentId) &&
          (!bHideDisabled || (timer->m_state != PVR_TIMER_STATE_DISABLED)))
      {
        item.reset(new CFileItem(timer));
        std::string strItemPath(
          CPVRTimersPath(path.GetPath(), timer->m_iClientId, timer->m_iClientIndex).GetPath());
        item->SetPath(strItemPath);
        items.Add(item);
      }
    }
  }
  return true;
}

bool CPVRTimers::GetDirectory(const std::string& strPath, CFileItemList &items) const
{
  CPVRTimersPath path(strPath);
  if (path.IsValid())
  {
    if (path.IsTimersRoot())
    {
      /* Root folder containing either timer rules or timers. */
      return GetRootDirectory(path, items);
    }
    else if (path.IsTimerRule())
    {
      /* Sub folder containing the timers scheduled by the given timer rule. */
      return GetSubDirectory(path, items);
    }
  }

  CLog::Log(LOGERROR,"CPVRTimers - %s - invalid URL %s", __FUNCTION__, strPath.c_str());
  return false;
}

/********** channel methods **********/

bool CPVRTimers::DeleteTimersOnChannel(const CPVRChannelPtr &channel, bool bDeleteTimerRules /* = true */, bool bCurrentlyActiveOnly /* = false */)
{
  bool bReturn = false;
  {
    CSingleLock lock(m_critSection);

    for (MapTags::reverse_iterator it = m_tags.rbegin(); it != m_tags.rend(); ++it)
    {
      for (VecTimerInfoTag::iterator timerIt = it->second->begin(); timerIt != it->second->end(); ++timerIt)
      {
        bool bDeleteActiveItem = !bCurrentlyActiveOnly || (*timerIt)->IsRecording();
        bool bDeleteTimerRuleItem = bDeleteTimerRules || !(*timerIt)->IsTimerRule();
        bool bChannelsMatch = (*timerIt)->ChannelTag() == channel;

        if (bDeleteActiveItem && bDeleteTimerRuleItem && bChannelsMatch)
        {
          CLog::Log(LOGDEBUG,"PVRTimers - %s - deleted timer %d on client %d", __FUNCTION__, (*timerIt)->m_iClientIndex, (*timerIt)->m_iClientId);
          bReturn = (*timerIt)->DeleteFromClient(true) || bReturn;
          g_PVRManager.SetChanged();
        }
      }
    }
  }

  g_PVRManager.NotifyObservers(ObservableMessageTimersReset);

  return bReturn;
}

/********** static methods **********/

bool CPVRTimers::AddTimer(const CPVRTimerInfoTagPtr &item)
{
  if (!item->m_channel && item->GetTimerType() && !item->GetTimerType()->IsEpgBasedTimerRule())
  {
    CLog::Log(LOGERROR, "PVRTimers - %s - no channel given", __FUNCTION__);
    CGUIDialogOK::ShowAndGetInput(CVariant{19033}, CVariant{19109}); // Couldn't save timer
    return false;
  }

  if (!g_PVRClients->SupportsTimers(item->m_iClientId))
  {
    CGUIDialogOK::ShowAndGetInput(CVariant{19033}, CVariant{19215});
    return false;
  }

  if (!g_PVRManager.CheckParentalLock(item->m_channel))
    return false;

  return item->AddToClient();
}

bool CPVRTimers::DeleteTimer(const CPVRTimerInfoTagPtr &tag, bool bForce /* = false */, bool bDeleteRule /* = false */)
{
  if (!tag)
    return false;

  if (bDeleteRule)
  {
    /* delete the timer rule that scheduled this timer. */
    CPVRTimerInfoTagPtr ruleTag = g_PVRTimers->GetByClient(tag->m_iClientId, tag->GetTimerRuleId());
    if (!ruleTag)
    {
      CLog::Log(LOGERROR, "PVRTimers - %s - unable to obtain timer rule for given timer", __FUNCTION__);
      return false;
    }
    return ruleTag->DeleteFromClient(bForce);
  }

  return tag->DeleteFromClient(bForce);
}

bool CPVRTimers::RenameTimer(CFileItem &item, const std::string &strNewName)
{
  /* Check if a CPVRTimerInfoTag is inside file item */
  if (!item.IsPVRTimer())
  {
    CLog::Log(LOGERROR, "PVRTimers - %s - no TimerInfoTag given", __FUNCTION__);
    return false;
  }

  CPVRTimerInfoTagPtr tag = item.GetPVRTimerInfoTag();
  if (!tag)
    return false;

  return tag->RenameOnClient(strNewName);
}

bool CPVRTimers::UpdateTimer(const CPVRTimerInfoTagPtr &item)
{
  return item->UpdateOnClient();
}

CPVRTimerInfoTagPtr CPVRTimers::GetByClient(int iClientId, unsigned int iClientTimerId) const
{
  CSingleLock lock(m_critSection);

  for (MapTags::const_iterator it = m_tags.begin(); it != m_tags.end(); ++it)
  {
    for (VecTimerInfoTag::const_iterator timerIt = it->second->begin(); timerIt != it->second->end(); ++timerIt)
    {
      if ((*timerIt)->m_iClientId == iClientId &&
          (*timerIt)->m_iClientIndex == iClientTimerId)
        return *timerIt;
    }
  }

  CPVRTimerInfoTagPtr empty;
  return empty;
}

bool CPVRTimers::IsRecordingOnChannel(const CPVRChannel &channel) const
{
  CSingleLock lock(m_critSection);

  for (MapTags::const_iterator it = m_tags.begin(); it != m_tags.end(); ++it)
  {
    for (VecTimerInfoTag::const_iterator timerIt = it->second->begin(); timerIt != it->second->end(); ++timerIt)
    {
      if ((*timerIt)->IsRecording() &&
          (*timerIt)->m_iClientChannelUid == channel.UniqueID() &&
          (*timerIt)->m_iClientId == channel.ClientID())
        return true;
    }
  }

  return false;
}

CPVRTimerInfoTagPtr CPVRTimers::GetActiveTimerForChannel(const CPVRChannelPtr &channel) const
{
  CSingleLock lock(m_critSection);
  for (const auto &tagsEntry : m_tags)
  {
    for (const auto &timersEntry : *tagsEntry.second)
    {
      if (timersEntry->IsRecording() &&
          timersEntry->m_iClientChannelUid == channel->UniqueID() &&
          timersEntry->m_iClientId == channel->ClientID())
        return timersEntry;
    }
  }

  return CPVRTimerInfoTagPtr();
}

CPVRTimerInfoTagPtr CPVRTimers::GetTimerForEpgTag(const CEpgInfoTagPtr &epgTag) const
{
  if (epgTag)
  {
    // already a timer assigned to tag?
    CPVRTimerInfoTagPtr timer(epgTag->Timer());
    if (timer)
      return timer;

    // try to find a matching timer for the tag.
    if (epgTag->ChannelTag())
    {
      const CPVRChannelPtr channel(epgTag->ChannelTag());
      CSingleLock lock(m_critSection);

      for (MapTags::const_iterator it = m_tags.begin(); it != m_tags.end(); ++it)
      {
        for (VecTimerInfoTag::const_iterator timerIt = it->second->begin(); timerIt != it->second->end(); ++timerIt)
        {
          timer = *timerIt;

          if (!timer->IsTimerRule() &&
              (timer->GetEpgInfoTag(false) == epgTag ||
               (timer->m_iEpgUid != EPG_TAG_INVALID_UID && timer->m_iEpgUid == epgTag->UniqueBroadcastID()) ||
               (timer->m_iClientChannelUid == channel->UniqueID() &&
                timer->m_bIsRadio == channel->IsRadio() &&
                timer->StartAsUTC() <= epgTag->StartAsUTC() &&
                timer->EndAsUTC() >= epgTag->EndAsUTC())))
          {
            return timer;
          }
        }
      }
    }
  }

  return CPVRTimerInfoTagPtr();
}

CPVRTimerInfoTagPtr CPVRTimers::GetTimerRule(const CPVRTimerInfoTagPtr &timer) const
{
  if (timer)
  {
    unsigned int iRuleId = timer->GetTimerRuleId();
    if (iRuleId != PVR_TIMER_NO_PARENT)
    {
      int iClientId = timer->m_iClientId;

      CSingleLock lock(m_critSection);
      for (const auto &tagsEntry : m_tags)
      {
        for (const auto &timersEntry : *tagsEntry.second)
        {
          if (timersEntry->m_iClientId == iClientId && timersEntry->m_iClientIndex == iRuleId)
            return timersEntry;
        }
      }
    }
  }
  return CPVRTimerInfoTagPtr();
}

CFileItemPtr CPVRTimers::GetTimerRule(const CFileItem *item) const
{
  CPVRTimerInfoTagPtr timer;
  if (item && item->HasEPGInfoTag())
    timer = item->GetEPGInfoTag()->Timer();
  else if (item && item->HasPVRTimerInfoTag())
    timer = item->GetPVRTimerInfoTag();

  if (timer)
    return CFileItemPtr(new CFileItem(GetTimerRule(timer)));

  return CFileItemPtr();
}

void CPVRTimers::Notify(const Observable &obs, const ObservableMessage msg)
{
  if (msg == ObservableMessageEpgContainer)
    g_PVRManager.TriggerTimersUpdate();
}

CDateTime CPVRTimers::GetNextEventTime(void) const
{
  const bool dailywakup = CSettings::GetInstance().GetBool(CSettings::SETTING_PVRPOWERMANAGEMENT_DAILYWAKEUP);
  const CDateTime now = CDateTime::GetUTCDateTime();
  const CDateTimeSpan prewakeup(0, 0, CSettings::GetInstance().GetInt(CSettings::SETTING_PVRPOWERMANAGEMENT_PREWAKEUP), 0);
  const CDateTimeSpan idle(0, 0, CSettings::GetInstance().GetInt(CSettings::SETTING_PVRPOWERMANAGEMENT_BACKENDIDLETIME), 0);

  CDateTime wakeuptime;

  /* Check next active time */
  CFileItemPtr item = GetNextActiveTimer();
  if (item && item->HasPVRTimerInfoTag())
  {
    const CDateTimeSpan prestart(0, 0, item->GetPVRTimerInfoTag()->MarginStart(), 0);
    const CDateTime start = item->GetPVRTimerInfoTag()->StartAsUTC();
    wakeuptime = ((start - prestart - prewakeup - idle) > now) ?
        start - prestart - prewakeup :
        now + idle;
  }

  /* check daily wake up */
  if (dailywakup)
  {
    CDateTime dailywakeuptime;
    dailywakeuptime.SetFromDBTime(CSettings::GetInstance().GetString(CSettings::SETTING_PVRPOWERMANAGEMENT_DAILYWAKEUPTIME));
    dailywakeuptime = dailywakeuptime.GetAsUTCDateTime();

    dailywakeuptime.SetDateTime(
      now.GetYear(), now.GetMonth(), now.GetDay(),
      dailywakeuptime.GetHour(), dailywakeuptime.GetMinute(), dailywakeuptime.GetSecond()
    );

    if ((dailywakeuptime - idle) < now)
    {
      const CDateTimeSpan oneDay(1,0,0,0);
      dailywakeuptime += oneDay;
    }
    if (!wakeuptime.IsValid() || dailywakeuptime < wakeuptime)
      wakeuptime = dailywakeuptime;
  }

  const CDateTime retVal(wakeuptime);
  return retVal;
}

void CPVRTimers::UpdateChannels(void)
{
  CSingleLock lock(m_critSection);
  for (MapTags::iterator it = m_tags.begin(); it != m_tags.end(); ++it)
  {
    for (VecTimerInfoTag::iterator timerIt = it->second->begin(); timerIt != it->second->end(); ++timerIt)
      (*timerIt)->UpdateChannel();
  }
}

void CPVRTimers::GetAll(CFileItemList& items) const
{
  CFileItemPtr item;
  CSingleLock lock(m_critSection);
  for (MapTags::const_iterator it = m_tags.begin(); it != m_tags.end(); ++it)
  {
    for (VecTimerInfoTag::const_iterator timerIt = it->second->begin(); timerIt != it->second->end(); ++timerIt)
    {
      item.reset(new CFileItem(*timerIt));
      items.Add(item);
    }
  }
}

CPVRTimerInfoTagPtr CPVRTimers::GetById(unsigned int iTimerId) const
{
  CPVRTimerInfoTagPtr item;
  CSingleLock lock(m_critSection);
  for (MapTags::const_iterator it = m_tags.begin(); !item && it != m_tags.end(); ++it)
  {
    for (VecTimerInfoTag::const_iterator timerIt = it->second->begin(); !item && timerIt != it->second->end(); ++timerIt)
    {
      if ((*timerIt)->m_iTimerId == iTimerId)
        item = *timerIt;
    }
  }
  return item;
}


//= CPVRTimersPath ============================================================

const std::string CPVRTimersPath::PATH_ADDTIMER = "pvr://timers/addtimer/";
const std::string CPVRTimersPath::PATH_NEW      = "pvr://timers/new/";

CPVRTimersPath::CPVRTimersPath(const std::string &strPath)
{
  Init(strPath);
}

CPVRTimersPath::CPVRTimersPath(const std::string &strPath, int iClientId, unsigned int iParentId)
{
  if (Init(strPath))
  {
    /* set/replace client and parent id. */
    m_path = StringUtils::Format("pvr://timers/%s/%s/%d/%d",
                                 m_bRadio      ? "radio" : "tv",
                                 m_bTimerRules ? "rules" : "timers",
                                 iClientId,
                                 iParentId);
    m_iClientId = iClientId;
    m_iParentId = iParentId;
    m_bRoot = false;
  }
}

CPVRTimersPath::CPVRTimersPath(bool bRadio, bool bTimerRules) :
  m_path(StringUtils::Format(
    "pvr://timers/%s/%s", bRadio ? "radio" : "tv", bTimerRules ? "rules" : "timers")),
  m_bValid(true),
  m_bRoot(true),
  m_bRadio(bRadio),
  m_bTimerRules(bTimerRules),
  m_iClientId(-1),
  m_iParentId(0)
{
}

bool CPVRTimersPath::Init(const std::string &strPath)
{
  std::string strVarPath(strPath);
  URIUtils::RemoveSlashAtEnd(strVarPath);

  m_path = strVarPath;
  const std::vector<std::string> segments = URIUtils::SplitPath(m_path);

  m_bValid   = (((segments.size() == 4) || (segments.size() == 6)) &&
                (segments.at(1) == "timers") &&
                ((segments.at(2) == "radio") || (segments.at(2) == "tv"))&&
                ((segments.at(3) == "rules") || (segments.at(3) == "timers")));
  m_bRoot    = (m_bValid && (segments.size() == 4));
  m_bRadio   = (m_bValid && (segments.at(2) == "radio"));
  m_bTimerRules = (m_bValid && (segments.at(3) == "rules"));

  if (!m_bValid || m_bRoot)
  {
    m_iClientId = -1;
    m_iParentId = 0;
  }
  else
  {
    char *end;
    m_iClientId = std::strtol (segments.at(4).c_str(), &end, 10);
    m_iParentId = std::strtoul(segments.at(5).c_str(), &end, 10);
  }

  return m_bValid;
}
