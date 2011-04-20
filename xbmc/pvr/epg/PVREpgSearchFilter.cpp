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

#include "guilib/LocalizeStrings.h"
#include "utils/TextSearch.h"

#include "PVREpgSearchFilter.h"
#include "pvr/PVRManager.h"
#include "PVREpgContainer.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"

using namespace std;
using namespace PVR;

void PVREpgSearchFilter::Reset()
{
  EpgSearchFilter::Reset();
  m_iChannelNumber           = -1;
  m_bFTAOnly                 = false;
  m_iChannelGroup            = -1;
  m_bIgnorePresentTimers     = true;
  m_bIgnorePresentRecordings = true;
}

bool PVREpgSearchFilter::FilterEntry(const CPVREpgInfoTag &tag) const
{
  bool bReturn = EpgSearchFilter::FilterEntry(tag);

  if (bReturn)
  {
    if (m_iChannelNumber != -1)
    {
      if (m_iChannelNumber == -2)
      {
        if (tag.ChannelTag()->IsRadio())
          bReturn = false;
      }
      else if (m_iChannelNumber == -3)
      {
        if (!tag.ChannelTag()->IsRadio())
          bReturn = false;
      }
      else if (tag.ChannelTag()->ChannelNumber() != m_iChannelNumber)
        bReturn = false;
    }
    if (m_bFTAOnly && tag.ChannelTag()->IsEncrypted())
    {
      bReturn = false;
    }
    if (m_iChannelGroup != -1)
    {
      const CPVRChannelGroup *group = g_PVRChannelGroups->GetById(tag.ChannelTag()->IsRadio(), m_iChannelGroup);
      if (!group || !group->IsGroupMember(tag.ChannelTag()))
        bReturn = false;
    }
  }

  return bReturn;
}
