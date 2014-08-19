#include "PlexTestUtils.h"
#include "PlexApplication.h"
#include "Client/PlexServerManager.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
void PlexServerManagerTestUtility::SetUp()
{
  if (!server)
  {
    server = CPlexServerPtr(new CPlexServer("abc123", "test server", true));
    CPlexConnectionPtr conn = CPlexConnectionPtr(new CPlexConnection(
        CPlexConnection::CONNECTION_DISCOVERED, "10.10.10.10", 32400, "http", "token"));
    server->SetVersion("0.9.9.7.0-abc123");
    server->AddConnection(conn);
  }

  g_plexApplication.serverManager = CPlexServerManagerPtr(new CPlexServerManager(server));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void PlexServerManagerTestUtility::TearDown()
{
  g_plexApplication.serverManager.reset();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool PlexTestUtils::listFromXML(const CStdString& xml, CFileItemList& list)
{
  PlexDirectoryFakeDataTest dir(xml);
  return dir.GetDirectory("plexserver://abc123/library/sections/1/all", list);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// does all the boiler-plate, the tests are expected to modify the other attributes if needed
CPlexServerPtr PlexTestUtils::serverWithConnection(const std::string& uuid, const std::string& host)
{
  CPlexServerPtr server = CPlexServerPtr(new CPlexServer(uuid, "test server: " + uuid, true));
  CPlexConnectionPtr conn = CPlexConnectionPtr(new CPlexConnection(CPlexConnection::CONNECTION_DISCOVERED,
                                                                   host, 32400, "http", "token"));
  server->AddConnection(conn);
  server->SetActiveConnection(conn);
  
  return server;
}

CPlexServerPtr PlexTestUtils::serverWithConnection(const CPlexConnectionPtr& connection)
{
  CPlexServerPtr server = CPlexServerPtr(new CPlexServer("uuid", "test server", true));
  server->AddConnection(connection);
  server->SetActiveConnection(connection);
  return server;
}
