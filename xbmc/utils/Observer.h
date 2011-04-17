#pragma once

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

#include "StdString.h"
#include "threads/CriticalSection.h"
#include "utils/JobManager.h"
#include "interfaces/AnnouncementManager.h"

class Observable;
class ObservableMessageJob;

class Observer
{
public:
  virtual void Notify(const Observable &obs, const CStdString& msg) = 0;
};

class Observable : public ANNOUNCEMENT::IAnnouncer
{
  friend class ObservableMessageJob;

public:
  Observable();
  virtual ~Observable();
  Observable &operator=(const Observable &observable);

  void AddObserver(Observer *o);
  void RemoveObserver(Observer *o);
  void NotifyObservers(const CStdString& msg = "", bool bAsync = false);
  void SetChanged(bool bSetTo = true);

  virtual void Announce(ANNOUNCEMENT::EAnnouncementFlag flag, const char *sender, const char *message, const CVariant &data);

private:
  static void SendMessage(Observable *obs, const CStdString &strMessage);

  bool                    m_bObservableChanged;
  std::vector<Observer *> m_observers;
  CCriticalSection        m_critSection;
  bool                    m_bAsyncAllowed;
};

class ObservableMessageJob : public CJob
{
private:
  Observable              m_observable;
  std::vector<Observer *> m_observers;
  CStdString              m_strMessage;
public:
  ObservableMessageJob(const Observable &obs, const CStdString &strMessage);
  virtual ~ObservableMessageJob() {}
  virtual const char *GetType() const { return "observable-message-job"; }

  virtual bool DoWork();
};
