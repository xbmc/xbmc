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
#include "PVRChannel.h"
#include "../addons/include/xbmc_pvr_types.h"

class CPVRChannelGroups;
class CPVREpg;

class CPVRChannels : public std::vector<CPVRChannel>
{
private:
  bool m_bRadio;          /* true if this container holds radio channels, false if it holds TV channels */
  int  m_iHiddenChannels; /* the amount of hidden channels in this container */

public:
  CPVRChannels(bool bRadio);

  bool Load();
  void Unload();
  bool Update();
  void ReNumberAndCheck(void);
  void SearchAndSetChannelIcons(bool writeDB = false);
  int GetNumChannels() const { return size(); }
  int GetNumHiddenChannels() const { return m_iHiddenChannels; }
  int GetChannels(CFileItemList* results, int group_id = -1);
  int GetHiddenChannels(CFileItemList* results);
  void MoveChannel(unsigned int oldindex, unsigned int newindex);
  void HideChannel(unsigned int number);
  CPVRChannel *GetByNumber(int Number);
  CPVRChannel *GetByClient(int Number, int ClientID);
  CPVRChannel *GetByChannelID(long ChannelID);
  CPVRChannel *GetByUniqueID(long UniqueID);
  CPVRChannel *GetByIndex(unsigned int index);
  CStdString GetNameForChannel(unsigned int Number);
  CStdString GetChannelIcon(unsigned int Number);
  void SetChannelIcon(unsigned int Number, CStdString Icon);
  void ResetChannelEPGLinks();
  void Clear();

  static int GetNumChannelsFromAll();
  static void SearchMissingChannelIcons();
  static CPVRChannel *GetByClientFromAll(int Number, int ClientID);
  static CPVRChannel *GetByChannelIDFromAll(long ChannelID);
  static CPVRChannel *GetByUniqueIDFromAll(long UniqueID);
  static CPVRChannel *GetByPath(CStdString &path);
  static bool GetDirectory(const CStdString& strPath, CFileItemList &items);
};

extern CPVRChannels      PVRChannelsTV;
extern CPVRChannels      PVRChannelsRadio;

extern CPVRChannelGroups PVRChannelGroupsTV;
extern CPVRChannelGroups PVRChannelGroupsRadio;
