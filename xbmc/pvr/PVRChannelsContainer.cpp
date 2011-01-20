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

#include "PVRChannelsContainer.h"
#include "PVRChannelGroupInternal.h"

CPVRChannelsContainer::CPVRChannelsContainer()
{
  m_currentPlayingChannel = NULL;
  InitializeCriticalSection(&m_criticalSection);

  CPVRChannelGroup *channelsTV = new CPVRChannelGroupInternal(false);
  push_back(channelsTV);

  CPVRChannelGroup *channelsRadio = new CPVRChannelGroupInternal(true);
  push_back(channelsRadio);
}

CPVRChannelsContainer::~CPVRChannelsContainer()
{
  erase(begin(), end());
  DeleteCriticalSection(&m_criticalSection);
}

const CPVRChannelGroup *CPVRChannelsContainer::Get(int iChannelsId)
{
  if (iChannelsId < 0 || (unsigned int) iChannelsId > size() - 1)
    return NULL;

  return at(iChannelsId);
}

const CPVRChannelGroup *CPVRChannelsContainer::Get(bool bRadio)
{
  return bRadio ? g_PVRChannels.Get(RADIO) : g_PVRChannels.Get(TV);
}

int CPVRChannelsContainer::Load(void)
{
  return ((CPVRChannelGroup *)GetTV())->Load() +
      ((CPVRChannelGroup *)GetRadio())->Load();
}

const CPVRChannelGroup *CPVRChannelsContainer::GetTV()
{
  return g_PVRChannels.Get(TV);
}

const CPVRChannelGroup *CPVRChannelsContainer::GetRadio()
{
  return g_PVRChannels.Get(RADIO);
}

CPVRChannelsContainer g_PVRChannels;
