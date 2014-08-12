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

#define TOKEN_CONN(token) NEW_CONN(CONNECTION_DISCOVERED, "10.0.0.1", 32400, "http", token)

TEST(PlexConnection, tokenComparison)
{
  EXPECT_TRUE(TOKEN_CONN("token")->Equals(TOKEN_CONN("token")));
  EXPECT_TRUE(TOKEN_CONN("")->Equals(TOKEN_CONN("token")));
  EXPECT_TRUE(TOKEN_CONN("")->Equals(TOKEN_CONN("")));
  EXPECT_FALSE(TOKEN_CONN("token")->Equals(TOKEN_CONN("token2")));
}

TEST(PlexConnection, merge)
{
  CPlexConnectionPtr conn = NEW_CONN(CONNECTION_DISCOVERED, "10.0.0.1", 32400, "http", "");
  CPlexConnectionPtr conn2 = NEW_CONN(CONNECTION_MYPLEX, "10.0.0.2", 32400, "http", "token");

  conn->Merge(conn2);
  EXPECT_STREQ("token", conn->GetAccessToken());
  EXPECT_STREQ("http://10.0.0.2:32400/", conn->GetAddress().Get());
}

TEST(PlexConnection, mergeTokens)
{
  CPlexConnectionPtr conn = TOKEN_CONN("token");
  conn->Merge(TOKEN_CONN("token2"));
  EXPECT_STREQ("token2", conn->GetAccessToken());

  conn->Merge(TOKEN_CONN(""));
  EXPECT_STREQ("token2", conn->GetAccessToken());

  conn = TOKEN_CONN("");
  conn->Merge(TOKEN_CONN("token2"));
  EXPECT_STREQ("token2", conn->GetAccessToken());
}
