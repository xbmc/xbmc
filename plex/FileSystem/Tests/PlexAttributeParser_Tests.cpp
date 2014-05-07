#include "PlexTest.h"
#include "PlexAttributeParser.h"
#include "URL.h"
#include "PlexApplication.h"
#include "Client/PlexServerManager.h"
#include "Client/PlexServer.h"
#include "settings/AdvancedSettings.h"

#include "FileItem.h"

TEST(PlexAttributeParserMediaUrlGetImageURL, basic)
{
  CURL u("plexserver://abc123:32400/foo");
  CURL imageUrl = CPlexAttributeParserMediaUrl::GetImageURL(u, "/abc123", 320, 320);

  EXPECT_FALSE(imageUrl.Get().empty());
  EXPECT_STREQ(imageUrl.GetFileName(), "photo/:/transcode");
  EXPECT_STREQ(imageUrl.GetOption("width"), "320");
  EXPECT_STREQ(imageUrl.GetOption("height"), "320");
  EXPECT_STREQ(imageUrl.GetOption("url"), "http://127.0.0.1:32400/abc123");
  EXPECT_FALSE(imageUrl.HasOption("format"));
}

TEST(PlexAttributeParserMediaUrlGetImageURL, serverWithOtherPort)
{
  CPlexConnectionPtr conn =
      CPlexConnectionPtr(new CPlexConnection(CPlexConnection::CONNECTION_MANUAL, "10.10.10.10", 32412));
  CPlexServerPtr server = CPlexServerPtr(new CPlexServer(conn));
  server->SetUUID("abc123");
  g_plexApplication.serverManager = CPlexServerManagerPtr(new CPlexServerManager(server));

  CURL u("plexserver://abc123");
  CURL imageUrl = CPlexAttributeParserMediaUrl::GetImageURL(u, "abc123", 320, 320);

  EXPECT_FALSE(imageUrl.Get().empty());
  EXPECT_TRUE(imageUrl.HasOption("url"));
  EXPECT_STREQ(imageUrl.GetOption("url"), "http://127.0.0.1:32412/abc123");

  g_plexApplication.serverManager.reset();
}

TEST(PlexAttributeParserMediaUrlGetImageURL, directURL)
{
  CURL u("plexserver://abc123:32400/foo");
  CURL imageUrl =
      CPlexAttributeParserMediaUrl::GetImageURL(u, "http://google.com/abc123", 320, 320);

  EXPECT_FALSE(imageUrl.Get().empty());
  EXPECT_STREQ(imageUrl.GetOption("url"), "http://google.com/abc123");

  // also test https URL's
  imageUrl = CPlexAttributeParserMediaUrl::GetImageURL(u, "https://google.com/abc123", 320, 320);
  EXPECT_STREQ(imageUrl.GetOption("url"), "https://google.com/abc123");
}

TEST(PlexAttributeParserMediaUrlGetImageURL, sourceSlashes)
{
  CURL u("plexserver://myplex:32400/foo");
  CURL imageUrl = CPlexAttributeParserMediaUrl::GetImageURL(u, "abc123", 320, 320);
  EXPECT_FALSE(imageUrl.Get().empty());
  EXPECT_STREQ(imageUrl.GetOption("url"), "http://127.0.0.1:32400/abc123");

  imageUrl = CPlexAttributeParserMediaUrl::GetImageURL(u, "/abc123", 320, 320);
  EXPECT_STREQ(imageUrl.GetOption("url"), "http://127.0.0.1:32400/abc123");
}

TEST(PlexAttributeParserMediaUrlGetImageURL, myplexHost)
{
  /* For this to work we need to insert a bestServer in the serverManager */
  g_plexApplication.serverManager = CPlexServerManagerPtr(new CPlexServerManager);
  CPlexConnectionPtr conn =
      CPlexConnectionPtr(new CPlexConnection(CPlexConnection::CONNECTION_MANUAL, "10.10.10.10", 32400));
  CPlexServerPtr server = CPlexServerPtr(new CPlexServer(conn));
  server->SetUUID("abc123");
  g_plexApplication.serverManager->SetBestServer(server, true);

  CURL u("plexserver://myplex:32400/foo");
  CURL imageUrl = CPlexAttributeParserMediaUrl::GetImageURL(u, "/abc123", 320, 320);

  EXPECT_FALSE(imageUrl.Get().empty());
  EXPECT_STREQ(imageUrl.GetHostName(), "abc123");

  // it also works with node addresses
  u.SetHostName("node");
  imageUrl = CPlexAttributeParserMediaUrl::GetImageURL(u, "/abc123", 320, 320);
  EXPECT_STREQ(imageUrl.GetHostName(), "abc123");

  // remove serverManager so it doesn't affect other tests.
  g_plexApplication.serverManager.reset();
}

TEST(PlexAttributeParserMediaUrlGetImageURL, forcedJpeg)
{
  g_advancedSettings.m_bForceJpegImageFormat = true;

  CURL u("plexserver://myplex:32400/foo");
  CURL imageUrl = CPlexAttributeParserMediaUrl::GetImageURL(u, "/abc123", 320, 320);
  EXPECT_FALSE(imageUrl.Get().empty());
  EXPECT_STREQ(imageUrl.GetOption("format"), "jpg");

  // reset settings
  g_advancedSettings.Initialize();
}

TEST(PlexAttributeParserMediaUrlGetImageURL, syncedServer)
{
  /* For this to work we need to insert a bestServer in the serverManager */
  CPlexServerPtr server = CPlexServerPtr(new CPlexServer);
  server->SetUUID("abc123");
  server->SetSynced(true);
  g_plexApplication.serverManager = CPlexServerManagerPtr(new CPlexServerManager(server));

  server = CPlexServerPtr(new CPlexServer);
  server->SetUUID("cba321");
  g_plexApplication.serverManager->SetBestServer(server, true);

  CURL u("plexserver://abc123:32400");
  CURL imageUrl = CPlexAttributeParserMediaUrl::GetImageURL(u, "/abc123", 320, 320);

  EXPECT_FALSE(imageUrl.Get().empty());
  EXPECT_STREQ(imageUrl.GetHostName(), "cba321");

  g_plexApplication.serverManager.reset();
}

TEST(PlexAttributeParserMediaUrlGetImageURL, secondaryServer)
{
  CPlexServerPtr server = CPlexServerPtr(new CPlexServer);
  server->SetUUID("abc123");
  server->SetServerClass("secondary");
  g_plexApplication.serverManager = CPlexServerManagerPtr(new CPlexServerManager(server));

  server = CPlexServerPtr(new CPlexServer);
  server->SetUUID("cba321");
  g_plexApplication.serverManager->SetBestServer(server, true);

  CURL u("plexserver://abc123:32400");
  CURL imageUrl = CPlexAttributeParserMediaUrl::GetImageURL(u, "/abc123", 320, 320);

  EXPECT_FALSE(imageUrl.Get().empty());
  EXPECT_STREQ(imageUrl.GetHostName(), "cba321");

  g_plexApplication.serverManager.reset();
}

static CPlexAttributeParserMediaUrl parser;

#define EXPECT_SIZE(key, h, w)                                                                     \
  {                                                                                                \
    EXPECT_TRUE(item.HasArt(key));                                                                 \
    CURL u(item.GetArt(key));                                                                      \
    EXPECT_STREQ(u.GetOption("width"), w);                                                         \
    EXPECT_STREQ(u.GetOption("height"), h);                                                        \
  }

TEST(PlexAttributeParserMediaUrl, thumb)
{
  CFileItem item;
  parser.Process(CURL("plexserver://abc123"), "thumb", "imageurl", &item);

  EXPECT_SIZE("smallThumb", "320", "320");
  EXPECT_SIZE("thumb", "720", "720");
  EXPECT_SIZE("bigThumb", "2048", "2048");
}

TEST(PlexAttributeParserMediaUrl, poster)
{
  CFileItem item;
  parser.Process(CURL("plexserver://abc123"), "poster", "imageurl", &item);

  EXPECT_SIZE("smallPoster", "320", "320");
  EXPECT_SIZE("poster", "720", "720");
  EXPECT_SIZE("bigPoster", "2048", "2048");
}

TEST(PlexAttributeParserMediaUrl, grandparentThumb)
{
  CFileItem item;
  parser.Process(CURL("plexserver://abc123"), "grandparentThumb", "imageurl", &item);

  EXPECT_SIZE("smallGrandparentThumb", "320", "320");
  EXPECT_SIZE("grandparentThumb", "720", "720");
  EXPECT_SIZE("tvshow.thumb", "720", "720");
  EXPECT_SIZE("bigGrandparentThumb", "2048", "2048");
}

TEST(PlexAttributeParserMediaUrl, banner)
{
  CFileItem item;
  parser.Process(CURL("plexserver://abc123"), "banner", "imageurl", &item);

  EXPECT_SIZE("banner", "200", "800");
}

TEST(PlexAttributeParserMediaUrl, art)
{
  CFileItem item;
  parser.Process(CURL("plexserver://abc123"), "art", "imageurl", &item);

  EXPECT_SIZE("fanart", "2048", "2048");
}

TEST(PlexAttributeParserMediaUrl, picture)
{
  CFileItem item;
  parser.Process(CURL("plexserver://abc123"), "picture", "imageurl", &item);

  EXPECT_SIZE("picture", "2048", "2048");
}

TEST(PlexAttributeParserMediaUrl, other)
{
  CFileItem item;
  parser.Process(CURL("plexserver://abc123"), "foobar", "imageurl", &item);

  EXPECT_SIZE("foobar", "320", "320");
}
