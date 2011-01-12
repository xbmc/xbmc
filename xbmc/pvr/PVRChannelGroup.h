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

#include "VideoInfoTag.h"
#include "DateTime.h"
#include "FileItem.h"
#include "../addons/include/xbmc_pvr_types.h"

class CPVRChannelGroup : public std::vector<CPVRChannel *>
{
private:
  unsigned long m_iGroupID;
  CStdString    m_GroupName;
  int           m_iSortOrder;
  bool          m_bRadio;

public:
  CPVRChannelGroup(bool bRadio);
  virtual ~CPVRChannelGroup(void);

  bool RemoveFromGroup(const CPVRChannel *channel);
  bool AddToGroup(CPVRChannel *channel);
  bool IsGroupMember(const CPVRChannel *channel);

  /**
   * Get a channel given it's channel number.
   */
  CPVRChannel *GetByChannelNumber(int iChannelNumber); // TODO
  CPVRChannel *GetByChannelNumberUp(int iChannelNumber); // TODO
  CPVRChannel *GetByChannelNumberDown(int iChannelNumber); // TODO

  long GroupID(void) const { return m_iGroupID; }
  void SetGroupID(long group) { m_iGroupID = group; }
  CStdString GroupName(void) const { return m_GroupName; }
  void SetGroupName(CStdString name) { m_GroupName = name; }
  long SortOrder(void) const { return m_iSortOrder; }
  void SetSortOrder(long sortorder) { m_iSortOrder = sortorder; }
};
