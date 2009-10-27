#pragma once
/*
 *      Copyright (C) 2005-2008 Team XBMC
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

#include "Database.h"
#include "DateTime.h"
#include "FileItem.h"
#include "settings/VideoSettings.h"
#include "utils/PVREpg.h"
#include "utils/PVRChannels.h"

class CTVDatabase : public CDatabase
{
public:
  CTVDatabase(void);
  virtual ~CTVDatabase(void);

  virtual bool CommitTransaction();

  long AddClient(const CStdString &client, const CStdString &guid);
  int GetLastChannel(DWORD clientID);
  bool UpdateLastChannel(DWORD clientID, unsigned int channelID, CStdString m_strChannel);

  /* Database Channel handling */
  long AddDBChannel(const cPVRChannelInfoTag &info);
  bool RemoveAllChannels(DWORD clientID);
  bool RemoveDBChannel(const cPVRChannelInfoTag &info);
  long UpdateDBChannel(const cPVRChannelInfoTag &info);
  int  GetDBNumChannels(bool radio);
  int  GetNumHiddenChannels();
  bool HasChannel(DWORD clientID, const cPVRChannelInfoTag &info);
  bool GetDBChannelList(cPVRChannels &results, bool radio);
  bool GetChannelSettings(DWORD clientID, unsigned int channelID, CVideoSettings &settings);
  bool SetChannelSettings(DWORD clientID, unsigned int channelID, const CVideoSettings &settings);

  /* Database Channel Group handling */
  long AddGroup(const CStdString &groupName);
  bool DeleteGroup(unsigned int groupID);
  bool RenameGroup(unsigned int groupID, const CStdString &newname);
  bool GetGroupList(cPVRChannelGroups &results);

protected:
  long GetClientId(const CStdString &guid);
  long GetGroupId(const CStdString &groupname);

private:
  virtual bool CreateTables();
};
