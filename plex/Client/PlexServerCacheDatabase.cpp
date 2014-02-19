#include "PlexServerCacheDatabase.h"

#include "dbwrappers/Database.h"
#include "dbwrappers/dataset.h"
#include "log.h"

#include "StringUtils.h"
#include <boost/foreach.hpp>

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CPlexServerCacheDatabase::CreateTables()
{
  try
  {
    CDatabase::CreateTables();

    CLog::Log(LOGINFO, "CPlexServerCacheDatabase::CreateTables create server table");
    m_pDS->exec("CREATE TABLE server ( uuid text primary key, name text, version text, owner text, synced bool, owned bool, serverClass text, supportsDeletion bool, supportsVideoTranscoding bool, supportsAudioTranscoding bool, transcoderQualities text, transcoderBitrates text, transcoderResolutions text );\n");
    CLog::Log(LOGINFO, "CPlexServerCacheDatabase::CreateTables create connections table");
    m_pDS->exec("CREATE TABLE connections ( serverUUID text, host text, port integer, token text, type integer, scheme text );\n");
    CLog::Log(LOGINFO, "CPlexServerCacheDatabase::CreateTables create connections table index");
    m_pDS->exec("create index connectionUUID on connections ( serverUUID );\n");
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s unable to create tables", __FUNCTION__);
    return false;
  }

  return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CPlexServerCacheDatabase::clearTables()
{
  CLog::Log(LOGINFO, "CPlexServerCacheDatabase::clearTables");

  try
  {
    m_pDS->exec("delete from connections");
    m_pDS->exec("delete from server");
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CPlexServerCacheDatabase::clearTables failed to empty out the tables");
    return false;
  }

  return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CPlexServerCacheDatabase::storeServer(const CPlexServerPtr &server)
{
  if (server->GetSynced())
    return true;

  CStdString qualities = StringUtils::Join(server->GetTranscoderQualities(), ",");
  CStdString bitrates = StringUtils::Join(server->GetTranscoderBitrates(), ",");
  CStdString resolutions = StringUtils::Join(server->GetTranscoderResolutions(), ",");

  CStdString sql = PrepareSQL("insert into server values ('%s', '%s', '%s', '%s', %i, %i, '%s', %i, %i, %i, '%s', '%s', '%s');",
                              server->GetUUID().c_str(), server->GetName().c_str(), server->GetVersion().c_str(), server->GetOwner().c_str(),
                              server->GetSynced(), server->GetOwned(), server->GetServerClass().c_str(), server->SupportsDeletion(),
                              server->SupportsVideoTranscoding(), server->SupportsAudioTranscoding(), qualities.c_str(),
                              bitrates.c_str(), resolutions.c_str());
  try
  {
    m_pDS->exec(sql);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CPlexServerCacheDatabase:storeServer failed to store server %s", server->GetName().c_str());
    return false;
  }

  std::vector<CPlexConnectionPtr> connections;
  server->GetConnections(connections);

  BOOST_FOREACH(CPlexConnectionPtr conn, connections)
  {
    if (!storeConnection(server->GetUUID(), conn))
      return false;
  }

  return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CPlexServerCacheDatabase::storeConnection(const CStdString &uuid, const CPlexConnectionPtr &connection)
{
  CStdString sql = PrepareSQL("insert into connections values ('%s', '%s', %i, '%s', %i, '%s');\n",
                              uuid.c_str(), connection->GetAddress().GetHostName().c_str(), connection->GetAddress().GetPort(),
                              connection->GetAccessToken().c_str(), connection->m_type, connection->GetAddress().GetProtocol().c_str());
  try
  {
    m_pDS->exec(sql);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CPlexServerCacheDatabase::storeConnection failed to store connection %s", connection->GetAddress().Get().c_str());
    return false;
  }

  return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CPlexServerCacheDatabase::getCachedServers(std::vector<CPlexServerPtr>& servers)
{
  try
  {
    if (m_pDB.get() == NULL) return false;
    if (m_pDS.get() == NULL) return false;

    m_pDS->query("select * from server;\n");
    while (!m_pDS->eof())
    {
      CStdString name = m_pDS->fv("name").get_asString();
      CStdString uuid = m_pDS->fv("uuid").get_asString();
      bool owned = m_pDS->fv("owned").get_asBool();
      bool synced = m_pDS->fv("synced").get_asBool();

      if (name.empty() || uuid.empty())
      {
        CLog::Log(LOGERROR, "Could not read server from table, skipping");
        m_pDS->next();
        continue;
      }

      CLog::Log(LOGDEBUG, "CPlexServerCacheDatabase::getCachedServers reading server %s", name.c_str());

      CPlexServerPtr server = CPlexServerPtr(new CPlexServer(uuid, name, owned, synced));
      server->SetOwner(m_pDS->fv("owner").get_asString());
      server->SetVersion(m_pDS->fv("version").get_asString());
      server->SetSupportsAudioTranscoding(m_pDS->fv("supportsAudioTranscoding").get_asBool());
      server->SetSupportsVideoTranscoding(m_pDS->fv("supportsVideoTranscoding").get_asBool());
      server->SetSupportsDeletion(m_pDS->fv("supportsDeletion").get_asBool());

      CStdString resolutions = m_pDS->fv("transcoderResolutions").get_asString();
      if (!resolutions.empty())
      {
        PlexStringVector t = StringUtils::Split(resolutions, ",");
        server->SetTranscoderResolutions(t);
      }

      CStdString qualities = m_pDS->fv("transcoderQualities").get_asString();
      if (!qualities.empty())
      {
        PlexStringVector t = StringUtils::Split(qualities, ",");
        server->SetTranscoderQualities(t);
      }

      CStdString bitrates = m_pDS->fv("transcoderBitrates").get_asString();
      if (!bitrates.empty())
      {
        PlexStringVector t = StringUtils::Split(bitrates, ",");
        server->SetTranscoderBitrates(t);
      }

      servers.push_back(server);
      m_pDS->next();
    }

    m_pDS->close();

    /* now add connections */
    BOOST_FOREACH(CPlexServerPtr server, servers)
    {
      CStdString sql = PrepareSQL("select * from connections where serverUUID='%s';\n", server->GetUUID().c_str());
      m_pDS->query(sql);

      while (!m_pDS->eof())
      {
        int type = m_pDS->fv("type").get_asInt();
        CStdString address = m_pDS->fv("host").get_asString();
        CStdString schema = m_pDS->fv("scheme").get_asString();
        int port = m_pDS->fv("port").get_asInt();
        CStdString token = m_pDS->fv("token").get_asString();

        CPlexConnectionPtr connection = CPlexConnectionPtr(new CPlexConnection(type, address, port, schema, token));
        server->AddConnection(connection);

        m_pDS->next();
      }

      m_pDS->close();
    }

    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CPlexServerCacheDatabase::getCachedServers failed to read servers from database.");
    return false;
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CPlexServerCacheDatabase::cacheServers()
{
  PlexServerList servers = g_plexApplication.serverManager->GetAllServers();

  BeginTransaction();

  clearTables();

  BOOST_FOREACH(CPlexServerPtr server, servers)
  {
    if (!storeServer(server))
    {
      CLog::Log(LOGERROR, "CPlexServerCacheDatabase::cacheServers failed to store server, rolling back.");
      RollbackTransaction();
      return false;
    }
  }

  CommitTransaction();
  return true;
}
