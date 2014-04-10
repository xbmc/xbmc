#include "PlexTestUtils.h"
#include "PlexApplication.h"
#include "Client/PlexServerManager.h"

void PlexServerManagerTestUtility::SetUp()
{
  if (!server)
  {
    server = CPlexServerPtr(new CPlexServer("abc123", "test server", true));
    CPlexConnectionPtr conn = CPlexConnectionPtr(new CPlexConnection(
        CPlexConnection::CONNECTION_DISCOVERED, "10.10.10.10", 32400, "http", "token"));
    server->AddConnection(conn);
  }

  g_plexApplication.serverManager = CPlexServerManagerPtr(new CPlexServerManager(server));
}

void PlexServerManagerTestUtility::TearDown()
{
  g_plexApplication.serverManager.reset();
}
