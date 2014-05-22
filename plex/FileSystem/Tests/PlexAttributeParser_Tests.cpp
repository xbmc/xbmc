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

static CPlexAttributeParserMediaFlag mflag;

TEST(PlexAttributeParserMediaFlag, basic)
{
  CFileItem item;
  item.SetProperty("mediaTagPrefix", "abc123");
  item.SetProperty("mediaTagVersion", "1");

  mflag.Process(CURL("plexserver://abc123"), "aspectRatio", "16:9", &item);
  EXPECT_TRUE(item.HasProperty("mediaTag-aspectRatio"));
  EXPECT_TRUE(item.HasArt("mediaTag::aspectRatio"));
}

TEST(PlexAttributeParserMediaFlag, testCache)
{
  CFileItem item;
  item.SetProperty("mediaTagPrefix", "abc123");
  item.SetProperty("mediaTagVersion", "1");

  mflag.Process(CURL("plexserver://abc123"), "aspectRatio", "16:9", &item);
  EXPECT_TRUE(item.HasProperty("mediaTag-aspectRatio"));
  EXPECT_TRUE(item.HasArt("mediaTag::aspectRatio"));

  CFileItem item2;
  item2.SetProperty("mediaTagPrefix", "abc123");
  item2.SetProperty("mediaTagVersion", "2");

  mflag.Process(CURL("plexserver://abc123"), "aspectRatio", "16:9", &item2);
  EXPECT_TRUE(item2.HasProperty("mediaTag-aspectRatio"));
  EXPECT_TRUE(item2.HasArt("mediaTag::aspectRatio"));
}

const char itemxml[] =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
    "<MediaContainer size=\"1\" allowSync=\"1\" identifier=\"com.plexapp.plugins.library\" librarySectionID=\"1\" librarySectionTitle=\"Movies\" librarySectionUUID=\"8a963b4b-68b5-4e01-9c0b-64cc416fbc3f\" mediaTagPrefix=\"/system/bundle/media/flags/\" mediaTagVersion=\"1391191269\">"
    "<Video ratingKey=\"1888\" key=\"/library/metadata/1888\" guid=\"com.plexapp.agents.imdb://tt0290334?lang=en\" studio=\"20th Century Fox\" type=\"movie\" title=\"X2\" contentRating=\"PG-13\" summary=\"Professor Charles Xavier and his team of genetically gifted superheroes face a rising tide of anti-mutant sentiment led by Col. William Stryker in this sequel to the Marvel Comics-based blockbuster X-Men. Storm, Wolverine and Jean Grey must join their usual nemeses Magneto and Mystique to unhinge Stryker&apos;s scheme to exterminate all mutants.\" rating=\"6.3000001907348597\" year=\"2003\" tagline=\"The time has come for those who are different to stand united\" thumb=\"/library/metadata/1888/thumb/1391598827\" art=\"/library/metadata/1888/art/1391598827\" duration=\"8027491\" originallyAvailableAt=\"2003-04-24\" addedAt=\"1385840831\" updatedAt=\"1391598827\">"
    "<Media videoResolution=\"1080\" id=\"1846\" duration=\"8027491\" bitrate=\"11675\" width=\"1920\" height=\"800\" aspectRatio=\"2.35\" audioChannels=\"6\" audioCodec=\"dca\" videoCodec=\"h264\" container=\"mkv\" videoFrameRate=\"24p\">"
    "<Part id=\"1950\" key=\"/library/parts/1950/file.mkv\" duration=\"8027491\" file=\"/data/Videos/Movies/X2 X-Men United (2003)/X2 X-Men United.2003.mkv\" size=\"11714969381\" container=\"mkv\" indexes=\"sd\">"
    "<Stream id=\"47867\" streamType=\"1\" codec=\"h264\" index=\"0\" bitrate=\"10164\" language=\"English\" languageCode=\"eng\" bitDepth=\"8\" cabac=\"1\" chromaSubsampling=\"4:2:0\" codecID=\"V_MPEG4/ISO/AVC\" colorSpace=\"yuv\" duration=\"8027486\" frameRate=\"23.976\" frameRateMode=\"cfr\" hasScalingMatrix=\"0\" height=\"800\" level=\"41\" profile=\"high\" refFrames=\"5\" scanType=\"progressive\" title=\"\" width=\"1920\" />"
    "<Stream id=\"47868\" streamType=\"2\" selected=\"1\" codec=\"dca\" index=\"1\" channels=\"6\" bitrate=\"1509\" language=\"English\" languageCode=\"eng\" bitDepth=\"24\" bitrateMode=\"cbr\" codecID=\"A_DTS\" duration=\"8027491\" samplingRate=\"48000\" title=\"\" />"
    "<Stream id=\"47869\" streamType=\"3\" selected=\"1\" default=\"1\" index=\"2\" language=\"English\" languageCode=\"eng\" codecID=\"S_TEXT/UTF8\" format=\"srt\" title=\"\" />"
    "<Stream id=\"47870\" streamType=\"3\" index=\"3\" language=\"Nederlands\" languageCode=\"dut\" codecID=\"S_TEXT/UTF8\" format=\"srt\" title=\"\" />"
    "<Stream id=\"47871\" streamType=\"3\" index=\"4\" language=\"Français\" languageCode=\"fre\" codecID=\"S_TEXT/UTF8\" format=\"srt\" title=\"\" />"
    "<Stream id=\"47872\" streamType=\"3\" index=\"5\" language=\"Italiano\" languageCode=\"ita\" codecID=\"S_TEXT/UTF8\" format=\"srt\" title=\"\" />"
    "<Stream id=\"47873\" streamType=\"3\" index=\"6\" language=\"Português\" languageCode=\"por\" codecID=\"S_TEXT/UTF8\" format=\"srt\" title=\"\" />"
    "</Part>"
    "</Media>"
    "<Genre id=\"21\" tag=\"Thriller\" />"
    "<Genre id=\"2373\" tag=\"Action/Adventure\" />"
    "<Genre id=\"1516\" tag=\"Action film\" />"
    "<Writer id=\"3435\" tag=\"David Hayter\" />"
    "<Writer id=\"3500\" tag=\"Michael Dougherty\" />"
    "<Director id=\"3434\" tag=\"Bryan Singer\" />"
    "<Country id=\"24\" tag=\"USA\" />"
    "<Role id=\"3501\" tag=\"Bryce Hodgson\" role=\"Artie Maddicks\" />"
    "<Role id=\"3502\" tag=\"Daniel Cudmore\" role=\"Piotr Rasputin\" />"
    "<Collection id=\"3931\" tag=\"Marvel\" />"
    "<Field name=\"thumb\" locked=\"1\" />"
    "<Field name=\"collection\" locked=\"1\" />"
    "</Video>"
    "</MediaContainer>";

TEST(PlexAttributeParserMediaFlag, fullitem)
{
  CFileItemList list;
  EXPECT_TRUE(PlexTestUtils::listFromXML(itemxml, list));
  EXPECT_TRUE(list.Get(0));
  CFileItemPtr item = list.Get(0);
  EXPECT_TRUE(item->HasProperty("mediaTag-audioChannels"));
  EXPECT_EQ(6, item->GetProperty("mediaTag-audioChannels").asInteger());
}
