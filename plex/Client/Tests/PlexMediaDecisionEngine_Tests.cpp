#include "PlexTest.h"
#include "PlexMediaDecisionEngine.h"

class PlexMediaDecisionJobFake : public CPlexMediaDecisionJob
{
public:
  PlexMediaDecisionJobFake(const CFileItem& item, const CFileItemPtr& resolvedItem = CFileItemPtr(),
                           const CFileItemPtr& indItem = CFileItemPtr())
    : CPlexMediaDecisionJob(item)
  {
    fakeResolveItem = resolvedItem;
    indirectItem = indItem;
  }

  CFileItemPtr GetUrl(const CStdString& url)
  {
    CURL u(url);
    if (indirectItem && boost::starts_with(u.GetFileName(), ":/plugins/com.plexapp.system/serviceFunction"))
    {
      if (url == indirectItem->GetProperty("indirectUrl").asString())
        return indirectItem;
      return CFileItemPtr();
    }

    EXPECT_TRUE(fakeResolveItem);

    return fakeResolveItem;
  }

  CFileItemPtr fakeResolveItem;
  CFileItemPtr indirectItem;
};

class PlexMediaDecisionEngineTest : public PlexServerManagerTestUtility
{
public:
  void SetUp()
  {
    origItem.SetPlexDirectoryType(PLEX_DIR_TYPE_VIDEO);
    origItem.SetProperty("unprocessed_key", "/library/metadata/123");
    origItem.SetPath("plexserver://abc123/library/foo");
    origItem.SetProperty("type", "movie");

    PlexServerManagerTestUtility::SetUp();
  }

  CFileItemPtr getDetailedItem(const CStdString& xml)
  {
    CFileItemList list;
    PlexTestUtils::listFromXML(xml, list);
    CFileItemPtr detailedItem = list.Get(0);

    EXPECT_TRUE(detailedItem);
    return detailedItem;
  }

  CFileItem origItem;
};

TEST_F(PlexMediaDecisionEngineTest, basicResolv)
{
  PlexMediaDecisionJobFake job(origItem, getDetailedItem(testItem_movie));

  EXPECT_TRUE(job.DoWork());
  CFileItem& media = job.m_choosenMedia;
  EXPECT_STREQ("1888", media.GetProperty("ratingKey").asString().c_str());
  EXPECT_STREQ("plexserver://abc123/library/parts/1950/file.mkv", media.GetPath());
}

TEST_F(PlexMediaDecisionEngineTest, channelResolv)
{
  CFileItemPtr indirectItem = getDetailedItem(testItem_channelTwitIndirect);
  CFileItemPtr channelItem = getDetailedItem(testItem_channelTwit);
  int selectedMediaItem = 1;

  indirectItem->SetProperty("indirectUrl", channelItem->m_mediaItems[selectedMediaItem]->m_mediaParts[0]->GetPath());
  origItem.SetProperty("selectedMediaItem", selectedMediaItem);

  PlexMediaDecisionJobFake job(origItem, channelItem, indirectItem);

  EXPECT_TRUE(job.DoWork());
  CFileItem& media = job.m_choosenMedia;
  EXPECT_STREQ(indirectItem->m_mediaItems[0]->m_mediaParts[0]->GetPath(), media.GetPath());
}

TEST_F(PlexMediaDecisionEngineTest, itunesResolve)
{
  CFileItemPtr item = getDetailedItem(testItem_channelItunesAlbum);
  EXPECT_TRUE(item->GetProperty("isSynthesized").asBoolean());

  PlexMediaDecisionJobFake job(*item);
  EXPECT_TRUE(job.DoWork());
  EXPECT_STREQ("plexserver://abc123/library/sections/1/all/219.mp3", job.m_choosenMedia.GetPath());
}
