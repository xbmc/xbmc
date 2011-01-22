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

#include "PVRChannelGroup.h"
#include "../addons/include/xbmc_pvr_types.h"

class CPVRChannelGroups : public std::vector<CPVRChannelGroup>
{
private:
  bool  m_bRadio;

  int GetIndexForGroupID(int iGroupId);

public:
  CPVRChannelGroups(bool bRadio);
  virtual ~CPVRChannelGroups(void);

  bool Load(void);
  void Unload(void);

  CPVRChannelGroup *GetGroupAll(void);

  int GetGroupList(CFileItemList* results);
  CPVRChannelGroup *GetGroupById(int iGroupId);
  int GetFirstChannelForGroupID(int iGroupId);
  int GetPreviousGroupID(int iGroupId);
  int GetNextGroupID(int current_group_id);

  void AddGroup(const CStdString &name);
  bool RenameGroup(int GroupId, const CStdString &newname);
  bool DeleteGroup(int GroupId);
  bool ChannelToGroup(const CPVRChannel &channel, int GroupId);
  CStdString GetGroupName(int GroupId);
  int GetGroupId(CStdString GroupName);
};
