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

#include "BroadcastManager.h"
#include <stdio.h>
#include "log.h"

using namespace std;
using namespace BROADCAST;

vector<IBroadcastListener *> CBroadcastManager::m_listeners;

void CBroadcastManager::AddListener(IBroadcastListener *listener)
{
  m_listeners.push_back(listener);
}

void CBroadcastManager::RemoveListener(IBroadcastListener *listener)
{
  for (unsigned int i = 0; i < m_listeners.size(); i++)
  {
    if (m_listeners[i] == listener)
    {
      m_listeners.erase(m_listeners.begin() + i);
      printf("found and removed listener\n");
      return;
    }
  }
}

void CBroadcastManager::Broadcast(EBroadcastFlag flag, string message)
{
  CLog::Log(LOGDEBUG, "BroadcastManager - Broadcast: %s", message.c_str());
  printf("BroadcastManager - Broadcast: %s\n", message.c_str());
  for (unsigned int i = 0; i < m_listeners.size(); i++)
    m_listeners[i]->Broadcast(flag, message);
}
