/*
 *      Copyright (C) 2005-2010 Team XBMC
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

#include "FileItem.h"
#include "settings/GUISettings.h"
#include "dialogs/GUIDialogOK.h"
#include "threads/SingleLock.h"

#include "PVRTimers.h"
#include "PVRTimerInfoTag.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/channels/PVRChannel.h"
#include "pvr/PVRManager.h"
#include "pvr/epg/PVREpgInfoTag.h"

using namespace std;

CPVRTimers::CPVRTimers(void)
{

}

int CPVRTimers::Load()
{
  Unload();
  return Update();
}

void CPVRTimers::Unload()
{
  CSingleLock lock(m_critSection);
  for (unsigned int iTimerPtr = 0; iTimerPtr < size(); iTimerPtr++)
    delete at(iTimerPtr);
  clear();
}

int CPVRTimers::LoadFromClients(void)
{
  return CPVRManager::GetClients()->GetTimers(this);
}

bool CPVRTimers::Update(void)
{
  CPVRTimers PVRTimers_tmp;
  PVRTimers_tmp.LoadFromClients();

  return UpdateEntries(&PVRTimers_tmp);
}

bool CPVRTimers::IsRecording(void)
{
  bool bReturn = false;
  CSingleLock lock(m_critSection);

  for (unsigned int iTimerPtr = 0; iTimerPtr < size(); iTimerPtr++)
  {
    if (at(iTimerPtr)->IsRecording())
    {
      bReturn = true;
      break;
    }
  }

  return bReturn;
}

bool CPVRTimers::UpdateEntries(CPVRTimers *timers)
{
  bool bChanged = false;

  CSingleLock lock(m_critSection);

  /* go through the timer list and check for updated or new timers */
  for (unsigned int iTimerPtr = 0; iTimerPtr < timers->size(); iTimerPtr++)
  {
    const CPVRTimerInfoTag *timer = timers->at(iTimerPtr);

    /* check if this timer is present in this container */
    CPVRTimerInfoTag *existingTimer = (CPVRTimerInfoTag *) GetByClient(timer->m_iClientID, timer->m_iClientIndex);
    if (existingTimer)
    {
      /* if it's present, update the current tag */
      if (existingTimer->UpdateEntry(*timer))
      {
        bChanged = true;

        CLog::Log(LOGINFO,"PVRTimers - %s - updated timer %d on client %d",
            __FUNCTION__, timer->m_iClientIndex, timer->m_iClientID);
      }
    }
    else
    {
      /* new timer */
      CPVRTimerInfoTag *newTimer = new CPVRTimerInfoTag;
      newTimer->UpdateEntry(*timer);
      push_back(newTimer);
      bChanged = true;

      CLog::Log(LOGINFO,"PVRTimers - %s - added timer %d on client %d",
          __FUNCTION__, timer->m_iClientIndex, timer->m_iClientID);
    }
  }

  /* check for deleted timers */
  unsigned int iSize = size();
  for (unsigned int iTimerPtr = 0; iTimerPtr < iSize; iTimerPtr++)
  {
    CPVRTimerInfoTag *timer = (CPVRTimerInfoTag *) at(iTimerPtr);
    if (!timer)
      continue;
    if (timers->GetByClient(timer->m_iClientID, timer->m_iClientIndex) == NULL)
    {
      /* timer was not found */
      CLog::Log(LOGINFO,"PVRTimers - %s - deleted timer %d on client %d",
          __FUNCTION__, timer->m_iClientIndex, timer->m_iClientID);

      CPVREpgInfoTag *epgTag = (CPVREpgInfoTag *) at(iTimerPtr)->m_EpgInfo;
      if (epgTag)
        epgTag->SetTimer(NULL);

      delete at(iTimerPtr);
      erase(begin() + iTimerPtr);
      iTimerPtr--;
      iSize--;
      bChanged = true;
    }
  }

  return bChanged;
}

bool CPVRTimers::UpdateEntry(const CPVRTimerInfoTag &timer)
{
  CPVRTimerInfoTag *tag = NULL;
  CSingleLock lock(m_critSection);

  if ((tag = GetByClient(timer.m_iClientID, timer.m_iClientIndex)) == NULL)
  {
    tag = new CPVRTimerInfoTag();
    push_back(tag);
  }

  return tag->UpdateEntry(timer);
}

/********** getters **********/

int CPVRTimers::GetTimers(CFileItemList* results)
{
  CSingleLock lock(m_critSection);
  for (unsigned int i = 0; i < size(); ++i)
  {
    CFileItemPtr timer(new CFileItem(*at(i)));
    results->Add(timer);
  }

  return size();
}

const CPVRTimerInfoTag *CPVRTimers::GetNextActiveTimer(void)
{
  CPVRTimerInfoTag *tag = NULL;
  CSingleLock lock(m_critSection);

  for (unsigned int iTimerPtr = 0; iTimerPtr < size(); iTimerPtr++)
  {
    CPVRTimerInfoTag *current = at(iTimerPtr);
    if (current->IsActive() && (tag == NULL || current->Compare(*tag) < 0))
      tag = at(iTimerPtr);
  }

  return tag;
}

int CPVRTimers::GetActiveTimers(vector<CPVRTimerInfoTag *> *tags)
{
  int iInitialSize = tags->size();
  CSingleLock lock(m_critSection);

  for (unsigned int iTimerPtr = 0; iTimerPtr < size(); iTimerPtr++)
  {
    if (at(iTimerPtr)->IsActive())
      tags->push_back(at(iTimerPtr));
  }

  return tags->size() - iInitialSize;
}

int CPVRTimers::GetNumTimers()
{
  CSingleLock lock(m_critSection);
  return size();
}

bool CPVRTimers::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
  CStdString base(strPath);
  URIUtils::RemoveSlashAtEnd(base);

  CURL url(strPath);
  CStdString fileName = url.GetFileName();
  URIUtils::RemoveSlashAtEnd(fileName);

  if (fileName == "timers")
  {
    CFileItemPtr item;

    item.reset(new CFileItem(base + "/add.timer", false));
    item->SetLabel(g_localizeStrings.Get(19026));
    item->SetLabelPreformated(true);
    items.Add(item);

    CSingleLock lock(m_critSection);
    for (unsigned int i = 0; i < size(); ++i)
    {
      item.reset(new CFileItem(*at(i)));
      items.Add(item);
    }

    return true;
  }
  return false;
}

/********** channel methods **********/

bool CPVRTimers::ChannelHasTimers(const CPVRChannel &channel)
{
  CSingleLock lock(m_critSection);
  for (unsigned int ptr = 0; ptr < size(); ptr++)
  {
    CPVRTimerInfoTag *timer = at(ptr);

    if (timer->ChannelNumber() == channel.ChannelNumber() && timer->m_bIsRadio == channel.IsRadio())
      return true;
  }

  return false;
}


bool CPVRTimers::DeleteTimersOnChannel(const CPVRChannel &channel, bool bDeleteRepeating /* = true */, bool bCurrentlyActiveOnly /* = false */)
{
  bool bReturn = false;
  CSingleLock lock(m_critSection);

  for (unsigned int ptr = 0; ptr < size(); ptr++)
  {
    CPVRTimerInfoTag *timer = at(ptr);

    if (bCurrentlyActiveOnly &&
        (CDateTime::GetCurrentDateTime() < timer->m_StartTime ||
         CDateTime::GetCurrentDateTime() > timer->m_StopTime))
      continue;

    if (!bDeleteRepeating && timer->m_bIsRepeating)
      continue;

    if (timer->ChannelNumber() == channel.ChannelNumber() && timer->m_bIsRadio == channel.IsRadio())
    {
      bReturn = timer->DeleteFromClient(true) || bReturn;
      erase(begin() + ptr);
      ptr--;
    }
  }

  return bReturn;
}

CPVRTimerInfoTag *CPVRTimers::InstantTimer(CPVRChannel *channel, bool bStartTimer /* = true */)
{
  if (!channel)
  {
    if (!CPVRManager::Get()->GetCurrentChannel(channel))
      channel = (CPVRChannel *) CPVRManager::GetChannelGroups()->GetGroupAllTV()->GetFirstChannel();

    /* no channels present */
    if (!channel)
      return NULL;
  }

  CPVRTimerInfoTag *newTimer = new CPVRTimerInfoTag();

  int iDuration = g_guiSettings.GetInt("pvrrecord.instantrecordtime");
  if (!iDuration)
    iDuration   = 180; /* default to 180 minutes */

  int iPriority = g_guiSettings.GetInt("pvrrecord.defaultpriority");
  if (!iPriority)
    iPriority   = 50;  /* default to 50 */

  int iLifetime = g_guiSettings.GetInt("pvrrecord.defaultlifetime");
  if (!iLifetime)
    iLifetime   = 30;  /* default to 30 days */

  /* set the timer data */
  newTimer->m_iClientIndex      = -1;
  newTimer->m_bIsActive         = true;
  newTimer->m_strTitle          = channel->ChannelName();
  newTimer->m_strTitle          = g_localizeStrings.Get(19056);
  newTimer->m_iChannelNumber    = channel->ChannelNumber();
  newTimer->m_iClientNumber     = channel->ClientChannelNumber();
  newTimer->m_iClientChannelUid = channel->UniqueID();
  newTimer->m_iClientID         = channel->ClientID();
  newTimer->m_bIsRadio          = channel->IsRadio();
  newTimer->m_StartTime         = CDateTime::GetCurrentDateTime();
  newTimer->SetDuration(iDuration);
  newTimer->m_iPriority         = iPriority;
  newTimer->m_iLifetime         = iLifetime;

  /* generate summary string */
  newTimer->m_strSummary.Format("%s %s %s %s %s",
      newTimer->m_StartTime.GetAsLocalizedDate(),
      g_localizeStrings.Get(19159),
      newTimer->m_StartTime.GetAsLocalizedTime("", false),
      g_localizeStrings.Get(19160),
      newTimer->m_StopTime.GetAsLocalizedTime("", false));

  /* unused only for reference */
  newTimer->m_strFileNameAndPath = "pvr://timers/new";

  if (bStartTimer && !newTimer->AddToClient())
  {
    CLog::Log(LOGERROR, "PVRTimers - %s - unable to add an instant timer on the client", __FUNCTION__);
    delete newTimer;
    newTimer = NULL;
  }
  else
  {
    CSingleLock lock(m_critSection);
    push_back(newTimer);
    if (bStartTimer)
      channel->SetRecording(true);
  }

  return newTimer;
}

/********** static methods **********/

bool CPVRTimers::AddTimer(const CFileItem &item)
{
  /* Check if a CPVRTimerInfoTag is inside file item */
  if (!item.IsPVRTimer())
  {
    CLog::Log(LOGERROR, "cPVRTimers: AddTimer no TimerInfoTag given!");
    return false;
  }

  CPVRTimerInfoTag *tag = (CPVRTimerInfoTag *)item.GetPVRTimerInfoTag();
  if (!tag)
    return false;

  return AddTimer(*tag);
}

bool CPVRTimers::AddTimer(CPVRTimerInfoTag &item)
{
  if (!CPVRManager::GetClients()->GetClientProperties(item.m_iClientID)->SupportTimers)
  {
    CGUIDialogOK::ShowAndGetInput(19033,0,19215,0);
    return false;
  }

  return item.AddToClient();
}

bool CPVRTimers::DeleteTimer(const CFileItem &item, bool bForce /* = false */)
{
  /* Check if a CPVRTimerInfoTag is inside file item */
  if (!item.IsPVRTimer())
  {
    CLog::Log(LOGERROR, "cPVRTimers: DeleteTimer no TimerInfoTag given!");
    return false;
  }

  CPVRTimerInfoTag *tag = (CPVRTimerInfoTag *)item.GetPVRTimerInfoTag();
  if (!tag)
    return false;

  return DeleteTimer(*tag, bForce);
}

bool CPVRTimers::DeleteTimer(CPVRTimerInfoTag &item, bool bForce /* = false */)
{
  return item.DeleteFromClient(bForce);
}

bool CPVRTimers::RenameTimer(CFileItem &item, const CStdString &strNewName)
{
  /* Check if a CPVRTimerInfoTag is inside file item */
  if (!item.IsPVRTimer())
  {
    CLog::Log(LOGERROR, "cPVRTimers: RenameTimer no TimerInfoTag given!");
    return false;
  }

  CPVRTimerInfoTag* tag = item.GetPVRTimerInfoTag();
  if (!tag)
    return false;

  return RenameTimer(*tag, strNewName);
}

bool CPVRTimers::RenameTimer(CPVRTimerInfoTag &item, const CStdString &strNewName)
{
  return item.RenameOnClient(strNewName);
}

bool CPVRTimers::UpdateTimer(const CFileItem &item)
{
  /* Check if a CPVRTimerInfoTag is inside file item */
  if (!item.IsPVRTimer())
  {
    CLog::Log(LOGERROR, "cPVRTimers: UpdateTimer no TimerInfoTag given!");
    return false;
  }

  const CPVRTimerInfoTag* tag = item.GetPVRTimerInfoTag();
  if (!tag)
    return false;

  return UpdateTimer(*tag);
}

bool CPVRTimers::UpdateTimer(CPVRTimerInfoTag &item)
{
  return item.UpdateOnClient();
}

CPVRTimerInfoTag *CPVRTimers::GetByClient(int iClientId, int iClientTimerId)
{
  CPVRTimerInfoTag *returnTag = NULL;
  CSingleLock lock(m_critSection);

  for (unsigned int iTimerPtr = 0; iTimerPtr < size(); iTimerPtr++)
  {
    CPVRTimerInfoTag *timer = at(iTimerPtr);
    if (timer->m_iClientID == iClientId && timer->m_iClientIndex == iClientTimerId)
    {
      returnTag = timer;
      break;
    }
  }

  return returnTag;
}


CPVRTimerInfoTag *CPVRTimers::GetMatch(const CEpgInfoTag *Epg)
{
  CPVRTimerInfoTag *returnTag = NULL;
  CSingleLock lock(m_critSection);

  for (unsigned int ptr = 0; ptr < size(); ptr++)
  {
    CPVRTimerInfoTag *timer = at(ptr);

    if (!Epg || !Epg->GetTable() || !Epg->GetTable()->Channel())
      continue;

    const CPVRChannel *channel = Epg->GetTable()->Channel();
    if (timer->ChannelNumber() != channel->ChannelNumber()
        || timer->m_bIsRadio != channel->IsRadio())
      continue;

    if (timer->m_StartTime > Epg->Start() || timer->m_StopTime < Epg->End())
      continue;

    returnTag = timer;
    break;
  }
  return returnTag;
}

CPVRTimerInfoTag *CPVRTimers::GetMatch(const CFileItem *item)
{
  CPVRTimerInfoTag *returnTag = NULL;
  CSingleLock lock(m_critSection);

  if (item && item->HasEPGInfoTag())
    returnTag = GetMatch(item->GetEPGInfoTag());

  return returnTag;
}

