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
#include "utils/TVEPGInfoTag.h"
#include "utils/TVChannelInfoTag.h"

class CTVDatabase : public CDatabase
{
public:
  CTVDatabase(void);
  virtual ~CTVDatabase(void);

  virtual bool CommitTransaction();

  long AddClient(const CStdString &client);
  int GetLastChannel(DWORD clientID);
  bool UpdateLastChannel(DWORD clientID, unsigned int channelID, CStdString m_strChannel);

  /* Database Channel handling */
  long AddChannel(DWORD clientID, const CTVChannelInfoTag &info);
  bool RemoveAllChannels(DWORD clientID);
  bool RemoveChannel(DWORD clientID, const CTVChannelInfoTag &info);
  long UpdateChannel(DWORD clientID, const CTVChannelInfoTag &info);
  int  GetNumChannels(DWORD clientID);
  int  GetNumHiddenChannels(DWORD clientID);
  bool HasChannel(DWORD clientID, const CTVChannelInfoTag &info);
  bool GetChannelList(DWORD clientID, VECCHANNELS &results, bool radio);
  bool GetChannelSettings(DWORD clientID, unsigned int channelID, CVideoSettings &settings);
  bool SetChannelSettings(DWORD clientID, unsigned int channelID, const CVideoSettings &settings);

  /* Database Channel Group handling */
  long AddGroup(DWORD clientID, const CStdString &groupname);
  bool DeleteGroup(DWORD clientID, unsigned int groupID);
  bool RenameGroup(DWORD clientID, unsigned int GroupId, const CStdString &newname);
  bool GetGroupList(DWORD clientID, CHANNELGROUPS_DATA* results);

protected:
  long GetClientId(const CStdString &client);
  long GetGroupId(const CStdString &groupname);

private:
  virtual bool CreateTables();
};
