#include "gtest/gtest.h"
#include "FileItem.h"

#include "PlexUtils.h"

static CFileItemPtr getSubtitleStream()
{
  CFileItemPtr item(new CFileItem);
  item->SetProperty("streamType", PLEX_STREAM_SUBTITLE);
  item->SetProperty("language", "English");
  item->SetProperty("format", "srt");
  return item;
}

static CFileItemPtr getAudioStream()
{
  CFileItemPtr item(new CFileItem);
  item->SetProperty("streamType", PLEX_STREAM_AUDIO);
  item->SetProperty("language", "English");
  return item;
}

TEST(PlexUtilsGetPrettyStreamNameFromStreamItem, subtitleBasic)
{
  CFileItemPtr item = getSubtitleStream();
  EXPECT_STREQ(PlexUtils::GetPrettyStreamNameFromStreamItem(item), "English (SRT)");
}

TEST(PlexUtilsGetPrettyStreamNameFromStreamItem, subtitleForced)
{
  CFileItemPtr item = getSubtitleStream();
  item->SetProperty("forced", true);
  // this should really say SRT Forced, but since our strings are not loaded it will
  // be "" instead, we can detect the correct behavior by setting a space there. Stupid
  // and should be fixed
  EXPECT_STREQ(PlexUtils::GetPrettyStreamNameFromStreamItem(item), "English (SRT )");
}

TEST(PlexUtilsGetPrettyStreamNameFromStreamItem, subtitleNoFormat)
{
  CFileItemPtr item = getSubtitleStream();
  item->SetProperty("format", "");
  EXPECT_STREQ(PlexUtils::GetPrettyStreamNameFromStreamItem(item), "English");
}

TEST(PlexUtilsGetPrettyStreamNameFromStreamItem, subtitleNoFormatButCodec)
{
  CFileItemPtr item = getSubtitleStream();
  item->SetProperty("format", "");
  item->SetProperty("codec", "srt");
  EXPECT_STREQ(PlexUtils::GetPrettyStreamNameFromStreamItem(item), "English (SRT)");
}

TEST(PlexUtilsGetPrettyStreamNameFromStreamItem, subtitleCodecAndForced)
{
  CFileItemPtr item = getSubtitleStream();
  item->SetProperty("format", "");
  item->SetProperty("codec", "srt");
  item->SetProperty("forced", true);
  EXPECT_STREQ(PlexUtils::GetPrettyStreamNameFromStreamItem(item), "English (SRT )");
}

TEST(PlexUtilsGetPrettyStreamNameFromStreamItem, subtitleNoLanguage)
{
  CFileItemPtr item = getSubtitleStream();
  item->SetProperty("language", "");
  EXPECT_STREQ(PlexUtils::GetPrettyStreamNameFromStreamItem(item), "Unknown (SRT)");
}

TEST(PlexUtilsGetPrettyStreamNameFromStreamItem, subtitleNoLanguageForced)
{
  CFileItemPtr item = getSubtitleStream();
  item->SetProperty("language", "");
  item->SetProperty("forced", true);
  EXPECT_STREQ(PlexUtils::GetPrettyStreamNameFromStreamItem(item), "Unknown (SRT )");
}

TEST(PlexUtilsGetPrettyStreamNameFromStreamItem, audioBasic)
{
  CFileItemPtr item = getAudioStream();
  EXPECT_STREQ(PlexUtils::GetPrettyStreamNameFromStreamItem(item), "English");
}

TEST(PlexUtilsGetPrettyStreamNameFromStreamItem, audioChannels)
{
  CFileItemPtr item = getAudioStream();
  item->SetProperty("channels", "6");
  EXPECT_STREQ(PlexUtils::GetPrettyStreamNameFromStreamItem(item), "English (5.1)");
}

TEST(PlexUtilsGetPrettyStreamNameFromStreamItem, audioCodec)
{
  CFileItemPtr item = getAudioStream();
  item->SetProperty("codec", "ac3");
  EXPECT_STREQ(PlexUtils::GetPrettyStreamNameFromStreamItem(item), "English (AC3)");
}

TEST(PlexUtilsGetPrettyStreamNameFromStreamItem, audioCodecAndChannels)
{
  CFileItemPtr item = getAudioStream();
  item->SetProperty("codec", "ac3");
  item->SetProperty("channels", "6");
  EXPECT_STREQ(PlexUtils::GetPrettyStreamNameFromStreamItem(item), "English (AC3 5.1)");
}

TEST(PlexUtilsGetPrettyStreamNameFromStreamItem, audioUnknownLang)
{
  CFileItemPtr item = getAudioStream();
  item->ClearProperty("language");
  item->SetProperty("codec", "ac3");
  item->SetProperty("channels", "6");
  EXPECT_STREQ(PlexUtils::GetPrettyStreamNameFromStreamItem(item), "Unknown (AC3 5.1)");
}

#define EXPECT_TYPE(a, b)                                                                          \
  {                                                                                                \
    CFileItem item;                                                                                \
    item.SetPlexDirectoryType(a);                                                                  \
    EXPECT_EQ(PlexUtils::GetMediaTypeFromItem(item), b);                                           \
  }

TEST(PlexUtilsGetMediaTypeFromItem, music)
{
  EXPECT_TYPE(PLEX_DIR_TYPE_ALBUM, PLEX_MEDIA_TYPE_MUSIC);
  EXPECT_TYPE(PLEX_DIR_TYPE_ARTIST, PLEX_MEDIA_TYPE_MUSIC);
  EXPECT_TYPE(PLEX_DIR_TYPE_TRACK, PLEX_MEDIA_TYPE_MUSIC);
}

TEST(PlexUtilsGetMediaTypeFromItem, video)
{
  EXPECT_TYPE(PLEX_DIR_TYPE_VIDEO, PLEX_MEDIA_TYPE_VIDEO);
  EXPECT_TYPE(PLEX_DIR_TYPE_EPISODE, PLEX_MEDIA_TYPE_VIDEO);
  EXPECT_TYPE(PLEX_DIR_TYPE_CLIP, PLEX_MEDIA_TYPE_VIDEO);
  EXPECT_TYPE(PLEX_DIR_TYPE_MOVIE, PLEX_MEDIA_TYPE_VIDEO);
  EXPECT_TYPE(PLEX_DIR_TYPE_SEASON, PLEX_MEDIA_TYPE_VIDEO);
  EXPECT_TYPE(PLEX_DIR_TYPE_SHOW, PLEX_MEDIA_TYPE_VIDEO);
}

TEST(PlexUtilsGetMediaTypeFromItem, photo)
{
  EXPECT_TYPE(PLEX_DIR_TYPE_IMAGE, PLEX_MEDIA_TYPE_PHOTO);
  EXPECT_TYPE(PLEX_DIR_TYPE_PHOTO, PLEX_MEDIA_TYPE_PHOTO);
  EXPECT_TYPE(PLEX_DIR_TYPE_PHOTOALBUM, PLEX_MEDIA_TYPE_PHOTO);
}
