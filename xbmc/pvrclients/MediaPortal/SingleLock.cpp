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

#include "SingleLock.h"

/*
 * \brief CSingleLock constructor
 * \param cs
 */
CSingleLock::CSingleLock(CCriticalSection& cs): m_CriticalSection(cs)
{
  m_CriticalSection.lock();
}

CSingleLock::~CSingleLock()
{
  m_CriticalSection.unlock();
}
