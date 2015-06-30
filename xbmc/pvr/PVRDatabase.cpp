/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "PVRDatabase.h"
#include "dbwrappers/dataset.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "utils/log.h"
#include "utils/StringUtils.h"

#include "PVRManager.h"
#include "channels/PVRChannelGroupsContainer.h"
#include "channels/PVRChannelGroupInternal.h"
#include "addons/PVRClient.h"

using namespace dbiplus;
using namespace PVR;
using namespace ADDON;

#define PVRDB_DEBUGGING 0

bool CPVRDatabase::Open()
{
  return CDatabase::Open(g_advancedSettings.m_databaseTV);
}

void CPVRDatabase::CreateTables()
{
  CLog::Log(LOGINFO, "PVR - %s - creating tables", __FUNCTION__);

  CLog::Log(LOGDEBUG, "PVR - %s - creating table 'channels'", __FUNCTION__);
  m_pDS->exec(
      "CREATE TABLE channels ("
        "idChannel            integer primary key, "
        "iUniqueId            integer, "
        "bIsRadio             bool, "
        "bIsHidden            bool, "
        "bIsUserSetIcon       bool, "
        "bIsUserSetName       bool, "
        "bIsLocked            bool, "
        "sIconPath            varchar(255), "
        "sChannelName         varchar(64), "
        "bIsVirtual           bool, "
        "bEPGEnabled          bool, "
        "sEPGScraper          varchar(32), "
        "iLastWatched         integer,"

        // TODO use mapping table
        "iClientId            integer, "

        "idEpg                integer"
      ")"
  );

  CLog::Log(LOGDEBUG, "PVR - %s - creating table 'channelgroups'", __FUNCTION__);
  m_pDS->exec(
      "CREATE TABLE channelgroups ("
        "idGroup         integer primary key,"
        "bIsRadio        bool, "
        "iGroupType      integer, "
        "sName           varchar(64), "
        "iLastWatched    integer, "
        "bIsHidden       bool, "
        "iPosition       integer"
      ")"
  );

  CLog::Log(LOGDEBUG, "PVR - %s - creating table 'map_channelgroups_channels'", __FUNCTION__);
  m_pDS->exec(
      "CREATE TABLE map_channelgroups_channels ("
        "idChannel         integer, "
        "idGroup           integer, "
        "iChannelNumber    integer, "
        "iSubChannelNumber integer"
      ")"
  );
}

void CPVRDatabase::CreateAnalytics()
{
  CLog::Log(LOGINFO, "%s - creating indices", __FUNCTION__);
  m_pDS->exec("CREATE UNIQUE INDEX idx_channels_iClientId_iUniqueId on channels(iClientId, iUniqueId);");
  m_pDS->exec("CREATE INDEX idx_channelgroups_bIsRadio on channelgroups(bIsRadio);");
  m_pDS->exec("CREATE UNIQUE INDEX idx_idGroup_idChannel on map_channelgroups_channels(idGroup, idChannel);");
}

void CPVRDatabase::UpdateTables(int iVersion)
{
  if (iVersion < 13)
    m_pDS->exec("ALTER TABLE channels ADD idEpg integer;");

  if (iVersion < 20)
    m_pDS->exec("ALTER TABLE channels ADD bIsUserSetIcon bool");

  if (iVersion < 21)
    m_pDS->exec("ALTER TABLE channelgroups ADD iGroupType integer");

  if (iVersion < 22)
    m_pDS->exec("ALTER TABLE channels ADD bIsLocked bool");

  if (iVersion < 23)
    m_pDS->exec("ALTER TABLE channelgroups ADD iLastWatched integer");

  if (iVersion < 24)
    m_pDS->exec("ALTER TABLE channels ADD bIsUserSetName bool");

  if (iVersion < 25)
    m_pDS->exec("DROP TABLE IF EXISTS channelsettings");

  if (iVersion < 26)
  {
    m_pDS->exec("ALTER TABLE channels ADD iClientSubChannelNumber integer");
    m_pDS->exec("UPDATE channels SET iClientSubChannelNumber = 0");
    m_pDS->exec("ALTER TABLE map_channelgroups_channels ADD iSubChannelNumber integer");
    m_pDS->exec("UPDATE map_channelgroups_channels SET iSubChannelNumber = 0");
  }

  if (iVersion < 27)
    m_pDS->exec("ALTER TABLE channelgroups ADD bIsHidden bool");

  if (iVersion < 28)
  {
    VECADDONS addons;
    CAddonDatabase database;
    if (database.Open() && CAddonMgr::Get().GetAddons(ADDON_PVRDLL, addons, true))
    {
      /** find all old client IDs */
      std::string strQuery(PrepareSQL("SELECT idClient, sUid FROM clients"));
      m_pDS->query(strQuery);
      while (!m_pDS->eof() && !addons.empty())
      {
        /** try to find an add-on that matches the sUid */
        for (VECADDONS::iterator it = addons.begin(); it != addons.end(); ++it)
        {
          if ((*it)->ID() == m_pDS->fv(1).get_asString())
          {
            /** try to get the current ID from the database */
            int iAddonId = database.GetAddonId(*it);
            /** register a new id if it didn't exist */
            if (iAddonId <= 0)
              iAddonId = database.AddAddon(*it, 0);
            if (iAddonId > 0)
            {
              // this fails when an id becomes the id of one that's being replaced next iteration
              // but since almost everyone only has 1 add-on enabled...
              /** update the iClientId in the channels table */
              strQuery = PrepareSQL("UPDATE channels SET iClientId = %u WHERE iClientId = %u", iAddonId, m_pDS->fv(0).get_asInt());
              m_pDS->exec(strQuery);

              /** no need to check this add-on again */
              it = addons.erase(it);
              break;
            }
          }
        }
        m_pDS->next();
      }
    }
    m_pDS->exec("DROP TABLE clients");
  }

  if (iVersion < 29)
    m_pDS->exec("ALTER TABLE channelgroups ADD iPosition integer");
}

/********** Channel methods **********/

bool CPVRDatabase::DeleteChannels(void)
{
  CLog::Log(LOGDEBUG, "PVR - %s - deleting all channels from the database", __FUNCTION__);
  return DeleteValues("channels");
}

bool CPVRDatabase::DeleteClientChannels(const CPVRClient &client)
{
  /* invalid client Id */
  if (client.GetID() <= 0)
  {
    CLog::Log(LOGERROR, "PVR - %s - invalid client id: %i", __FUNCTION__, client.GetID());
    return false;
  }

  CLog::Log(LOGDEBUG, "PVR - %s - deleting all channels from client '%i' from the database", __FUNCTION__, client.GetID());

  Filter filter;
  filter.AppendWhere(PrepareSQL("iClientId = %u", client.GetID()));

  return DeleteValues("channels", filter);
}

bool CPVRDatabase::Delete(const CPVRChannel &channel)
{
  /* invalid channel */
  if (channel.ChannelID() <= 0)
    return false;

  CLog::Log(LOGDEBUG, "PVR - %s - deleting channel '%s' from the database", __FUNCTION__, channel.ChannelName().c_str());

  Filter filter;
  filter.AppendWhere(PrepareSQL("idChannel = %u", channel.ChannelID()));

  return DeleteValues("channels", filter);
}

int CPVRDatabase::Get(CPVRChannelGroupInternal &results)
{
  int iReturn(0);

  std::string strQuery = PrepareSQL("SELECT channels.idChannel, channels.iUniqueId, channels.bIsRadio, channels.bIsHidden, channels.bIsUserSetIcon, channels.bIsUserSetName, "
      "channels.sIconPath, channels.sChannelName, channels.bIsVirtual, channels.bEPGEnabled, channels.sEPGScraper, channels.iLastWatched, channels.iClientId, channels.bIsLocked, "
      "map_channelgroups_channels.iChannelNumber, channels.idEpg "
      "FROM map_channelgroups_channels "
      "LEFT JOIN channels ON channels.idChannel = map_channelgroups_channels.idChannel "
      "WHERE map_channelgroups_channels.idGroup = %u", results.GroupID());
  if (ResultQuery(strQuery))
  {
    try
    {
      bool bIgnoreEpgDB = CSettings::Get().GetBool("epg.ignoredbforclient");
      while (!m_pDS->eof())
      {
        CPVRChannelPtr channel = CPVRChannelPtr(new CPVRChannel());

        channel->m_iChannelId              = m_pDS->fv("idChannel").get_asInt();
        channel->m_iUniqueId               = m_pDS->fv("iUniqueId").get_asInt();
        channel->m_bIsRadio                = m_pDS->fv("bIsRadio").get_asBool();
        channel->m_bIsHidden               = m_pDS->fv("bIsHidden").get_asBool();
        channel->m_bIsUserSetIcon          = m_pDS->fv("bIsUserSetIcon").get_asBool();
        channel->m_bIsUserSetName          = m_pDS->fv("bIsUserSetName").get_asBool();
        channel->m_bIsLocked               = m_pDS->fv("bIsLocked").get_asBool();
        channel->m_strIconPath             = m_pDS->fv("sIconPath").get_asString();
        channel->m_strChannelName          = m_pDS->fv("sChannelName").get_asString();
        channel->m_bEPGEnabled             = m_pDS->fv("bEPGEnabled").get_asBool();
        channel->m_strEPGScraper           = m_pDS->fv("sEPGScraper").get_asString();
        channel->m_iLastWatched            = (time_t) m_pDS->fv("iLastWatched").get_asInt();
        channel->m_iClientId               = m_pDS->fv("iClientId").get_asInt();
        if (bIgnoreEpgDB)
          channel->m_iEpgId                = -1;
        else
          channel->m_iEpgId                = m_pDS->fv("idEpg").get_asInt();
        channel->UpdateEncryptionName();

#if PVRDB_DEBUGGING
        CLog::Log(LOGDEBUG, "PVR - %s - channel '%s' loaded from the database", __FUNCTION__, channel->m_strChannelName.c_str());
#endif
        PVRChannelGroupMember newMember = { channel, (unsigned int)m_pDS->fv("iChannelNumber").get_asInt() };
        results.m_sortedMembers.push_back(newMember);
        results.m_members.insert(std::make_pair(channel->StorageId(), newMember));

        m_pDS->next();
        ++iReturn;
      }
      m_pDS->close();
    }
    catch (...)
    {
      CLog::Log(LOGERROR, "PVR - %s - couldn't load channels from the database", __FUNCTION__);
    }
  }
  else
  {
    CLog::Log(LOGERROR, "PVR - %s - query failed", __FUNCTION__);
  }

  m_pDS->close();
  return iReturn;
}

/********** Channel group methods **********/

bool CPVRDatabase::RemoveChannelsFromGroup(const CPVRChannelGroup &group)
{
  Filter filter;
  filter.AppendWhere(PrepareSQL("idGroup = %u", group.GroupID()));

  return DeleteValues("map_channelgroups_channels", filter);
}

bool CPVRDatabase::GetCurrentGroupMembers(const CPVRChannelGroup &group, std::vector<int> &members)
{
  bool bReturn(false);
  /* invalid group id */
  if (group.GroupID() <= 0)
  {
    CLog::Log(LOGERROR, "PVR - %s - invalid group id: %d", __FUNCTION__, group.GroupID());
    return false;
  }

  std::string strCurrentMembersQuery = PrepareSQL("SELECT idChannel FROM map_channelgroups_channels WHERE idGroup = %u", group.GroupID());
  if (ResultQuery(strCurrentMembersQuery))
  {
    try
    {
      while (!m_pDS->eof())
      {
        members.push_back(m_pDS->fv("idChannel").get_asInt());
        m_pDS->next();
      }
      m_pDS->close();
      bReturn = true;
    }
    catch (...)
    {
      CLog::Log(LOGERROR, "PVR - %s - couldn't load channels from the database", __FUNCTION__);
    }
  }
  else
  {
    CLog::Log(LOGERROR, "PVR - %s - query failed", __FUNCTION__);
  }

  return bReturn;
}

bool CPVRDatabase::DeleteChannelsFromGroup(const CPVRChannelGroup &group)
{
  /* invalid group id */
  if (group.GroupID() <= 0)
  {
    CLog::Log(LOGERROR, "PVR - %s - invalid group id: %d", __FUNCTION__, group.GroupID());
    return false;
  }

  Filter filter;
  filter.AppendWhere(PrepareSQL("idGroup = %u", group.GroupID()));

  return DeleteValues("map_channelgroups_channels", filter);
}

bool CPVRDatabase::DeleteChannelsFromGroup(const CPVRChannelGroup &group, const std::vector<int> &channelsToDelete)
{
  bool bDelete(true);
  unsigned int iDeletedChannels(0);
  /* invalid group id */
  if (group.GroupID() <= 0)
  {
    CLog::Log(LOGERROR, "PVR - %s - invalid group id: %d", __FUNCTION__, group.GroupID());
    return false;
  }

  while (iDeletedChannels < channelsToDelete.size())
  {
    std::string strChannelsToDelete;

    for (unsigned int iChannelPtr = 0; iChannelPtr + iDeletedChannels < channelsToDelete.size() && iChannelPtr < 50; iChannelPtr++)
      strChannelsToDelete += StringUtils::Format(", %d", channelsToDelete.at(iDeletedChannels + iChannelPtr));

    if (!strChannelsToDelete.empty())
    {
      strChannelsToDelete.erase(0, 2);

      Filter filter;
      filter.AppendWhere(PrepareSQL("idGroup = %u", group.GroupID()));
      filter.AppendWhere(PrepareSQL("idChannel IN (%s)", strChannelsToDelete.c_str()));

      bDelete = DeleteValues("map_channelgroups_channels", filter) && bDelete;
    }

    iDeletedChannels += 50;
  }

  return bDelete;
}

bool CPVRDatabase::RemoveStaleChannelsFromGroup(const CPVRChannelGroup &group)
{
  bool bDelete(true);
  /* invalid group id */
  if (group.GroupID() <= 0)
  {
    CLog::Log(LOGERROR, "PVR - %s - invalid group id: %d", __FUNCTION__, group.GroupID());
    return false;
  }

  if (!group.IsInternalGroup())
  {
    /* First remove channels that don't exist in the main channels table */

    // XXX work around for frodo: fix this up so it uses one query for all db types
    // mysql doesn't support subqueries when deleting and sqlite doesn't support joins when deleting
    if (StringUtils::EqualsNoCase(g_advancedSettings.m_databaseTV.type, "mysql"))
    {
      std::string strQuery = PrepareSQL("DELETE m FROM map_channelgroups_channels m LEFT JOIN channels c ON (c.idChannel = m.idChannel) WHERE c.idChannel IS NULL");
      bDelete = ExecuteQuery(strQuery);
    }
    else
    {
      Filter filter;
      filter.AppendWhere("idChannel IN (SELECT m.idChannel FROM map_channelgroups_channels m LEFT JOIN channels on m.idChannel = channels.idChannel WHERE channels.idChannel IS NULL)");

      bDelete = DeleteValues("map_channelgroups_channels", filter);
    }
  }

  if (group.HasChannels())
  {
    std::vector<int> currentMembers;
    if (GetCurrentGroupMembers(group, currentMembers))
    {
      std::vector<int> channelsToDelete;
      for (unsigned int iChannelPtr = 0; iChannelPtr < currentMembers.size(); iChannelPtr++)
      {
        if (!group.IsGroupMember(currentMembers.at(iChannelPtr)))
          channelsToDelete.push_back(currentMembers.at(iChannelPtr));
      }

      bDelete = DeleteChannelsFromGroup(group, channelsToDelete) && bDelete;
    }
  }
  else
  {
    Filter filter;
    filter.AppendWhere(PrepareSQL("idGroup = %u", group.GroupID()));

    bDelete = DeleteValues("map_channelgroups_channels", filter) && bDelete;
  }

  return bDelete;
}

bool CPVRDatabase::DeleteChannelGroups(void)
{
  CLog::Log(LOGDEBUG, "PVR - %s - deleting all channel groups from the database", __FUNCTION__);

  return DeleteValues("channelgroups") &&
      DeleteValues("map_channelgroups_channels");
}

bool CPVRDatabase::Delete(const CPVRChannelGroup &group)
{
  /* invalid group id */
  if (group.GroupID() <= 0)
  {
    CLog::Log(LOGERROR, "PVR - %s - invalid group id: %d", __FUNCTION__, group.GroupID());
    return false;
  }

  Filter filter;
  filter.AppendWhere(PrepareSQL("idGroup = %u", group.GroupID()));
  filter.AppendWhere(PrepareSQL("bIsRadio = %u", group.IsRadio()));

  return RemoveChannelsFromGroup(group) &&
      DeleteValues("channelgroups", filter);
}

bool CPVRDatabase::Get(CPVRChannelGroups &results)
{
  bool bReturn = false;
  std::string strQuery = PrepareSQL("SELECT * from channelgroups WHERE bIsRadio = %u", results.IsRadio());

  if (ResultQuery(strQuery))
  {
    try
    {
      while (!m_pDS->eof())
      {
        CPVRChannelGroup data(m_pDS->fv("bIsRadio").get_asBool(), m_pDS->fv("idGroup").get_asInt(), m_pDS->fv("sName").get_asString());
        data.SetGroupType(m_pDS->fv("iGroupType").get_asInt());
        data.SetLastWatched((time_t) m_pDS->fv("iLastWatched").get_asInt());
        data.SetHidden(m_pDS->fv("bIsHidden").get_asBool());
        data.SetPosition(m_pDS->fv("iPosition").get_asInt());
        results.Update(data);

        CLog::Log(LOGDEBUG, "PVR - %s - group '%s' loaded from the database", __FUNCTION__, data.GroupName().c_str());
        m_pDS->next();
      }
      m_pDS->close();
      bReturn = true;
    }
    catch (...)
    {
      CLog::Log(LOGERROR, "%s - couldn't load channels from the database", __FUNCTION__);
    }
  }

  return bReturn;
}

int CPVRDatabase::Get(CPVRChannelGroup &group)
{
  int iReturn = -1;

  /* invalid group id */
  if (group.GroupID() < 0)
  {
    CLog::Log(LOGERROR, "PVR - %s - invalid group id: %d", __FUNCTION__, group.GroupID());
    return -1;
  }

  std::string strQuery = PrepareSQL("SELECT idChannel, iChannelNumber FROM map_channelgroups_channels WHERE idGroup = %u ORDER BY iChannelNumber", group.GroupID());
  if (ResultQuery(strQuery))
  {
    iReturn = 0;

    try
    {
      while (!m_pDS->eof())
      {
        int iChannelId = m_pDS->fv("idChannel").get_asInt();
        int iChannelNumber = m_pDS->fv("iChannelNumber").get_asInt();
        CPVRChannelPtr channel = g_PVRChannelGroups->GetGroupAll(group.IsRadio())->GetByChannelID(iChannelId);

        if (channel)
        {
#if PVRDB_DEBUGGING
          CLog::Log(LOGDEBUG, "PVR - %s - channel '%s' loaded from the database", __FUNCTION__, channel->m_strChannelName.c_str());
#endif
          PVRChannelGroupMember newMember = { channel, (unsigned int)iChannelNumber };
          group.m_sortedMembers.push_back(newMember);
          group.m_members.insert(std::make_pair(channel->StorageId(), newMember));
          iReturn++;
        }
        else
        {
          // remove a channel that doesn't exist (anymore) from the table
          Filter filter;
          filter.AppendWhere(PrepareSQL("idGroup = %u", group.GroupID()));
          filter.AppendWhere(PrepareSQL("idChannel = %u", iChannelId));

          DeleteValues("map_channelgroups_channels", filter);
        }

        m_pDS->next();
      }
      m_pDS->close();
    }
    catch(...)
    {
      CLog::Log(LOGERROR, "PVR - %s - failed to get channels", __FUNCTION__);
    }
  }

  if (iReturn > 0)
    group.SortByChannelNumber();

  return iReturn;
}

bool CPVRDatabase::PersistChannels(CPVRChannelGroup &group)
{
  bool bReturn(true);

  for (PVR_CHANNEL_GROUP_MEMBERS::iterator it = group.m_members.begin(); it != group.m_members.end(); ++it)
  {
    if (it->second.channel->IsChanged() || it->second.channel->IsNew())
      bReturn &= Persist(*it->second.channel);
  }

  bReturn &= CommitInsertQueries();

  if (bReturn)
  {
    std::string strQuery;
    std::string strValue;
    for (PVR_CHANNEL_GROUP_MEMBERS::iterator it = group.m_members.begin(); it != group.m_members.end(); ++it)
    {
      strQuery = PrepareSQL("iUniqueId = %u AND iClientId = %u", it->second.channel->UniqueID(), it->second.channel->ClientID());
      strValue = GetSingleValue("channels", "idChannel", strQuery);
      if (!strValue.empty() && StringUtils::IsInteger(strValue))
        it->second.channel->SetChannelID(atoi(strValue.c_str()));
    }
  }

  return bReturn;
}

bool CPVRDatabase::PersistGroupMembers(const CPVRChannelGroup &group)
{
  bool bReturn = true;
  bool bRemoveChannels = true;
  std::string strQuery;
  CSingleLock lock(group.m_critSection);

  if (group.HasChannels())
  {
    for (PVR_CHANNEL_GROUP_SORTED_MEMBERS::const_iterator it = group.m_sortedMembers.begin(); it != group.m_sortedMembers.end(); ++it)
    {
      std::string strWhereClause = PrepareSQL("idChannel = %u AND idGroup = %u AND iChannelNumber = %u AND iSubChannelNumber = %u",
          (*it).channel->ChannelID(), group.GroupID(), (*it).iChannelNumber, (*it).iSubChannelNumber);

      std::string strValue = GetSingleValue("map_channelgroups_channels", "idChannel", strWhereClause);
      if (strValue.empty())
      {
        strQuery = PrepareSQL("REPLACE INTO map_channelgroups_channels ("
            "idGroup, idChannel, iChannelNumber, iSubChannelNumber) "
            "VALUES (%i, %i, %i, %i);",
            group.GroupID(), (*it).channel->ChannelID(), (*it).iChannelNumber, (*it).iSubChannelNumber);
        QueueInsertQuery(strQuery);
      }
    }
    lock.Leave();

    bReturn = CommitInsertQueries();
    bRemoveChannels = RemoveStaleChannelsFromGroup(group);
  }

  return bReturn && bRemoveChannels;
}

/********** Client methods **********/

bool CPVRDatabase::ResetEPG(void)
{
  std::string strQuery = PrepareSQL("UPDATE channels SET idEpg = 0");
  return ExecuteQuery(strQuery);
}

bool CPVRDatabase::Persist(CPVRChannelGroup &group)
{
  bool bReturn(false);
  if (group.GroupName().empty())
  {
    CLog::Log(LOGERROR, "%s - empty group name", __FUNCTION__);
    return bReturn;
  }

  std::string strQuery;
  bReturn = true;
  {
    CSingleLock lock(group.m_critSection);

    /* insert a new entry when this is a new group, or replace the existing one otherwise */
    if (group.GroupID() <= 0)
      strQuery = PrepareSQL("INSERT INTO channelgroups (bIsRadio, iGroupType, sName, iLastWatched, bIsHidden, iPosition) VALUES (%i, %i, '%s', %u, %i, %i)",
          (group.IsRadio() ? 1 :0), group.GroupType(), group.GroupName().c_str(), group.LastWatched(), group.IsHidden(), group.GetPosition());
    else
      strQuery = PrepareSQL("REPLACE INTO channelgroups (idGroup, bIsRadio, iGroupType, sName, iLastWatched, bIsHidden, iPosition) VALUES (%i, %i, %i, '%s', %u, %i, %i)",
          group.GroupID(), (group.IsRadio() ? 1 :0), group.GroupType(), group.GroupName().c_str(), group.LastWatched(), group.IsHidden(), group.GetPosition());

    bReturn = ExecuteQuery(strQuery);

    /* set the group id if it was <= 0 */
    if (bReturn && group.GroupID() <= 0)
      group.m_iGroupId = (int) m_pDS->lastinsertid();
  }

  /* only persist the channel data for the internal groups */
  if (group.IsInternalGroup())
    bReturn &= PersistChannels(group);

  /* persist the group member entries */
  if (bReturn)
    bReturn = PersistGroupMembers(group);

  return bReturn;
}

bool CPVRDatabase::Persist(CPVRChannel &channel)
{
  bool bReturn(false);

  /* invalid channel */
  if (channel.UniqueID() <= 0)
  {
    CLog::Log(LOGERROR, "PVR - %s - invalid channel uid: %d", __FUNCTION__, channel.UniqueID());
    return bReturn;
  }

  std::string strQuery;
  if (channel.ChannelID() <= 0)
  {
    /* new channel */
    strQuery = PrepareSQL("INSERT INTO channels ("
        "iUniqueId, bIsRadio, bIsHidden, bIsUserSetIcon, bIsUserSetName, bIsLocked, "
        "sIconPath, sChannelName, bIsVirtual, bEPGEnabled, sEPGScraper, iLastWatched, iClientId, "
        "idEpg) "
        "VALUES (%i, %i, %i, %i, %i, %i, '%s', '%s', %i, %i, '%s', %u, %i, %i)",
        channel.UniqueID(), (channel.IsRadio() ? 1 :0), (channel.IsHidden() ? 1 : 0), (channel.IsUserSetIcon() ? 1 : 0), (channel.IsUserSetName() ? 1 : 0), (channel.IsLocked() ? 1 : 0),
        channel.IconPath().c_str(), channel.ChannelName().c_str(), 0, (channel.EPGEnabled() ? 1 : 0), channel.EPGScraper().c_str(), channel.LastWatched(), channel.ClientID(),
        channel.EpgID());
  }
  else
  {
    /* update channel */
    strQuery = PrepareSQL("REPLACE INTO channels ("
        "iUniqueId, bIsRadio, bIsHidden, bIsUserSetIcon, bIsUserSetName, bIsLocked, "
        "sIconPath, sChannelName, bIsVirtual, bEPGEnabled, sEPGScraper, iLastWatched, iClientId, "
        "idChannel, idEpg) "
        "VALUES (%i, %i, %i, %i, %i, %i, '%s', '%s', %i, %i, '%s', %u, %i, %i, %i)",
        channel.UniqueID(), (channel.IsRadio() ? 1 :0), (channel.IsHidden() ? 1 : 0), (channel.IsUserSetIcon() ? 1 : 0), (channel.IsUserSetName() ? 1 : 0), (channel.IsLocked() ? 1 : 0),
        channel.IconPath().c_str(), channel.ChannelName().c_str(), 0, (channel.EPGEnabled() ? 1 : 0), channel.EPGScraper().c_str(), channel.LastWatched(), channel.ClientID(),
        channel.ChannelID(),
        channel.EpgID());
  }

  if (QueueInsertQuery(strQuery))
  {
    /* update the channel ID for new channels */
    if (channel.ChannelID() <= 0)
      channel.SetChannelID((int)m_pDS->lastinsertid());

    bReturn = true;
  }

  return bReturn;
}

bool CPVRDatabase::UpdateLastWatched(const CPVRChannel &channel)
{
  std::string strQuery = PrepareSQL("UPDATE channels SET iLastWatched = %d WHERE idChannel = %d",
    channel.LastWatched(), channel.ChannelID());

  return ExecuteQuery(strQuery);
}

bool CPVRDatabase::UpdateLastWatched(const CPVRChannelGroup &group)
{
  std::string strQuery = PrepareSQL("UPDATE channelgroups SET iLastWatched = %d WHERE idGroup = %d",
    group.LastWatched(), group.GroupID());

  return ExecuteQuery(strQuery);
}
