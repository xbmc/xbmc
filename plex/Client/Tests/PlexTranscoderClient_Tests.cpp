#include "PlexTest.h"
#include "Client/PlexTranscoderClient.h"
#include "FileItem.h"
#include "Client/PlexConnection.h"
#include "GlobalsHandling.h"

using ::testing::Return;

#define ADD_QUAL(a,b) { CFileItemPtr mItem = CFileItemPtr(new CFileItem); mItem->SetProperty("id", b); mItem->SetProperty("mediaTag-videoResolution", a); item.m_mediaItems.push_back(mItem); }

///////////////////////////////////////////////////////////////////////////////////////////////////
class PlexTranscoderClientFake : public CPlexTranscoderClient
{
public:
  MOCK_CONST_METHOD0(transcodeForced, bool());
  MOCK_CONST_METHOD0(localBitrate, int());
  MOCK_CONST_METHOD0(remoteBitrate, int());
  MOCK_CONST_METHOD0(transcodeSubtitles, bool());
};

///////////////////////////////////////////////////////////////////////////////////////////////////
class PlexTranscoderClientTests : public testing::Test
{
public:
  void SetUp()
  {
    client = new ::testing::NiceMock<PlexTranscoderClientFake>;
  }

  void TearDown()
  {
    delete client;
  }

  ::testing::NiceMock<PlexTranscoderClientFake>* client;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
TEST_F(PlexTranscoderClientTests, autoSelectQuality_basic)
{
  CFileItem item;
  ADD_QUAL("480", 42);
  ADD_QUAL("1080", 40);
  ADD_QUAL("SD", 43);
  ADD_QUAL("720", 41);

  EXPECT_EQ(40, PlexTranscoderClientFake::autoSelectQuality(item, PLEX_ONLINE_QUALITY_1080p));
  EXPECT_EQ(41, PlexTranscoderClientFake::autoSelectQuality(item, PLEX_ONLINE_QUALITY_720p));
  EXPECT_EQ(42, PlexTranscoderClientFake::autoSelectQuality(item, PLEX_ONLINE_QUALITY_480p));
  EXPECT_EQ(43, PlexTranscoderClientFake::autoSelectQuality(item, PLEX_ONLINE_QUALITY_SD));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
TEST_F(PlexTranscoderClientTests, autoSelectQuality_badFlag)
{
  CFileItem item;
  ADD_QUAL("foobar", 40);
  ADD_QUAL("720", 41);
  EXPECT_EQ(41, PlexTranscoderClientFake::autoSelectQuality(item, PLEX_ONLINE_QUALITY_720p));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
TEST_F(PlexTranscoderClientTests, shouldTranscode_directplaytrack)
{
  CFileItem item;
  item.SetProperty("bitrate", 3000);
  
  // Local Server
  CPlexServerPtr server = PlexTestUtils::serverWithConnection("", "127.0.0.1");
  
  ON_CALL(*client, localBitrate()).WillByDefault(Return(0));
  ON_CALL(*client, remoteBitrate()).WillByDefault(Return(0));
  ON_CALL(*client, transcodeForced()).WillByDefault(Return(false));
  
  // DirectPlay on a movie
  item.SetProperty("type", "track");
  EXPECT_EQ(false, client->ShouldTranscode(server, item));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
TEST_F(PlexTranscoderClientTests, shouldTranscode_directplayvideo)
{
  CFileItem item;
  item.SetProperty("bitrate", 3000);
  item.SetProperty("type", "movie");
  
  // Local Server
  CPlexServerPtr server = PlexTestUtils::serverWithConnection("", "127.0.0.1");
  
  ON_CALL(*client, localBitrate()).WillByDefault(Return(0));
  ON_CALL(*client, remoteBitrate()).WillByDefault(Return(0));
  ON_CALL(*client, transcodeForced()).WillByDefault(Return(false));
  
  // DirectPlay on a movie
  EXPECT_EQ(false, client->ShouldTranscode(server, item));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
TEST_F(PlexTranscoderClientTests, shouldTranscode_localforcetranscode)
{
  CFileItem item;
  item.SetProperty("bitrate", 3000);
  item.SetProperty("type", "movie");
  
  // Local Server
  CPlexServerPtr server = PlexTestUtils::serverWithConnection("", "127.0.0.1");
  
  ON_CALL(*client, localBitrate()).WillByDefault(Return(4000));
  ON_CALL(*client, remoteBitrate()).WillByDefault(Return(0));
  ON_CALL(*client, transcodeForced()).WillByDefault(Return(true));
  
  // Local Transcode below requested quality, with forcing
  EXPECT_EQ(true, client->ShouldTranscode(server, item));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
TEST_F(PlexTranscoderClientTests, shouldTranscode_locallowerthanthreshold)
{
  CFileItem item;
  item.SetProperty("bitrate", 3000);
  item.SetProperty("type", "movie");
  
  // Local Server
  CPlexServerPtr server = PlexTestUtils::serverWithConnection("", "127.0.0.1");
  
  ON_CALL(*client, localBitrate()).WillByDefault(Return(4000));
  ON_CALL(*client, remoteBitrate()).WillByDefault(Return(0));
  ON_CALL(*client, transcodeForced()).WillByDefault(Return(false));
  
  // Local Transcode below requested quality, with forcing
  EXPECT_EQ(false, client->ShouldTranscode(server, item));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
TEST_F(PlexTranscoderClientTests, shouldTranscode_localhigherthanthreshold)
{
  CFileItem item;
  item.SetProperty("bitrate", 3000);
  item.SetProperty("type", "movie");
  
  // Local Server
  CPlexServerPtr server = PlexTestUtils::serverWithConnection("", "127.0.0.1");
  
  ON_CALL(*client, localBitrate()).WillByDefault(Return(2000));
  ON_CALL(*client, remoteBitrate()).WillByDefault(Return(0));
  ON_CALL(*client, transcodeForced()).WillByDefault(Return(false));
  
  // Local Transcode below requested quality, with forcing
  EXPECT_EQ(true, client->ShouldTranscode(server, item));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
TEST_F(PlexTranscoderClientTests, shouldTranscode_remoteforcetranscode)
{
  CFileItem item;
  item.SetProperty("bitrate", 3000);
  item.SetProperty("type", "movie");
  
  // Local Server
  CPlexServerPtr server = PlexTestUtils::serverWithConnection("", "99.0.0.1");
  
  ON_CALL(*client, localBitrate()).WillByDefault(Return(0));
  ON_CALL(*client, remoteBitrate()).WillByDefault(Return(4000));
  ON_CALL(*client, transcodeForced()).WillByDefault(Return(true));
  
  // Local Transcode below requested quality, with forcing
  EXPECT_EQ(true, client->ShouldTranscode(server, item));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
TEST_F(PlexTranscoderClientTests, shouldTranscode_remotelowerthanthreshold)
{
  CFileItem item;
  item.SetProperty("bitrate", 3000);
  item.SetProperty("type", "movie");
  
  // Local Server
  CPlexServerPtr server = PlexTestUtils::serverWithConnection("", "99.0.0.1");
  
  ON_CALL(*client, localBitrate()).WillByDefault(Return(0));
  ON_CALL(*client, remoteBitrate()).WillByDefault(Return(4000));
  ON_CALL(*client, transcodeForced()).WillByDefault(Return(false));
  
  // Local Transcode below requested quality, with forcing
  EXPECT_EQ(false, client->ShouldTranscode(server, item));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
TEST_F(PlexTranscoderClientTests, shouldTranscode_remotehigherthanthreshold)
{
  CFileItem item;
  item.SetProperty("bitrate", 3000);
  item.SetProperty("type", "movie");
  
  // Local Server
  CPlexServerPtr server = PlexTestUtils::serverWithConnection("", "99.0.0.1");
  
  ON_CALL(*client, localBitrate()).WillByDefault(Return(0));
  ON_CALL(*client, remoteBitrate()).WillByDefault(Return(2000));
  ON_CALL(*client, transcodeForced()).WillByDefault(Return(false));
  
  // Local Transcode below requested quality, with forcing
  EXPECT_EQ(true, client->ShouldTranscode(server, item));
}
