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

#include "PVRChannelGroups.h"

class CPVRChannelGroupsContainer
{
private:
  CPVRChannelGroups *m_groupsRadio;
  CPVRChannelGroups *m_groupsTV;

public:
  CPVRChannelGroupsContainer(void);
  ~CPVRChannelGroupsContainer(void);

  bool Load(void);
  void Unload(void);

  CPVRChannelGroups *GetTV(void) { return Get(false); }
  CPVRChannelGroups *GetRadio(void) { return Get(true); }
  CPVRChannelGroups *Get(bool bRadio);

  CPVRChannelGroup *GetGroupAllTV(void) { return GetGroupAll(false); }
  CPVRChannelGroup *GetGroupAllRadio(void) { return GetGroupAll(true); }
  CPVRChannelGroup *GetGroupAll(bool bRadio);
};

extern CPVRChannelGroupsContainer g_PVRChannelGroups;
