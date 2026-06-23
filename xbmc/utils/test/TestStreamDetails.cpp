/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "cores/VideoPlayer/Interface/StreamInfo.h"
#include "filesystem/File.h"
#include "test/TestUtils.h"
#include "utils/Archive.h"
#include "utils/StreamDetails.h"
#include "utils/Variant.h"

#include <gtest/gtest.h>

namespace
{
void ArchiveRoundTrip(CStreamDetails& source, CStreamDetails& loaded)
{
  XFILE::CFile* file = XBMC_CREATETEMPFILE(".ar");
  ASSERT_NE(file, nullptr);

  CArchive arstore(file, CArchive::store);
  arstore << source;
  arstore.Close();

  ASSERT_EQ(0, file->Seek(0, SEEK_SET));
  CArchive arload(file, CArchive::load);
  arload >> loaded;
  arload.Close();

  EXPECT_TRUE(XBMC_DELETETEMPFILE(file));
}

void AddArchiveTestStreams(CStreamDetails& details, const std::string& profile, bool profileScanned)
{
  auto* video = new CStreamDetailVideo();
  video->m_strCodec = "hevc";
  video->m_strProfile = profile;
  video->m_bProfileScanned = profileScanned;
  video->m_fAspect = 1.78f;
  video->m_iHeight = 1080;
  video->m_iWidth = 1920;
  video->m_iDuration = 30;
  video->m_strStereoMode = "left_right";
  video->m_strLanguage = "eng";
  video->m_strHdrType = "hdr10";
  video->m_strHdrDetail = "maxcll";
  details.AddStream(video);

  auto* audio = new CStreamDetailAudio();
  audio->m_strCodec = "aac";
  audio->m_strLanguage = "eng";
  audio->m_iChannels = 6;
  details.AddStream(audio);

  auto* subtitle = new CStreamDetailSubtitle();
  subtitle->m_strLanguage = "spa";
  details.AddStream(subtitle);
  details.DetermineBestStreams();
}
} // namespace

TEST(TestStreamDetails, General)
{
  CStreamDetails a;
  CStreamDetailVideo *video = new CStreamDetailVideo();
  CStreamDetailAudio *audio = new CStreamDetailAudio();
  CStreamDetailSubtitle *subtitle = new CStreamDetailSubtitle();

  video->m_iWidth = 1920;
  video->m_iHeight = 1080;
  video->m_fAspect = 2.39f;
  video->m_iDuration = 30;
  video->m_strCodec = "h264";
  video->m_strProfile = "High";
  video->m_strStereoMode = "left_right";
  video->m_strLanguage = "eng";

  audio->m_iChannels = 2;
  audio->m_strCodec = "aac";
  audio->m_strLanguage = "eng";

  subtitle->m_strLanguage = "eng";

  a.AddStream(video);
  a.AddStream(audio);

  EXPECT_TRUE(a.HasItems());

  EXPECT_EQ(1, a.GetStreamCount(CStreamDetail::VIDEO));
  EXPECT_EQ(1, a.GetVideoStreamCount());
  EXPECT_STREQ("", a.GetVideoCodec().c_str());
  EXPECT_STREQ("", a.GetVideoProfile().c_str());
  EXPECT_EQ(0.0f, a.GetVideoAspect());
  EXPECT_EQ(0, a.GetVideoWidth());
  EXPECT_EQ(0, a.GetVideoHeight());
  EXPECT_EQ(0, a.GetVideoDuration());
  EXPECT_STREQ("", a.GetStereoMode().c_str());

  EXPECT_EQ(1, a.GetStreamCount(CStreamDetail::AUDIO));
  EXPECT_EQ(1, a.GetAudioStreamCount());

  EXPECT_EQ(0, a.GetStreamCount(CStreamDetail::SUBTITLE));
  EXPECT_EQ(0, a.GetSubtitleStreamCount());

  a.AddStream(subtitle);
  EXPECT_EQ(1, a.GetStreamCount(CStreamDetail::SUBTITLE));
  EXPECT_EQ(1, a.GetSubtitleStreamCount());

  a.DetermineBestStreams();
  EXPECT_STREQ("h264", a.GetVideoCodec().c_str());
  EXPECT_STREQ("High", a.GetVideoProfile().c_str());
  EXPECT_EQ(2.39f, a.GetVideoAspect());
  EXPECT_EQ(1920, a.GetVideoWidth());
  EXPECT_EQ(1080, a.GetVideoHeight());
  EXPECT_EQ(30, a.GetVideoDuration());
  EXPECT_STREQ("left_right", a.GetStereoMode().c_str());
}

TEST(TestStreamDetails, EmptyVideoProfile)
{
  CStreamDetails details;
  details.AddStream(new CStreamDetailVideo());
  details.DetermineBestStreams();

  EXPECT_EQ(details.GetVideoProfile(), "");
  EXPECT_FALSE(details.HasVideoProfileScanned());
  EXPECT_TRUE(details.HasUnscannedVideoProfile());
}

TEST(TestStreamDetails, SerializeVideoProfile)
{
  CStreamDetailVideo video;
  video.m_strProfile = "Main 10";

  CVariant value;
  video.Serialize(value);

  EXPECT_EQ(value["profile"].asString(), "Main 10");
}

TEST(TestStreamDetails, ScannedEmptyVideoProfile)
{
  CStreamDetails details;
  auto* video = new CStreamDetailVideo();
  video->m_bProfileScanned = true;
  details.AddStream(video);
  details.DetermineBestStreams();

  EXPECT_EQ(details.GetVideoProfile(), "");
  EXPECT_TRUE(details.HasVideoProfileScanned());
  EXPECT_FALSE(details.HasUnscannedVideoProfile());
}

TEST(TestStreamDetails, ScannedNonEmptyVideoProfile)
{
  CStreamDetails details;
  auto* video = new CStreamDetailVideo();
  video->m_strProfile = "Main 10";
  video->m_bProfileScanned = true;
  details.AddStream(video);
  details.DetermineBestStreams();

  EXPECT_EQ(details.GetVideoProfile(), "Main 10");
  EXPECT_TRUE(details.HasVideoProfileScanned());
  EXPECT_FALSE(details.HasUnscannedVideoProfile());
}

TEST(TestStreamDetails, VideoStreamInfoPreservesProfile)
{
  VideoStreamInfo info;
  info.profileName = "Main 10";

  CStreamDetailVideo video(info);

  EXPECT_EQ(video.m_strProfile, "Main 10");
  EXPECT_TRUE(video.m_bProfileScanned);
}

TEST(TestStreamDetails, UpdateMissingVideoProfilesFromExtractedDetails)
{
  CStreamDetails nfo;
  auto* nfoVideo = new CStreamDetailVideo();
  nfoVideo->m_strCodec = "hevc";
  nfoVideo->m_iWidth = 1280;
  nfoVideo->m_iHeight = 720;
  nfoVideo->m_strProfile.clear();
  nfoVideo->m_bProfileScanned = false;
  nfo.AddStream(nfoVideo);
  nfo.DetermineBestStreams();

  CStreamDetails extracted;
  auto* extractedVideo = new CStreamDetailVideo();
  extractedVideo->m_strCodec = "h264";
  extractedVideo->m_iWidth = 3840;
  extractedVideo->m_iHeight = 2160;
  extractedVideo->m_strProfile = "Main 10";
  extractedVideo->m_bProfileScanned = true;
  extracted.AddStream(extractedVideo);
  extracted.DetermineBestStreams();

  EXPECT_TRUE(nfo.UpdateMissingVideoProfilesFrom(extracted));
  EXPECT_EQ(nfo.GetVideoCodec(), "hevc");
  EXPECT_EQ(nfo.GetVideoWidth(), 1280);
  EXPECT_EQ(nfo.GetVideoHeight(), 720);
  EXPECT_EQ(nfo.GetVideoProfile(), "Main 10");
  EXPECT_TRUE(nfo.HasVideoProfileScanned());
}

TEST(TestStreamDetails, UpdateMissingVideoProfilesDoesNotOverwriteScannedProfile)
{
  CStreamDetails nfo;
  auto* nfoVideo = new CStreamDetailVideo();
  nfoVideo->m_strProfile = "";
  nfoVideo->m_bProfileScanned = true;
  nfo.AddStream(nfoVideo);
  nfo.DetermineBestStreams();

  CStreamDetails extracted;
  auto* extractedVideo = new CStreamDetailVideo();
  extractedVideo->m_strProfile = "Main 10";
  extractedVideo->m_bProfileScanned = true;
  extracted.AddStream(extractedVideo);
  extracted.DetermineBestStreams();

  EXPECT_FALSE(nfo.UpdateMissingVideoProfilesFrom(extracted));
  EXPECT_EQ(nfo.GetVideoProfile(), "");
  EXPECT_TRUE(nfo.HasVideoProfileScanned());
}

TEST(TestStreamDetails, ArchiveRoundTripPreservesExistingFields)
{
  CStreamDetails source;
  AddArchiveTestStreams(source, "Main 10", true);

  CStreamDetails loaded;
  ArchiveRoundTrip(source, loaded);

  EXPECT_EQ(loaded.GetVideoCodec(), "hevc");
  EXPECT_EQ(loaded.GetVideoProfile(), "Main 10");
  EXPECT_TRUE(loaded.HasVideoProfileScanned());
  EXPECT_FLOAT_EQ(loaded.GetVideoAspect(), 1.78f);
  EXPECT_EQ(loaded.GetVideoHeight(), 1080);
  EXPECT_EQ(loaded.GetVideoWidth(), 1920);
  EXPECT_EQ(loaded.GetVideoDuration(), 30);
  EXPECT_EQ(loaded.GetStereoMode(), "left_right");
  EXPECT_EQ(loaded.GetVideoLanguage(), "eng");
  EXPECT_EQ(loaded.GetVideoHdrType(), "hdr10");
  EXPECT_EQ(loaded.GetVideoHdrDetail(), "maxcll");
  EXPECT_EQ(loaded.GetAudioCodec(), "aac");
  EXPECT_EQ(loaded.GetAudioLanguage(), "eng");
  EXPECT_EQ(loaded.GetAudioChannels(), 6);
  EXPECT_EQ(loaded.GetSubtitleLanguage(), "spa");
}

TEST(TestStreamDetails, ArchiveRoundTripPreservesScannedEmptyVideoProfile)
{
  CStreamDetails source;
  AddArchiveTestStreams(source, "", true);

  CStreamDetails loaded;
  ArchiveRoundTrip(source, loaded);

  EXPECT_EQ(loaded.GetVideoProfile(), "");
  EXPECT_TRUE(loaded.HasVideoProfileScanned());
  EXPECT_FALSE(loaded.HasUnscannedVideoProfile());
}

TEST(TestStreamDetails, ArchiveRoundTripPreservesUnknownVideoProfile)
{
  CStreamDetails source;
  AddArchiveTestStreams(source, "", false);

  CStreamDetails loaded;
  ArchiveRoundTrip(source, loaded);

  EXPECT_EQ(loaded.GetVideoProfile(), "");
  EXPECT_FALSE(loaded.HasVideoProfileScanned());
  EXPECT_TRUE(loaded.HasUnscannedVideoProfile());
}

TEST(TestStreamDetails, LegacyArchiveLoadsExistingFieldsWithUnknownVideoProfile)
{
  XFILE::CFile* file = XBMC_CREATETEMPFILE(".ar");
  ASSERT_NE(file, nullptr);

  CArchive arstore(file, CArchive::store);
  arstore << 3;
  arstore << static_cast<int>(CStreamDetail::VIDEO);
  arstore << std::string("hevc");
  arstore << 1.78f;
  arstore << 1080;
  arstore << 1920;
  arstore << 30;
  arstore << std::string("left_right");
  arstore << std::string("eng");
  arstore << std::string("hdr10");
  arstore << std::string("maxcll");
  arstore << static_cast<int>(CStreamDetail::AUDIO);
  arstore << std::string("aac");
  arstore << std::string("eng");
  arstore << 6;
  arstore << static_cast<int>(CStreamDetail::SUBTITLE);
  arstore << std::string("spa");
  arstore.Close();

  ASSERT_EQ(0, file->Seek(0, SEEK_SET));
  CStreamDetails loaded;
  CArchive arload(file, CArchive::load);
  arload >> loaded;
  arload.Close();

  EXPECT_EQ(loaded.GetVideoCodec(), "hevc");
  EXPECT_EQ(loaded.GetVideoProfile(), "");
  EXPECT_FALSE(loaded.HasVideoProfileScanned());
  EXPECT_TRUE(loaded.HasUnscannedVideoProfile());
  EXPECT_FLOAT_EQ(loaded.GetVideoAspect(), 1.78f);
  EXPECT_EQ(loaded.GetVideoHeight(), 1080);
  EXPECT_EQ(loaded.GetVideoWidth(), 1920);
  EXPECT_EQ(loaded.GetVideoDuration(), 30);
  EXPECT_EQ(loaded.GetStereoMode(), "left_right");
  EXPECT_EQ(loaded.GetVideoLanguage(), "eng");
  EXPECT_EQ(loaded.GetVideoHdrType(), "hdr10");
  EXPECT_EQ(loaded.GetVideoHdrDetail(), "maxcll");
  EXPECT_EQ(loaded.GetAudioCodec(), "aac");
  EXPECT_EQ(loaded.GetAudioLanguage(), "eng");
  EXPECT_EQ(loaded.GetAudioChannels(), 6);
  EXPECT_EQ(loaded.GetSubtitleLanguage(), "spa");
  EXPECT_TRUE(XBMC_DELETETEMPFILE(file));
}

TEST(TestStreamDetails, EqualityDetectsVideoProfileDifference)
{
  CStreamDetails left;
  auto* leftVideo = new CStreamDetailVideo();
  leftVideo->m_strCodec = "hevc";
  leftVideo->m_strProfile = "Main";
  left.AddStream(leftVideo);

  CStreamDetails right;
  auto* rightVideo = new CStreamDetailVideo();
  rightVideo->m_strCodec = "hevc";
  rightVideo->m_strProfile = "Main 10";
  right.AddStream(rightVideo);

  EXPECT_NE(left, right);
}

TEST(TestStreamDetails, VideoDimsToResolutionDescription)
{
  EXPECT_STREQ("1080",
               CStreamDetails::VideoDimsToResolutionDescription(1920, 1080).c_str());
}

TEST(TestStreamDetails, VideoAspectToAspectDescription)
{
  EXPECT_STREQ("2.40", CStreamDetails::VideoAspectToAspectDescription(2.39f).c_str());
}
