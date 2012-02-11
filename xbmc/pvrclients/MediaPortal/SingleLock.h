#pragma once
/*
 *      Copyright (C) 2005-2011 Team XBMC
 *      http://www.xbmc.org
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "CriticalSection.h"
#include "NonCopyable.h"
#include "client.h"

using namespace ADDON;

class CSingleLock: public NonCopyable
{
  public:
    CSingleLock(CCriticalSection& cs): m_CriticalSection(cs)
    {
      //XBMC->Log(LOG_DEBUG, "%s: lock %p", __FUNCTION__, &m_CriticalSection);
      m_CriticalSection.lock();
    };

    ~CSingleLock(void)
    {
      //XBMC->Log(LOG_DEBUG, "%s: unlock %p", __FUNCTION__, &m_CriticalSection);
      m_CriticalSection.unlock();
    };

  private:
    CCriticalSection& m_CriticalSection;
};
