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

Observable::Observable()
{
  m_Observers.clear();
}

Observable::~Observable()
{
  m_Observers.clear();
}

void Observable::AddObserver(Observer& o)
{
  m_Observers.insert(&o);
}

void Observable::RemoveObserver(Observer& o)
{
  m_Observers.erase(&o);
}

void Observable::NotifyObservers(const CStdString& msg)
{
  if (m_bObservableChanged)
  {
    std::set<Observer*>::iterator itr;

    m_bObservableChanged = false;

    for(itr = m_Observers.begin(); itr != m_Observers.end(); itr++)
      (*itr)->Notify(msg);
  }
}

void Observable::SetChanged(bool SetTo)
{
  m_bObservableChanged = SetTo;
}

