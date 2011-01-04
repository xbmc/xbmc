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
#include "PVRChannels.h"

CPVRChannelsContainer::CPVRChannelsContainer()
{
  CPVRChannels *channelsTV = new CPVRChannels(false);
  push_back(channelsTV);

  CPVRChannels *channelsRadio = new CPVRChannels(true);
  push_back(channelsRadio);
}

CPVRChannelsContainer::~CPVRChannelsContainer()
{
  erase(begin(), end());
}

const CPVRChannels *CPVRChannelsContainer::Get(int iChannelsId)
{
  if (iChannelsId < 0 || (unsigned int) iChannelsId > size() - 1)
    return NULL;

  return at(iChannelsId);
}

const CPVRChannels *CPVRChannelsContainer::Get(bool bRadio)
{
  return bRadio ? g_PVRChannels.Get(RADIO) : g_PVRChannels.Get(TV);
}

const CPVRChannels *CPVRChannelsContainer::GetTV()
{
  return g_PVRChannels.Get(TV);
}

const CPVRChannels *CPVRChannelsContainer::GetRadio()
{
  return g_PVRChannels.Get(RADIO);
}

CPVRChannelsContainer g_PVRChannels;
