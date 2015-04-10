/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <tuple>
#include <vector>
#include "utils/StreamDetails.h"
#include "video/VideoDimensions.h"

#include "gtest/gtest.h"

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
  video->m_strStereoMode = "left_right";

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
  EXPECT_EQ(2.39f, a.GetVideoAspect());
  EXPECT_EQ(1920, a.GetVideoWidth());
  EXPECT_EQ(1080, a.GetVideoHeight());
  EXPECT_EQ(30, a.GetVideoDuration());
  EXPECT_STREQ("left_right", a.GetStereoMode().c_str());
}

TEST(TestStreamDetails, VideoDimensionsToFidelityAndQuality)
{
  // Test data assembled from several different libraries using mediainfo
  std::vector<std::tuple<std::string, std::string, int, int>> dataProvider =
  {
    { std::make_tuple("480", "SD", 528, 224) },
    { std::make_tuple("480", "SD", 448, 256) },
    { std::make_tuple("480", "SD", 608, 256) },
    { std::make_tuple("480", "SD", 640, 256) },
    { std::make_tuple("480", "SD", 640, 272) },
    { std::make_tuple("480", "SD", 656, 272) },
    { std::make_tuple("480", "SD", 512, 288) },
    { std::make_tuple("480", "SD", 560, 304) },
    { std::make_tuple("480", "SD", 576, 304) },
    { std::make_tuple("480", "SD", 704, 304) },
    { std::make_tuple("480", "SD", 720, 304) },
    { std::make_tuple("480", "SD", 576, 320) },
    { std::make_tuple("480", "SD", 592, 320) },
    { std::make_tuple("480", "SD", 608, 320) },
    { std::make_tuple("480", "SD", 708, 328) },
    { std::make_tuple("480", "SD", 576, 336) },
    { std::make_tuple("480", "SD", 608, 336) },
    { std::make_tuple("480", "SD", 624, 336) },
    { std::make_tuple("480", "SD", 640, 336) },
    { std::make_tuple("480", "SD", 672, 336) },
    { std::make_tuple("480", "SD", 706, 344) },
    { std::make_tuple("480", "SD", 464, 352) },
    { std::make_tuple("480", "SD", 624, 352) },
    { std::make_tuple("480", "SD", 640, 352) },
    { std::make_tuple("480", "SD", 704, 352) },
    { std::make_tuple("480", "SD", 704, 358) },
    { std::make_tuple("480", "SD", 718, 360) },
    { std::make_tuple("480", "SD", 640, 368) },
    { std::make_tuple("480", "SD", 720, 368) },
    { std::make_tuple("480", "SD", 512, 384) },
    { std::make_tuple("480", "SD", 688, 384) },
    { std::make_tuple("480", "SD", 720, 384) },
    { std::make_tuple("480", "SD", 716, 396) },
    { std::make_tuple("480", "SD", 608, 400) },
    { std::make_tuple("480", "SD", 720, 400) },
    { std::make_tuple("480", "SD", 720, 416) },
    { std::make_tuple("480", "SD", 648, 428) },
    { std::make_tuple("480", "SD", 576, 432) },
    { std::make_tuple("480", "SD", 708, 444) },
    { std::make_tuple("480", "SD", 712, 460) },
    { std::make_tuple("480", "SD", 720, 462) },
    { std::make_tuple("480", "SD", 720, 464) },
    { std::make_tuple("480", "SD", 708, 468) },
    { std::make_tuple("480", "SD", 714, 470) },
    { std::make_tuple("480", "SD", 696, 472) },
    { std::make_tuple("480", "SD", 640, 480) },
    { std::make_tuple("480", "SD", 672, 480) },
    { std::make_tuple("480", "SD", 704, 480) },
    { std::make_tuple("480", "SD", 716, 480) },
    { std::make_tuple("480", "SD", 720, 480) },
    { std::make_tuple("576", "SD", 704, 528) },
    { std::make_tuple("576", "SD", 716, 548) },
    { std::make_tuple("576", "SD", 720, 552) },
    { std::make_tuple("576", "SD", 716, 560) },
    { std::make_tuple("576", "SD", 720, 566) },
    { std::make_tuple("576", "SD", 697, 576) },
    { std::make_tuple("576", "SD", 698, 576) },
    { std::make_tuple("576", "SD", 716, 576) },
    { std::make_tuple("576", "SD", 720, 576) },
    { std::make_tuple("576", "SD", 720, 592) },
    { std::make_tuple("720", "HD", 1280, 464) },
    { std::make_tuple("720", "HD", 1280, 528) },
    { std::make_tuple("720", "HD", 1280, 532) },
    { std::make_tuple("720", "HD", 1280, 534) },
    { std::make_tuple("720", "HD", 1280, 536) },
    { std::make_tuple("720", "HD", 1280, 538) },
    { std::make_tuple("720", "HD", 1280, 542) },
    { std::make_tuple("720", "HD", 1272, 544) },
    { std::make_tuple("720", "HD", 1280, 544) },
    { std::make_tuple("720", "HD", 1280, 546) },
    { std::make_tuple("720", "HD", 1280, 568) },
    { std::make_tuple("720", "HD", 1280, 572) },
    { std::make_tuple("720", "HD", 1280, 688) },
    { std::make_tuple("720", "HD", 1280, 690) },
    { std::make_tuple("720", "HD", 1280, 692) },
    { std::make_tuple("720", "HD", 1280, 694) },
    { std::make_tuple("720", "HD", 1280, 696) },
    { std::make_tuple("720", "HD", 1280, 714) },
    { std::make_tuple("720", "HD", 1280, 716) },
    { std::make_tuple("720", "HD", 1280, 718) },
    { std::make_tuple("720", "HD", 960, 720) },
    { std::make_tuple("720", "HD", 964, 720) },
    { std::make_tuple("720", "HD", 992, 720) },
    { std::make_tuple("720", "HD", 1194, 720) },
    { std::make_tuple("720", "HD", 1200, 720) },
    { std::make_tuple("720", "HD", 1264, 720) },
    { std::make_tuple("720", "HD", 1274, 720) },
    { std::make_tuple("720", "HD", 1280, 720) },
    { std::make_tuple("1080", "HD", 1920, 752) },
    { std::make_tuple("1080", "HD", 1920, 784) },
    { std::make_tuple("1080", "HD", 1916, 796) },
    { std::make_tuple("1080", "HD", 1920, 798) },
    { std::make_tuple("1080", "HD", 1918, 800) },
    { std::make_tuple("1080", "HD", 1920, 800) },
    { std::make_tuple("1080", "HD", 1920, 802) },
    { std::make_tuple("1080", "HD", 1920, 804) },
    { std::make_tuple("1080", "HD", 1920, 808) },
    { std::make_tuple("1080", "HD", 1920, 814) },
    { std::make_tuple("1080", "HD", 1920, 816) },
    { std::make_tuple("1080", "HD", 1920, 818) },
    { std::make_tuple("1080", "HD", 1916, 820) },
    { std::make_tuple("1080", "HD", 1920, 824) },
    { std::make_tuple("1080", "HD", 1920, 856) },
    { std::make_tuple("1080", "HD", 1844, 992) },
    { std::make_tuple("1080", "HD", 1920, 1032) },
    { std::make_tuple("1080", "HD", 1920, 1036) },
    { std::make_tuple("1080", "HD", 1920, 1038) },
    { std::make_tuple("1080", "HD", 1920, 1040) },
    { std::make_tuple("1080", "HD", 1080, 1040) },
    { std::make_tuple("1080", "HD", 1920, 1056) },
    { std::make_tuple("1080", "HD", 1920, 1072) },
    { std::make_tuple("1080", "HD", 1392, 1080) },
    { std::make_tuple("1080", "HD", 1424, 1080) },
    { std::make_tuple("1080", "HD", 1440, 1080) },
    { std::make_tuple("1080", "HD", 1488, 1080) },
    { std::make_tuple("1080", "HD", 1712, 1080) },
    { std::make_tuple("1080", "HD", 1792, 1080) },
    { std::make_tuple("1080", "HD", 1808, 1080) },
    { std::make_tuple("1080", "HD", 1824, 1080) },
    { std::make_tuple("1080", "HD", 1884, 1080) },
    { std::make_tuple("1080", "HD", 1920, 1080) },
    { std::make_tuple("1080", "HD", 3840, 1080) },
    { std::make_tuple("4K", "HD", 4096, 1714) },
    { std::make_tuple("4K", "HD", 4096, 1744) },
    { std::make_tuple("4K", "HD", 3840, 2076) },
    { std::make_tuple("4K", "HD", 3840, 2076) }
  };

  // Unpack each data set and check that the quality/fidelity matches
  for (const auto &tuple : dataProvider)
  {
    std::string quality;
    std::string fidelity;
    int width;
    int height;

    std::tie(quality, fidelity, width, height) = tuple;

    CVideoDimensions dimensions(width, height);
    EXPECT_STREQ(quality.c_str(), dimensions.GetQuality().c_str());
    EXPECT_STREQ(fidelity.c_str(), dimensions.GetFidelity().c_str());
  }
}

TEST(TestStreamDetails, VideoAspectToAspectDescription)
{
  EXPECT_STREQ("2.40", CStreamDetails::VideoAspectToAspectDescription(2.39f).c_str());
}
