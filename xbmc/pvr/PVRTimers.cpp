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
#include "GUIDialogOK.h"
#include "utils/SingleLock.h"

#include "PVRTimers.h"
#include "PVRTimerInfoTag.h"
#include "PVRChannel.h"
#include "PVRManager.h"
#include "PVREpgInfoTag.h"

CPVRTimers PVRTimers;

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
    CPVRTimerInfoTag *timerTag = &at(iTimerPtr);
    if (!timerTag || !timerTag->Active())
      continue;

    CPVREpgInfoTag *epgTag = (CPVREpgInfoTag *)timerTag->EpgInfoTag();
    if (!epgTag)
      continue;

    epgTag->SetTimer(NULL);
  }

  /* clear timers */
  clear();

  /* get all timers from the clients */
  CLIENTMAP *clients = g_PVRManager.Clients();
  CLIENTMAPITR itr = clients->begin();
  while (itr != clients->end())
  {
    if (g_PVRManager.GetClientProps((*itr).second->GetID())->SupportTimers)
    {
      if ((*itr).second->GetNumTimers() > 0)
      {
        (*itr).second->GetAllTimers(this);
      }
    }
    itr++;
  }

  //XXX
  g_PVRManager.UpdateRecordingsCache();

  /* set channel timers */
  for (unsigned int ptr = 0; ptr < size(); ptr++)
  {
    /* get the timer tag */
    CPVRTimerInfoTag *timerTag = &at(ptr);
    if (!timerTag || !timerTag->Active())
      continue;

    /* try to get the channel */
    CPVRChannel *channel = CPVRChannelGroup::GetByClientFromAll(timerTag->Number(), timerTag->ClientID());
    if (!channel)
      continue;

    /* try to get the EPG */
    CPVREpg *epg = channel->GetEPG();
    if (!epg)
      continue;

    /* try to set the timer on the epg tag that matches */
    CPVREpgInfoTag *epgTag = (CPVREpgInfoTag *) epg->InfoTagBetween(timerTag->Start(), timerTag->Stop());
    if (epgTag)
      epgTag->SetTimer(timerTag);
  }

  return size() - iCurSize;
}

bool CPVRTimers::Update(const CPVRTimerInfoTag &timer)
{
  // TODO currently just adds the timer to this container
  push_back(timer);

  return true;
}

/********** getters **********/

int CPVRTimers::GetTimers(CFileItemList* results)
{
  Update();

  for (unsigned int i = 0; i < size(); ++i)
  {
    CFileItemPtr timer(new CFileItem(at(i)));
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
    if ((at(i).Active()) && (!t0 || (at(i).Stop() > CDateTime::GetCurrentDateTime() && at(i).Compare(*t0) < 0)))
    {
      t0 = &at(i);
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
  CUtil::RemoveSlashAtEnd(base);

  CURL url(strPath);
  CStdString fileName = url.GetFileName();
  CUtil::RemoveSlashAtEnd(fileName);

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
      item.reset(new CFileItem(at(i)));
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
    CPVRTimerInfoTag timer = at(ptr);

    if (timer.ChannelNumber() == channel.ChannelNumber() && timer.IsRadio() == channel.IsRadio())
      return true;
  }

  return false;
}


bool CPVRTimers::DeleteTimersOnChannel(const CPVRChannel &channel, bool bForce /* = false */)
{
  bool bReturn = true;

  for (unsigned int ptr = 0; ptr < size(); ptr++)
  {
    CPVRTimerInfoTag timer = at(ptr);

    if (timer.ChannelNumber() == channel.ChannelNumber() && timer.IsRadio() == channel.IsRadio())
    {
      bReturn = timer.DeleteFromClient(bForce) && bReturn;
      erase(begin() + ptr);
      ptr--;
    }
  }

  return bReturn;
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

  const CPVRTimerInfoTag* tag = item.GetPVRTimerInfoTag();
  if (!tag)
    return false;

  return AddTimer(*tag);
}

bool CPVRTimers::AddTimer(const CPVRTimerInfoTag &item)
{
  if (!g_PVRManager.GetClientProps(item.ClientID())->SupportTimers)
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
    item.SetTitle(strNewName);
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

bool CPVRTimers::UpdateTimer(const CPVRTimerInfoTag &item)
{
  return item.UpdateOnClient();
}

CPVRTimerInfoTag *CPVRTimers::GetMatch(CDateTime t)
{
  // TODO
  return NULL;
}

CPVRTimerInfoTag *CPVRTimers::GetMatch(time_t t)
{
  // TODO
  return NULL;
}

CPVRTimerInfoTag *CPVRTimers::GetMatch(const CPVREpgInfoTag *Epg, int *Match)
{
  // TODO
  return NULL;
}

