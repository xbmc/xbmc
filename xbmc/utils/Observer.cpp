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

#include "Observer.h"
#include "threads/SingleLock.h"

using namespace std;
using namespace ANNOUNCEMENT;

Observable::Observable()
{
  m_observers.clear();
  m_bAsyncAllowed = true;
  CAnnouncementManager::AddAnnouncer(this);
}

Observable::~Observable()
{
  m_observers.clear();
  CAnnouncementManager::RemoveAnnouncer(this);
}

Observable &Observable::operator=(const Observable &observable)
{
  CSingleLock lock(m_critSection);

  m_bObservableChanged = observable.m_bObservableChanged;
  for (unsigned int iObsPtr = 0; iObsPtr < m_observers.size(); iObsPtr++)
    m_observers.push_back(observable.m_observers.at(iObsPtr));

  return *this;
}

void Observable::AddObserver(Observer *o)
{
  CSingleLock lock(m_critSection);
  m_observers.push_back(o);
}

void Observable::RemoveObserver(Observer *o)
{
  CSingleLock lock(m_critSection);
  for (unsigned int ptr = 0; ptr < m_observers.size(); ptr++)
  {
    if (m_observers.at(ptr) == o)
    {
      m_observers.erase(m_observers.begin() + ptr);
      break;
    }
  }
}

void Observable::NotifyObservers(const CStdString& strMessage /* = "" */, bool bAsync /* = false */)
{
  CSingleLock lock(m_critSection);
  if (m_bObservableChanged)
  {
    if (bAsync && m_bAsyncAllowed)
    {
      CJobManager::GetInstance().AddJob(new ObservableMessageJob(*this, strMessage), this);
    }
    else
    {
      for(unsigned int ptr = 0; ptr < m_observers.size(); ptr++)
        m_observers.at(ptr)->Notify(*this, strMessage);
    }
    m_bObservableChanged = false;
  }
}

void Observable::SetChanged(bool SetTo)
{
  CSingleLock lock(m_critSection);
  m_bObservableChanged = SetTo;
}

void Observable::Announce(EAnnouncementFlag flag, const char *sender, const char *message, const CVariant &data)
{
  if (flag == System && !strcmp(sender, "xbmc") && !strcmp(message, "ApplicationStop"))
  {
    CSingleLock lock(m_critSection);
    m_bAsyncAllowed = false;
  }
}

ObservableMessageJob::ObservableMessageJob(const Observable &obs, const CStdString &strMessage)
{
  m_strMessage = strMessage;
  m_observable = obs;

  for (unsigned int iObserverPtr = 0; iObserverPtr < obs.m_observers.size(); iObserverPtr++)
    m_observers.push_back(obs.m_observers.at(iObserverPtr));
}

bool ObservableMessageJob::DoWork()
{
  for(unsigned int ptr = 0; ptr < m_observers.size(); ptr++)
    m_observers.at(ptr)->Notify(m_observable, m_strMessage);

  return true;
}
