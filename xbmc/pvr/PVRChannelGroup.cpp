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

#include "PVRChannelGroup.h"
#include "PVRChannel.h"

CPVRChannelGroup::CPVRChannelGroup(bool bRadio)
{
  m_iGroupID  = 0;
  m_GroupName = "";
  m_bRadio    = false;
}

CPVRChannelGroup::~CPVRChannelGroup(void)
{
  erase(begin(), end());
}

bool CPVRChannelGroup::RemoveFromGroup(const CPVRChannel *channel)
{
  bool bReturn = false;

  for (unsigned int iChannelPtr = 0; iChannelPtr < size(); iChannelPtr++)
  {
    if (*channel == *at(iChannelPtr))
    {
      erase(begin() + iChannelPtr);
      bReturn = true;
      break;
    }
  }

  return bReturn;
}

bool CPVRChannelGroup::AddToGroup(CPVRChannel *channel)
{
  bool bReturn = false;

  if (!IsGroupMember(channel))
  {
    push_back(channel);
    bReturn = true;
  }

  return bReturn;
}

bool CPVRChannelGroup::IsGroupMember(const CPVRChannel *channel)
{
  bool bReturn = false;

  for (unsigned int iChannelPtr = 0; iChannelPtr < size(); iChannelPtr++)
  {
    if (*channel == *at(iChannelPtr))
    {
      bReturn = true;
      break;
    }
  }

  return bReturn;
}
