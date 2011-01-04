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

void CPVRTimers::Unload()
{
  Clear();
}

bool CPVRTimers::Update()
{
  CSingleLock lock(m_critSection);

  CLIENTMAP *clients = g_PVRManager.Clients();

  Clear();

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

  g_PVRManager.SyncInfo();
  return true;
}

int CPVRTimers::GetNumTimers()
{
  return size();
}

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

CPVRTimerInfoTag *CPVRTimers::GetTimer(CPVRTimerInfoTag *Timer)
{
  CSingleLock lock(m_critSection);
  for (unsigned int i = 0; i < size(); i++)
  {
    if (at(i).ChannelNumber() == Timer->Number() &&
        ((at(i).Weekdays() && at(i).Weekdays() == Timer->Weekdays()) || (!at(i).Weekdays() && at(i).FirstDay() == Timer->FirstDay())) &&
        at(i).Start() == Timer->Start() &&
        at(i).Stop() == Timer->Stop())
      return &at(i);
  }
  return NULL;
}

CPVRTimerInfoTag *CPVRTimers::GetMatch(CDateTime t)
{

  return NULL;
}

CPVRTimerInfoTag *CPVRTimers::GetMatch(time_t t)
{

  return NULL;
}

CPVRTimerInfoTag *CPVRTimers::GetMatch(const CPVREpgInfoTag *Epg, int *Match)
{

  return NULL;
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

bool CPVRTimers::AddTimer(const CFileItem &item)
{
  /* Check if a CPVRTimerInfoTag is inside file item */
  if (!item.IsPVRTimer())
  {
    CLog::Log(LOGERROR, "cPVRTimers: AddTimer no TimerInfoTag given!");
    return false;
  }

  const CPVRTimerInfoTag* tag = item.GetPVRTimerInfoTag();
  if (!g_PVRManager.GetClientProps(tag->ClientID())->SupportTimers)
  {
    CGUIDialogOK::ShowAndGetInput(19033,0,19215,0);
    return false;
  }
  return tag->Add();
}

bool CPVRTimers::DeleteTimer(const CFileItem &item, bool force)
{
  /* Check if a CPVRTimerInfoTag is inside file item */
  if (!item.IsPVRTimer())
  {
    CLog::Log(LOGERROR, "cPVRTimers: DeleteTimer no TimerInfoTag given!");
    return false;
  }

  const CPVRTimerInfoTag* tag = item.GetPVRTimerInfoTag();
  return tag->Delete(force);
}

bool CPVRTimers::RenameTimer(CFileItem &item, CStdString &newname)
{
  /* Check if a CPVRTimerInfoTag is inside file item */
  if (!item.IsPVRTimer())
  {
    CLog::Log(LOGERROR, "cPVRTimers: RenameTimer no TimerInfoTag given!");
    return false;
  }

  CPVRTimerInfoTag* tag = item.GetPVRTimerInfoTag();
  if (tag->Rename(newname))
  {
    tag->SetTitle(newname);
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
  return tag->Update();
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

void CPVRTimers::Clear()
{
  /* Clear all current present Timers inside list */
  erase(begin(), end());
  return;
}
