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

#include "Database.h"
#include "DateTime.h"

class CPVRChannelGroup;
class CPVRChannelGroupInternal;
class CPVRChannelsContainer;
class CPVRChannel;
class CPVRChannelGroups;
class CPVREpg;
class CPVREpgInfoTag;
class CVideoSettings;

class CPVRDatabase : public CDatabase
{
public:
  CPVRDatabase(void);
  virtual ~CPVRDatabase(void);

  virtual bool Open();

  virtual int GetMinVersion() const { return 7; };
  const char *GetDefaultDBName() const { return "MyTV4.db"; };

  /********** Channel methods **********/

  /**
   * Remove all channels from the database
   */
  bool EraseChannels();

  /**
   * Remove all channels from a client from the database
   */
  bool EraseClientChannels(long iClientId);

  /**
   * Add or update a channel entry in the database
   */
  long UpdateChannel(const CPVRChannel &channel, bool bQueueWrite = false);

  /**
   * Remove a channel entry from the database
   */
  bool RemoveChannel(const CPVRChannel &channel);

  /**
   * Get the list of channels from the database
   */
  int GetChannels(CPVRChannelGroupInternal &results, bool bIsRadio);

  /**
   * The amount of channels
   */
  int GetChannelCount(bool bRadio, bool bHidden = false);

  /**
   * Get the Id of the channel that was played last
   */
  int GetLastChannel();

  /**
   * Update the channel that was played last
   */
  bool UpdateLastChannel(const CPVRChannel &channel);

  /********** Channel settings methods **********/

  /**
   * Remove all channel settings from the database
   */
  bool EraseChannelSettings();

  /**
   * Get channel settings from the database
   */
  bool GetChannelSettings(const CPVRChannel &channel, CVideoSettings &settings);

  /**
   * Store channel settings in the database
   */
  bool SetChannelSettings(const CPVRChannel &channel, const CVideoSettings &settings);

  /********** Channel group methods **********/

  /**
   * Remove all channel groups from the database
   */
  bool EraseChannelGroups(bool bRadio = false);

  /**
   * Add a channel group to the database
   */
  long AddChannelGroup(const CStdString &strGroupName, int iSortOrder, bool bRadio = false);

  /**
   * Delete a channel group from the database
   */
  bool DeleteChannelGroup(int iGroupId, bool bRadio = false);

  /**
   * Get the channel groups
   */
  bool GetChannelGroupList(CPVRChannelGroups &results);
  bool GetChannelGroupList(CPVRChannelGroups &results, bool bRadio);

  /**
   * Get the group members for a group
   */
  int GetChannelsInGroup(CPVRChannelGroup *group);

  /**
   * Change the name of a channel group
   */
  bool SetChannelGroupName(int iGroupId, const CStdString &strNewName, bool bRadio = false);

  /**
   * Change the sort order of a channel group
   */
  bool SetChannelGroupSortOrder(int iGroupId, int iSortOrder, bool bRadio = false);

protected:
  /**
   * Get the Id of a channel group
   */
  long GetChannelGroupId(const CStdString &strGroupName, bool bRadio = false);

  /********** Client methods **********/
public:
  /**
   * Remove all client information from the database
   */
  bool EraseClients();

  /**
   * Add a client to the database if it's not already in there.
   */
  long AddClient(const CStdString &strClientName, const CStdString &strGuid);

  /**
   * Remove a client from the database
   */
  bool RemoveClient(const CStdString &strGuid);

protected:

  /**
   * Get the database ID of a client
   */
  long GetClientId(const CStdString &strClientUid);

  /********** EPG methods **********/
public:
  /**
   * Remove all EPG information from the database
   */
  bool EraseEpg();

  /**
   * Erases all EPG entries for a channel.
   */
  bool EraseEpgForChannel(const CPVRChannel &channel, const CDateTime &start = NULL, const CDateTime &end = NULL);

  /**
   * Erases all EPG entries older than 1 day.
   */
  bool EraseOldEpgEntries();

  /**
   * Remove a single EPG entry.
   */
  bool RemoveEpgEntry(const CPVREpgInfoTag &tag);

  /**
   * Get all EPG entries for a channel.
   */
  int GetEpgForChannel(CPVREpg *epg, const CDateTime &start = NULL, const CDateTime &end = NULL);

  /**
   * Get the start time of the first entry for a channel.
   * If iChannelId is <= 0, then all entries will be searched.
   */
  CDateTime GetEpgDataStart(long iChannelId = -1);

  /**
   * Get the end time of the last entry for a channel.
   * If iChannelId is <= 0, then all entries will be searched.
   */
  CDateTime GetEpgDataEnd(long iChannelId = -1);

  /**
   * Get the last stored EPG scan time.
   */
  CDateTime GetLastEpgScanTime();

  /**
   * Update the last scan time.
   */
  bool UpdateLastEpgScanTime(void);

  /**
   * Persist an infotag
   */
  bool UpdateEpgEntry(const CPVREpgInfoTag &tag, bool bSingleUpdate = true, bool bLastUpdate = false);

private:
  virtual bool CreateTables();
  virtual bool UpdateOldVersion(int version);
  CDateTime lastScanTime;
};
