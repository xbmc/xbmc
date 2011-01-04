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
  erase(begin(), end());
}

Observable::~Observable()
{
  erase(begin(), end());
}

void Observable::AddObserver(Observer *o)
{
  push_back(o);
}

void Observable::RemoveObserver(Observer *o)
{
  for (unsigned int ptr = 0; ptr < size(); ptr++)
  {
    if (at(ptr) == o)
    {
      erase(begin() + ptr);
      break;
    }
  }
}

void Observable::NotifyObservers(const CStdString& msg)
{
  if (m_bObservableChanged)
  {
    for(unsigned int ptr = 0; ptr < size(); ptr++)
    {
      Observer *obs = at(ptr);
      if (obs)
        at(ptr)->Notify(*this, msg);
    }

    m_bObservableChanged = false;
  }
}

void Observable::SetChanged(bool SetTo)
{
  m_bObservableChanged = SetTo;
}
