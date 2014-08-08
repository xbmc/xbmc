//
//  PlexConnection_Tests.cpp
//  Plex Home Theater
//
//  Created by Tobias Hieta on 08/08/14.
//
//

#include "PlexTest.h"
#include "Client/PlexConnection.h"

#define NEW_CONN(type, host, port, schema, token) CPlexConnectionPtr(new CPlexConnection(CPlexConnection::type, host, port, schema, token))

TEST(PlexConnection, EqualsTrue)
{
  CPlexConnectionPtr conn = NEW_CONN(CONNECTION_DISCOVERED, "10.0.0.1", 32400, "http", "token");
  CPlexConnectionPtr conn2 = NEW_CONN(CONNECTION_DISCOVERED, "10.0.0.1", 32400, "http", "token");
  
  EXPECT_TRUE(conn->Equals(conn2));
  
  conn2 = NEW_CONN(CONNECTION_MYPLEX, "10.0.0.1", 32400, "http", "token");
  EXPECT_TRUE(conn->Equals(conn2));
}

TEST(PlexConnection, EqualsFalse)
{
  CPlexConnectionPtr conn = NEW_CONN(CONNECTION_DISCOVERED, "10.0.0.1", 32400, "http", "token");
  CPlexConnectionPtr conn2 = NEW_CONN(CONNECTION_DISCOVERED, "10.0.0.1", 32400, "http", "token2");
  
  // should fail because of different tokens
  EXPECT_FALSE(conn->Equals(conn2));
  
  // should fail because of different hosts
  conn2 = NEW_CONN(CONNECTION_DISCOVERED, "10.0.0.2", 32400, "http", "token");
  EXPECT_FALSE(conn->Equals(conn2));

  // should fail because of different ports
  conn2 = NEW_CONN(CONNECTION_DISCOVERED, "10.0.0.1", 32401, "http", "token");
  EXPECT_FALSE(conn->Equals(conn2));
  
  // should fail because of different schemas
  conn2 = NEW_CONN(CONNECTION_DISCOVERED, "10.0.0.1", 32400, "https", "token");
  EXPECT_FALSE(conn->Equals(conn2));
}

TEST(PlexConnection, merge)
{
  CPlexConnectionPtr conn = NEW_CONN(CONNECTION_DISCOVERED, "10.0.0.1", 32400, "http", "");
  CPlexConnectionPtr conn2 = NEW_CONN(CONNECTION_MYPLEX, "10.0.0.2", 32400, "http", "token");
  
  conn->Merge(conn2);
  EXPECT_STREQ("token", conn->GetAccessToken());
  EXPECT_STREQ("http://10.0.0.2:32400/", conn->GetAddress().Get());
}

TEST(PlexConnection, mergeEmptyToken)
{
  CPlexConnectionPtr conn = NEW_CONN(CONNECTION_DISCOVERED, "10.0.0.1", 32400, "http", "token");
  CPlexConnectionPtr conn2 = NEW_CONN(CONNECTION_MYPLEX, "10.0.0.2", 32400, "http", "");
  
  conn->Merge(conn2);
  EXPECT_STREQ("", conn->GetAccessToken());
}
