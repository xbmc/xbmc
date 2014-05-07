#include "PlexTest.h"
#include "Client/PlexServer.h"
#include "Client/PlexConnection.h"

TEST(PlexServerGetLocalConnection, basic)
{
  CPlexConnectionPtr conn = CPlexConnectionPtr(new CPlexConnection(CPlexConnection::CONNECTION_MANUAL, "10.10.10.10", 32400));
  CPlexServerPtr server = CPlexServerPtr(new CPlexServer(conn));

  CPlexConnectionPtr localConn = server->GetLocalConnection();
  EXPECT_TRUE(localConn);
  EXPECT_STREQ(localConn->GetAddress().Get(), "http://10.10.10.10:32400/");
}

TEST(PlexServerGetLocalConnection, noLocalConnection)
{
  // non local connection
  CPlexConnectionPtr conn = CPlexConnectionPtr(new CPlexConnection(CPlexConnection::CONNECTION_MANUAL, "8.8.8.8", 32400));
  CPlexServerPtr server = CPlexServerPtr(new CPlexServer(conn));

  EXPECT_FALSE(server->GetLocalConnection());
}

TEST(PlexServerGetLocalConnection, localConnectionActive)
{
  CPlexConnectionPtr conn = CPlexConnectionPtr(new CPlexConnection(CPlexConnection::CONNECTION_MANUAL, "10.10.10.10", 32400));
  CPlexServerPtr server = CPlexServerPtr(new CPlexServer(conn));
  server->SetActiveConnection(conn);

  CPlexConnectionPtr localConn = server->GetLocalConnection();
  EXPECT_TRUE(localConn);
}

TEST(PlexServerGetLocalConnection, needSearchForLocalConnection)
{
  CPlexServerPtr server = CPlexServerPtr(new CPlexServer());

  CPlexConnectionPtr conn = CPlexConnectionPtr(new CPlexConnection(CPlexConnection::CONNECTION_MANUAL, "8.8.8.8", 32400));
  server->AddConnection(conn);

  conn = CPlexConnectionPtr(new CPlexConnection(CPlexConnection::CONNECTION_MANUAL, "10.10.10.10", 32400));
  server->AddConnection(conn);

  CPlexConnectionPtr localConn = server->GetLocalConnection();
  EXPECT_TRUE(localConn);
  EXPECT_STREQ(localConn->GetAddress().Get(), "http://10.10.10.10:32400/");
}
