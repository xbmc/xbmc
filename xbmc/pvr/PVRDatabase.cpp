/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      http://kodi.tv
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

#include <utility>

#include "ServiceBroker.h"
#include "addons/PVRClient.h"
#include "dbwrappers/dataset.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include "pvr/channels/PVRChannelGroups.h"

using namespace dbiplus;
using namespace PVR;

bool CPVRDatabase::Open()
{
  CSingleLock lock(m_critSection);
  return CDatabase::Open(g_advancedSettings.m_databaseTV);
}

void CPVRDatabase::Close()
{
  CSingleLock lock(m_critSection);
  CDatabase::Close();
}

void CPVRDatabase::CreateTables()
{
  CSingleLock lock(m_critSection);

  CLog::LogF(LOGINFO, "Creating PVR database tables");

  CLog::LogFC(LOGDEBUG, LOGPVR, "Creating table 'channels'");
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
        "iLastWatched         integer, "
        "iClientId            integer, " //! @todo use mapping table
        "idEpg                integer"
      ")"
  );

  CLog::LogFC(LOGDEBUG, LOGPVR, "Creating table 'channelgroups'");
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

  CLog::LogFC(LOGDEBUG, LOGPVR, "Creating table 'map_channelgroups_channels'");
  m_pDS->exec(
      "CREATE TABLE map_channelgroups_channels ("
        "idChannel         integer, "
        "idGroup           integer, "
        "iChannelNumber    integer, "
        "iSubChannelNumber integer"
      ")"
  );

  CLog::LogFC(LOGDEBUG, LOGPVR, "Creating table 'clients'");
  m_pDS->exec(
      "CREATE TABLE clients ("
        "idClient  integer primary key, "
        "iPriority integer"
      ")"
  );
}

void CPVRDatabase::CreateAnalytics()
{
  CSingleLock lock(m_critSection);

  CLog::LogF(LOGINFO, "Creating PVR database indices");
  m_pDS->exec("CREATE INDEX idx_clients_idClient on clients(idClient);");
  m_pDS->exec("CREATE UNIQUE INDEX idx_channels_iClientId_iUniqueId on channels(iClientId, iUniqueId);");
  m_pDS->exec("CREATE INDEX idx_channelgroups_bIsRadio on channelgroups(bIsRadio);");
  m_pDS->exec("CREATE UNIQUE INDEX idx_idGroup_idChannel on map_channelgroups_channels(idGroup, idChannel);");
}

void CPVRDatabase::UpdateTables(int iVersion)
{
  CSingleLock lock(m_critSection);

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
    m_pDS->exec("DROP TABLE clients");

  if (iVersion < 29)
    m_pDS->exec("ALTER TABLE channelgroups ADD iPosition integer");

  if (iVersion < 32)
    m_pDS->exec("CREATE TABLE clients (idClient integer primary key, iPriority integer)");
}

/********** Client methods **********/

bool CPVRDatabase::DeleteClients()
{
  CLog::LogFC(LOGDEBUG, LOGPVR, "Deleting all clients from the database");

  CSingleLock lock(m_critSection);
  return DeleteValues("clients");
}

bool CPVRDatabase::Persist(const CPVRClient &client)
{
  if (client.GetID() == PVR_INVALID_CLIENT_ID)
    return false;

  CLog::LogFC(LOGDEBUG, LOGPVR, "Persisting client '%s' to database", client.ID().c_str());

  CSingleLock lock(m_critSection);

  const std::string strQuery = PrepareSQL("REPLACE INTO clients (idClient, iPriority) VALUES (%i, %i);",
                                          client.GetID(), client.GetPriority());

  return ExecuteQuery(strQuery);
}

bool CPVRDatabase::Delete(const CPVRClient &client)
{
  if (client.GetID() == PVR_INVALID_CLIENT_ID)
    return false;

  CLog::LogFC(LOGDEBUG, LOGPVR, "Deleting client '%s' from the database", client.ID().c_str());

  CSingleLock lock(m_critSection);

  Filter filter;
  filter.AppendWhere(PrepareSQL("idClient = '%i'", client.GetID()));

  return DeleteValues("clients", filter);
}

int CPVRDatabase::GetPriority(const CPVRClient &client)
{
  if (client.GetID() == PVR_INVALID_CLIENT_ID)
    return 0;

  CLog::LogFC(LOGDEBUG, LOGPVR, "Getting priority for client '%s' from the database", client.ID().c_str());

  CSingleLock lock(m_critSection);

  const std::string strWhereClause = PrepareSQL("idClient = '%i'", client.GetID());
  const std::string strValue = GetSingleValue("clients", "iPriority", strWhereClause);

  if (strValue.empty())
    return 0;

  return atoi(strValue.c_str());
}

/********** Channel methods **********/

bool CPVRDatabase::DeleteChannels(void)
{
  CLog::LogFC(LOGDEBUG, LOGPVR, "Deleting all channels from the database");

  CSingleLock lock(m_critSection);
  return DeleteValues("channels");
}

bool CPVRDatabase::Delete(const CPVRChannel &channel)
{
  /* invalid channel */
  if (channel.ChannelID() <= 0)
    return false;

  CLog::LogFC(LOGDEBUG, LOGPVR, "Deleting channel '%s' from the database", channel.ChannelName().c_str());

  Filter filter;
  filter.AppendWhere(PrepareSQL("idChannel = %u", channel.ChannelID()));

  CSingleLock lock(m_critSection);
  return DeleteValues("channels", filter);
}

int CPVRDatabase::Get(CPVRChannelGroup &results, bool bCompressDB)
{
  int iReturn(0);
  bool bIgnoreEpgDB = CServiceBroker::GetSettings().GetBool(CSettings::SETTING_EPG_IGNOREDBFORCLIENT);

  std::string strQuery = PrepareSQL("SELECT channels.idChannel, channels.iUniqueId, channels.bIsRadio, channels.bIsHidden, channels.bIsUserSetIcon, channels.bIsUserSetName, "
      "channels.sIconPath, channels.sChannelName, channels.bIsVirtual, channels.bEPGEnabled, channels.sEPGScraper, channels.iLastWatched, channels.iClientId, channels.bIsLocked, "
      "map_channelgroups_channels.iChannelNumber, map_channelgroups_channels.iSubChannelNumber, channels.idEpg "
      "FROM map_channelgroups_channels "
      "LEFT JOIN channels ON channels.idChannel = map_channelgroups_channels.idChannel "
      "WHERE map_channelgroups_channels.idGroup = %u", results.GroupID());

  CSingleLock lock(m_critSection);
  if (ResultQuery(strQuery))
  {
    try
    {
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
        channel->m_iLastWatched            = static_cast<time_t>(m_pDS->fv("iLastWatched").get_asInt());
        channel->m_iClientId               = m_pDS->fv("iClientId").get_asInt();
        channel->m_iEpgId                  = bIgnoreEpgDB ? -1 : m_pDS->fv("idEpg").get_asInt();
        channel->UpdateEncryptionName();

        PVRChannelGroupMember newMember(channel,
                                        CPVRChannelNumber(static_cast<unsigned int>(m_pDS->fv("iChannelNumber").get_asInt()),
                                                          static_cast<unsigned int>(m_pDS->fv("iSubChannelNumber").get_asInt())),
                                        0);
        results.m_sortedMembers.emplace_back(newMember);
        results.m_members.insert(std::make_pair(channel->StorageId(), newMember));

        m_pDS->next();
        ++iReturn;
      }
      m_pDS->close();
    }
    catch (...)
    {
      CLog::LogF(LOGERROR, "Couldn't load channels from PVR database");
    }
  }
  else
  {
    CLog::LogF(LOGERROR, "PVR database query failed");
  }

  m_pDS->close();

  if (iReturn > 0 && bCompressDB)
    Compress(true);

  return iReturn;
}

/********** Channel group methods **********/

bool CPVRDatabase::RemoveChannelsFromGroup(const CPVRChannelGroup &group)
{
  Filter filter;
  filter.AppendWhere(PrepareSQL("idGroup = %u", group.GroupID()));

  CSingleLock lock(m_critSection);
  return DeleteValues("map_channelgroups_channels", filter);
}

bool CPVRDatabase::GetCurrentGroupMembers(const CPVRChannelGroup &group, std::vector<int> &members)
{
  bool bReturn(false);
  /* invalid group id */
  if (group.GroupID() <= 0)
  {
    CLog::LogF(LOGERROR, "Invalid channel group id: %d", group.GroupID());
    return false;
  }

  CSingleLock lock(m_critSection);

  const std::string strCurrentMembersQuery = PrepareSQL("SELECT idChannel FROM map_channelgroups_channels WHERE idGroup = %u", group.GroupID());
  if (ResultQuery(strCurrentMembersQuery))
  {
    try
    {
      while (!m_pDS->eof())
      {
        members.emplace_back(m_pDS->fv("idChannel").get_asInt());
        m_pDS->next();
      }
      m_pDS->close();
      bReturn = true;
    }
    catch (...)
    {
      CLog::LogF(LOGERROR, "Couldn't load channels from PVR database");
    }
  }
  else
  {
    CLog::LogF(LOGERROR, "PVR database query failed");
  }

  return bReturn;
}

bool CPVRDatabase::DeleteChannelsFromGroup(const CPVRChannelGroup &group, const std::vector<int> &channelsToDelete)
{
  bool bDelete(true);
  unsigned int iDeletedChannels(0);
  /* invalid group id */
  if (group.GroupID() <= 0)
  {
    CLog::LogF(LOGERROR, "Invalid channel group id: %d", group.GroupID());
    return false;
  }

  CSingleLock lock(m_critSection);

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

int CPVRDatabase::GetClientIdByChannelId(int iChannelId)
{
  const std::string strQuery = PrepareSQL("idChannel = %u", iChannelId);
  const std::string strValue = GetSingleValue("channels", "iClientId", strQuery);
  if (!strValue.empty())
    return std::atoi(strValue.c_str());

  return PVR_INVALID_CLIENT_ID;
}

bool CPVRDatabase::RemoveStaleChannelsFromGroup(const CPVRChannelGroup &group)
{
  bool bDelete(true);
  /* invalid group id */
  if (group.GroupID() <= 0)
  {
    CLog::LogF(LOGERROR, "Invalid channel group id: %d", group.GroupID());
    return false;
  }

  CSingleLock lock(m_critSection);

  if (!group.IsInternalGroup())
  {
    /* First remove channels that don't exist in the main channels table */

    // XXX work around for frodo: fix this up so it uses one query for all db types
    // mysql doesn't support subqueries when deleting and sqlite doesn't support joins when deleting
    if (StringUtils::EqualsNoCase(g_advancedSettings.m_databaseTV.type, "mysql"))
    {
      const  std::string strQuery = PrepareSQL("DELETE m FROM map_channelgroups_channels m LEFT JOIN channels c ON (c.idChannel = m.idChannel) WHERE c.idChannel IS NULL");
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
      for (int iChannelId : currentMembers)
      {
        if (!group.IsGroupMember(iChannelId))
        {
          int iClientId = GetClientIdByChannelId(iChannelId);
          if (iClientId == PVR_INVALID_CLIENT_ID || !group.IsMissingChannelsFromClient(iClientId))
          {
            channelsToDelete.emplace_back(iChannelId);
          }
        }
      }

      if (!channelsToDelete.empty())
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
  CLog::LogFC(LOGDEBUG, LOGPVR, "Deleting all channel groups from the database");

  CSingleLock lock(m_critSection);
  return DeleteValues("channelgroups") && DeleteValues("map_channelgroups_channels");
}

bool CPVRDatabase::Delete(const CPVRChannelGroup &group)
{
  /* invalid group id */
  if (group.GroupID() <= 0)
  {
    CLog::LogF(LOGERROR, "Invalid channel group id: %d", group.GroupID());
    return false;
  }

  CSingleLock lock(m_critSection);

  bool bIgnoreChannel = false;
  std::vector<int> currentMembers;
  if (GetCurrentGroupMembers(group, currentMembers))
  {
    for (int iChannelId : currentMembers)
    {
      int iClientId = GetClientIdByChannelId(iChannelId);
      if (iClientId != PVR_INVALID_CLIENT_ID && group.IsMissingChannelGroupMembersFromClient(iClientId))
      {
        bIgnoreChannel = true;
        break;
      }
    }
  }

  if (!bIgnoreChannel)
  {
    Filter filter;
    filter.AppendWhere(PrepareSQL("idGroup = %u", group.GroupID()));
    filter.AppendWhere(PrepareSQL("bIsRadio = %u", group.IsRadio()));

    return RemoveChannelsFromGroup(group) && DeleteValues("channelgroups", filter);
  }

  return true;
}

bool CPVRDatabase::Get(CPVRChannelGroups &results)
{
  bool bReturn = false;
  CSingleLock lock(m_critSection);

  const std::string strQuery = PrepareSQL("SELECT * from channelgroups WHERE bIsRadio = %u", results.IsRadio());
  if (ResultQuery(strQuery))
  {
    try
    {
      while (!m_pDS->eof())
      {
        CPVRChannelGroup data(m_pDS->fv("bIsRadio").get_asBool(), m_pDS->fv("idGroup").get_asInt(), m_pDS->fv("sName").get_asString());
        data.SetGroupType(m_pDS->fv("iGroupType").get_asInt());
        data.SetLastWatched(static_cast<time_t>(m_pDS->fv("iLastWatched").get_asInt()));
        data.SetHidden(m_pDS->fv("bIsHidden").get_asBool());
        data.SetPosition(m_pDS->fv("iPosition").get_asInt());
        results.Update(data);

        CLog::LogFC(LOGDEBUG, LOGPVR, "Group '%s' loaded from PVR database", data.GroupName().c_str());
        m_pDS->next();
      }
      m_pDS->close();
      bReturn = true;
    }
    catch (...)
    {
      CLog::LogF(LOGERROR, "Couldn't load channels from PVR database");
    }
  }

  return bReturn;
}

int CPVRDatabase::Get(CPVRChannelGroup &group, const CPVRChannelGroup &allGroup)
{
  int iReturn = -1;

  /* invalid group id */
  if (group.GroupID() < 0)
  {
    CLog::LogF(LOGERROR, "Invalid channel group id: %d", group.GroupID());
    return -1;
  }

  CSingleLock lock(m_critSection);

  const std::string strQuery = PrepareSQL("SELECT idChannel, iChannelNumber, iSubChannelNumber FROM map_channelgroups_channels "
                                          "WHERE idGroup = %u ORDER BY iChannelNumber", group.GroupID());
  if (ResultQuery(strQuery))
  {
    iReturn = 0;

    // create a map to speedup data lookup
    std::map<int, CPVRChannelPtr> allChannels;
    for (const auto& groupMember : allGroup.GetMembers())
    {
      allChannels.insert(std::make_pair(groupMember.channel->ChannelID(), groupMember.channel));
    }

    try
    {
      while (!m_pDS->eof())
      {
        int iChannelId = m_pDS->fv("idChannel").get_asInt();
        const auto& channel = allChannels.find(iChannelId);

        if (channel != allChannels.end())
        {
          PVRChannelGroupMember newMember(channel->second,
                                          CPVRChannelNumber(static_cast<unsigned int>(m_pDS->fv("iChannelNumber").get_asInt()),
                                                            static_cast<unsigned int>(m_pDS->fv("iSubChannelNumber").get_asInt())),
                                          0);
          group.m_sortedMembers.emplace_back(newMember);
          group.m_members.insert(std::make_pair(channel->second->StorageId(), newMember));
          ++iReturn;
        }
        else
        {
          // remove the channel from the table if it doesn't exist on client (anymore)
          int iClientId = GetClientIdByChannelId(iChannelId);
          if (iClientId == PVR_INVALID_CLIENT_ID || !allGroup.IsMissingChannelsFromClient(iClientId))
          {
            Filter filter;
            filter.AppendWhere(PrepareSQL("idGroup = %u", group.GroupID()));
            filter.AppendWhere(PrepareSQL("idChannel = %u", iChannelId));
            DeleteValues("map_channelgroups_channels", filter);
          }
        }

        m_pDS->next();
      }
      m_pDS->close();
    }
    catch(...)
    {
      CLog::LogF(LOGERROR, "Failed to get channels");
    }
  }

  if (iReturn > 0)
    group.SortByChannelNumber();

  return iReturn;
}

bool CPVRDatabase::PersistChannels(CPVRChannelGroup &group)
{
  bool bReturn(true);

  CPVRChannelPtr channel;
  for (const auto& groupMember : group.m_members)
  {
    channel = groupMember.second.channel;
    if (channel->IsChanged() || channel->IsNew())
    {
      if (Persist(*channel, false))
      {
        groupMember.second.channel->Persisted();
        bReturn = true;
      }
    }
  }

  bReturn &= CommitInsertQueries();

  if (bReturn)
  {
    std::string strQuery;
    std::string strValue;
    for (const auto& groupMember : group.m_members)
    {
      channel = groupMember.second.channel;
      strQuery = PrepareSQL("iUniqueId = %u AND iClientId = %u", channel->UniqueID(), channel->ClientID());
      strValue = GetSingleValue("channels", "idChannel", strQuery);
      if (!strValue.empty() && StringUtils::IsInteger(strValue))
        channel->SetChannelID(atoi(strValue.c_str()));
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
    for (const auto& groupMember : group.m_sortedMembers)
    {
      const std::string strWhereClause = PrepareSQL("idChannel = %u AND idGroup = %u AND iChannelNumber = %u AND iSubChannelNumber = %u",
          groupMember.channel->ChannelID(), group.GroupID(), groupMember.channelNumber.GetChannelNumber(), groupMember.channelNumber.GetSubChannelNumber());

      const std::string strValue = GetSingleValue("map_channelgroups_channels", "idChannel", strWhereClause);
      if (strValue.empty())
      {
        strQuery = PrepareSQL("REPLACE INTO map_channelgroups_channels ("
            "idGroup, idChannel, iChannelNumber, iSubChannelNumber) "
            "VALUES (%i, %i, %i, %i);",
            group.GroupID(), groupMember.channel->ChannelID(), groupMember.channelNumber.GetChannelNumber(), groupMember.channelNumber.GetSubChannelNumber());
        QueueInsertQuery(strQuery);
      }
    }

    bReturn = CommitInsertQueries();
    bRemoveChannels = RemoveStaleChannelsFromGroup(group);
  }

  return bReturn && bRemoveChannels;
}

/********** Client methods **********/

bool CPVRDatabase::ResetEPG(void)
{
  CSingleLock lock(m_critSection);
  const std::string strQuery = PrepareSQL("UPDATE channels SET idEpg = 0");
  return ExecuteQuery(strQuery);
}

bool CPVRDatabase::Persist(CPVRChannelGroup &group)
{
  bool bReturn(false);
  if (group.GroupName().empty())
  {
    CLog::LogF(LOGERROR, "Empty group name");
    return bReturn;
  }

  std::string strQuery;
  bReturn = true;
  CSingleLock lock(m_critSection);
  {
    /* insert a new entry when this is a new group, or replace the existing one otherwise */
    if (group.GroupID() <= 0)
      strQuery = PrepareSQL("INSERT INTO channelgroups (bIsRadio, iGroupType, sName, iLastWatched, bIsHidden, iPosition) VALUES (%i, %i, '%s', %u, %i, %i)",
          (group.IsRadio() ? 1 :0), group.GroupType(), group.GroupName().c_str(), static_cast<unsigned int>(group.LastWatched()), group.IsHidden(), group.GetPosition());
    else
      strQuery = PrepareSQL("REPLACE INTO channelgroups (idGroup, bIsRadio, iGroupType, sName, iLastWatched, bIsHidden, iPosition) VALUES (%i, %i, %i, '%s', %u, %i, %i)",
          group.GroupID(), (group.IsRadio() ? 1 :0), group.GroupType(), group.GroupName().c_str(), static_cast<unsigned int>(group.LastWatched()), group.IsHidden(), group.GetPosition());

    bReturn = ExecuteQuery(strQuery);

    /* set the group id if it was <= 0 */
    if (bReturn && group.GroupID() <= 0)
    {
      CSingleLock lock(group.m_critSection);
      group.m_iGroupId = (int) m_pDS->lastinsertid();
    }
  }

  /* only persist the channel data for the internal groups */
  if (group.IsInternalGroup())
    bReturn &= PersistChannels(group);

  /* persist the group member entries */
  if (bReturn)
    bReturn = PersistGroupMembers(group);

  return bReturn;
}

bool CPVRDatabase::Persist(CPVRChannel &channel, bool bCommit)
{
  bool bReturn(false);

  /* invalid channel */
  if (channel.UniqueID() <= 0)
  {
    CLog::LogF(LOGERROR, "Invalid channel uid: %d", channel.UniqueID());
    return bReturn;
  }

  CSingleLock lock(m_critSection);

  // Note: Do not use channel.ChannelID value to check presence of channel in channels table. It might not yet be set correctly.
  std::string strQuery = PrepareSQL("iUniqueId = %u AND iClientId = %u", channel.UniqueID(), channel.ClientID());
  const std::string strValue = GetSingleValue("channels", "idChannel", strQuery);
  if (strValue.empty())
  {
    /* new channel */
    strQuery = PrepareSQL("INSERT INTO channels ("
        "iUniqueId, bIsRadio, bIsHidden, bIsUserSetIcon, bIsUserSetName, bIsLocked, "
        "sIconPath, sChannelName, bIsVirtual, bEPGEnabled, sEPGScraper, iLastWatched, iClientId, "
        "idEpg) "
        "VALUES (%i, %i, %i, %i, %i, %i, '%s', '%s', %i, %i, '%s', %u, %i, %i)",
        channel.UniqueID(), (channel.IsRadio() ? 1 :0), (channel.IsHidden() ? 1 : 0), (channel.IsUserSetIcon() ? 1 : 0), (channel.IsUserSetName() ? 1 : 0), (channel.IsLocked() ? 1 : 0),
        channel.IconPath().c_str(), channel.ChannelName().c_str(), 0, (channel.EPGEnabled() ? 1 : 0), channel.EPGScraper().c_str(), static_cast<unsigned int>(channel.LastWatched()), channel.ClientID(),
        channel.EpgID());
  }
  else
  {
    /* update channel */
    strQuery = PrepareSQL("REPLACE INTO channels ("
        "iUniqueId, bIsRadio, bIsHidden, bIsUserSetIcon, bIsUserSetName, bIsLocked, "
        "sIconPath, sChannelName, bIsVirtual, bEPGEnabled, sEPGScraper, iLastWatched, iClientId, "
        "idChannel, idEpg) "
        "VALUES (%i, %i, %i, %i, %i, %i, '%s', '%s', %i, %i, '%s', %u, %i, %s, %i)",
        channel.UniqueID(), (channel.IsRadio() ? 1 :0), (channel.IsHidden() ? 1 : 0), (channel.IsUserSetIcon() ? 1 : 0), (channel.IsUserSetName() ? 1 : 0), (channel.IsLocked() ? 1 : 0),
        channel.IconPath().c_str(), channel.ChannelName().c_str(), 0, (channel.EPGEnabled() ? 1 : 0), channel.EPGScraper().c_str(), static_cast<unsigned int>(channel.LastWatched()), channel.ClientID(),
        strValue.c_str(),
        channel.EpgID());
  }

  if (QueueInsertQuery(strQuery))
  {
    bReturn = true;

    if (bCommit)
      bReturn = CommitInsertQueries();
  }

  return bReturn;
}

bool CPVRDatabase::UpdateLastWatched(const CPVRChannel &channel)
{
  CSingleLock lock(m_critSection);
  const std::string strQuery = PrepareSQL("UPDATE channels SET iLastWatched = %u WHERE idChannel = %d",
    static_cast<unsigned int>(channel.LastWatched()), channel.ChannelID());
  return ExecuteQuery(strQuery);
}

bool CPVRDatabase::UpdateLastWatched(const CPVRChannelGroup &group)
{
  CSingleLock lock(m_critSection);
  const std::string strQuery = PrepareSQL("UPDATE channelgroups SET iLastWatched = %u WHERE idGroup = %d",
    static_cast<unsigned int>(group.LastWatched()), group.GroupID());
  return ExecuteQuery(strQuery);
}
