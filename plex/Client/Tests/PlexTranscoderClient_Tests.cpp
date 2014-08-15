#include "PlexTest.h"
#include "Client/PlexTranscoderClient.h"
#include "FileItem.h"
#include "GUISettings.h"
#include "Client/PlexConnection.h"

#define ADD_QUAL(a,b) { CFileItemPtr mItem = CFileItemPtr(new CFileItem); mItem->SetProperty("id", b); mItem->SetProperty("mediaTag-videoResolution", a); item.m_mediaItems.push_back(mItem); }

TEST(PlexTranscoderClientTests, autoSelectQuality_basic)
{
  CFileItem item;
  ADD_QUAL("480", 42);
  ADD_QUAL("1080", 40);
  ADD_QUAL("SD", 43);
  ADD_QUAL("720", 41);

  EXPECT_EQ(40, CPlexTranscoderClient::autoSelectQuality(item, PLEX_ONLINE_QUALITY_1080p));
  EXPECT_EQ(41, CPlexTranscoderClient::autoSelectQuality(item, PLEX_ONLINE_QUALITY_720p));
  EXPECT_EQ(42, CPlexTranscoderClient::autoSelectQuality(item, PLEX_ONLINE_QUALITY_480p));
  EXPECT_EQ(43, CPlexTranscoderClient::autoSelectQuality(item, PLEX_ONLINE_QUALITY_SD));
}

TEST(PlexTranscoderClientTests, autoSelectQuality_badFlag)
{
  CFileItem item;
  ADD_QUAL("foobar", 40);
  ADD_QUAL("720", 41);
  EXPECT_EQ(41, CPlexTranscoderClient::autoSelectQuality(item, PLEX_ONLINE_QUALITY_720p));
}

TEST(PlexTranscoderClientTests, shouldTranscode)
{
  CFileItem item;
  item.SetProperty("bitrate", 3000);
  
  // Local Server
  CPlexConnectionPtr connection = CPlexConnectionPtr(new CPlexConnection(CPlexConnection::CONNECTION_MANUAL, "127.0.0.1", 32400));
  CPlexServerPtr server = CPlexServerPtr(new CPlexServer(connection));
  CPlexTranscoderClient transcoder;
  
  // directPlay
  g_guiSettings.SetInt("plexmediaserver.localquality", 0);
  g_guiSettings.SetInt("plexmediaserver.remotequality", 0);
  g_guiSettings.SetBool("plexmediaserver.forcetranscode", false);
  
  // transcode something that is not a video
  item.SetProperty("type", "track");
  EXPECT_EQ(false, transcoder.ShouldTranscode(server, item));
  
  // DirectPlay on a movie
  item.SetProperty("type", "movie");
  EXPECT_EQ(false, transcoder.ShouldTranscode(server, item));
  
  // Local Transcode below requested quality, without forcing
  g_guiSettings.SetInt("plexmediaserver.localquality", 4000);
  EXPECT_EQ(false, transcoder.ShouldTranscode(server, item));
  
  // Local Transcode below requested quality, with forcing
  g_guiSettings.SetBool("plexmediaserver.forcetranscode", true);
  EXPECT_EQ(true, transcoder.ShouldTranscode(server, item));
  
  // Local transcode above requested quality, without forcing
  g_guiSettings.SetBool("plexmediaserver.forcetranscode", false);
  g_guiSettings.SetInt("plexmediaserver.localquality", 2000);
  EXPECT_EQ(true, transcoder.ShouldTranscode(server, item));
  
  //same thing with remote
  connection = CPlexConnectionPtr(new CPlexConnection(CPlexConnection::CONNECTION_MANUAL, "99.0.0.1", 32400));
  server = CPlexServerPtr(new CPlexServer(connection));
  
  g_guiSettings.SetInt("plexmediaserver.localquality", 0);
  g_guiSettings.SetInt("plexmediaserver.remotequality", 0);
  g_guiSettings.SetBool("plexmediaserver.forcetranscode", false);
  
  item.SetProperty("bitrate", 3000);
  
  EXPECT_EQ(false, transcoder.ShouldTranscode(server, item));
  
  // Remote Transcode below requested quality, without forcing
  g_guiSettings.SetInt("plexmediaserver.remotequality", 4000);
  EXPECT_EQ(false, transcoder.ShouldTranscode(server, item));
  
  // Remote Transcode below requested quality, with forcing
  g_guiSettings.SetBool("plexmediaserver.forcetranscode", true);
  EXPECT_EQ(true, transcoder.ShouldTranscode(server, item));
  
  // Remote transcode above requested quality, without forcing
  g_guiSettings.SetBool("plexmediaserver.forcetranscode", false);
  g_guiSettings.SetInt("plexmediaserver.remotequality", 2000);
  EXPECT_EQ(true, transcoder.ShouldTranscode(server, item));
}