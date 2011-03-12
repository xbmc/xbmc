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
  for (unsigned int iTimerPtr = 0; iTimerPtr < size(); iTimerPtr++)
    delete at(iTimerPtr);
  clear();
}

int CPVRTimers::Update()
{
  CSingleLock lock(m_critSection);

  CLog::Log(LOGDEBUG, "PVRTimers - %s - updating timers",
      __FUNCTION__);

  int iCurSize = size();

  /* clear channel timers */
  for (unsigned int iTimerPtr = 0; iTimerPtr < size(); iTimerPtr++)
  {
    CPVRTimerInfoTag *timerTag = at(iTimerPtr);
    if (!timerTag || !timerTag->m_bIsActive)
      continue;

    CPVREpgInfoTag *epgTag = (CPVREpgInfoTag *) timerTag->m_EpgInfo;
    if (!epgTag)
      continue;

    epgTag->SetTimer(NULL);
  }

  /* clear timers */
  clear();

  /* get all timers from the clients */
  CLIENTMAP *clients = CPVRManager::Get()->Clients();
  CLIENTMAPITR itr = clients->begin();
  while (itr != clients->end())
  {
    if (CPVRManager::Get()->GetClientProperties((*itr).second->GetID())->SupportTimers)
    {
      if ((*itr).second->GetNumTimers() > 0)
      {
        (*itr).second->GetAllTimers(this);
      }
    }
    itr++;
  }

  /* set channel timers */
  for (unsigned int ptr = 0; ptr < size(); ptr++)
  {
    /* get the timer tag */
    CPVRTimerInfoTag *timerTag = at(ptr);
    if (!timerTag || !timerTag->m_bIsActive)
      continue;

    /* try to get the channel */
    CPVRChannel *channel = (CPVRChannel *) CPVRManager::GetChannelGroups()->GetByUniqueID(timerTag->m_iClientChannelUid, timerTag->m_iClientID);
    if (!channel)
      continue;

    /* try to get the EPG */
    CPVREpg *epg = channel->GetEPG();
    if (!epg)
      continue;

    /* try to set the timer on the epg tag that matches */
    CPVREpgInfoTag *epgTag = (CPVREpgInfoTag *) epg->InfoTagBetween(timerTag->m_StartTime, timerTag->m_StopTime);
    if (epgTag)
      epgTag->SetTimer(timerTag);
  }

  return size() - iCurSize;
}

bool CPVRTimers::Update(const CPVRTimerInfoTag &timer)
{
  CPVRTimerInfoTag *newTag = NULL;
  if (true) // TODO check if we already have a matching timer
  {
    newTag = new CPVRTimerInfoTag();
    push_back(newTag);
  }

  newTag->m_iClientID         = timer.m_iClientID;
  newTag->m_iClientIndex      = timer.m_iClientIndex;
  newTag->m_bIsActive         = timer.m_bIsActive;
  newTag->m_strTitle          = timer.m_strTitle;
  newTag->m_strDir            = timer.m_strDir;
  newTag->m_iClientNumber     = timer.m_iClientNumber;
  newTag->m_iClientChannelUid = timer.m_iClientChannelUid;
  newTag->m_StartTime         = timer.m_StartTime;
  newTag->m_StopTime          = timer.m_StopTime;
  newTag->m_FirstDay          = timer.m_FirstDay;
  newTag->m_iPriority         = timer.m_iPriority;
  newTag->m_iLifetime         = timer.m_iLifetime;
  newTag->m_bIsRecording      = timer.m_bIsRecording;
  newTag->m_bIsRepeating      = timer.m_bIsRepeating;
  newTag->m_iWeekdays         = timer.m_iWeekdays;
  newTag->m_iChannelNumber    = timer.m_iChannelNumber;
  newTag->m_bIsRadio          = timer.m_bIsRadio;

  // TODO epg entry

  return true;
}

/********** getters **********/

int CPVRTimers::GetTimers(CFileItemList* results)
{
  Update();

  for (unsigned int i = 0; i < size(); ++i)
  {
    CFileItemPtr timer(new CFileItem(*at(i)));
    results->Add(timer);
  }

  return size();
}

CPVRTimerInfoTag *CPVRTimers::GetNextActiveTimer(void)
{
  CSingleLock lock(m_critSection);
  CPVRTimerInfoTag *t0 = NULL;
  for (unsigned int i = 0; i < size(); i++)
  {
    if ((at(i)->m_bIsActive) && (!t0 || (at(i)->m_StopTime > CDateTime::GetCurrentDateTime() && at(i)->Compare(*t0) < 0)))
    {
      t0 = at(i);
    }
  }
  return t0;
}

int CPVRTimers::GetNumTimers()
{
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

    Update();

    item.reset(new CFileItem(base + "/add.timer", false));
    item->SetLabel(g_localizeStrings.Get(19026));
    item->SetLabelPreformated(true);
    items.Add(item);

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
  if (!CPVRManager::Get()->GetClientProperties(item.m_iClientID)->SupportTimers)
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

  const CPVRTimerInfoTag* tag = item.GetPVRTimerInfoTag();
  if (!tag)
    return false;

  return DeleteTimer(*tag, bForce);
}

bool CPVRTimers::DeleteTimer(const CPVRTimerInfoTag &item, bool bForce /* = false */)
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
  if (item.RenameOnClient(strNewName))
  {
    item.m_strTitle = strNewName;
    return true;
  }

  return false;
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

CPVRTimerInfoTag *CPVRTimers::GetMatch(const CEpgInfoTag *Epg)
{
  CPVRTimerInfoTag *returnTag = NULL;

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

  if (item && item->HasEPGInfoTag())
    returnTag = GetMatch(item->GetEPGInfoTag());

  return returnTag;
}

