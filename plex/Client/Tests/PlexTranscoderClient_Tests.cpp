#include "PlexTest.h"
#include "Client/PlexTranscoderClient.h"
#include "FileItem.h"

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
