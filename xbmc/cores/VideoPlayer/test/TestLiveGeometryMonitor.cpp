/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ServiceBroker.h"
#include "cores/DataCacheCore.h"
#include "cores/VideoPlayer/DVDCodecs/Video/DVDVideoCodec.h"
#include "cores/VideoPlayer/DVDMessage.h"
#include "cores/VideoPlayer/DVDMessageQueue.h"
#include "cores/VideoPlayer/DVDStreamInfo.h"
#include "cores/VideoPlayer/Interface/TimingConstants.h"
#include "cores/VideoPlayer/LiveGeometryMonitor.h"
#include "cores/VideoPlayer/Process/ProcessInfo.h"
#include "cores/VideoSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"

#include <algorithm>
#include <ctime>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace
{

/*!
 * \brief A picture whose planes are ordinary memory, which is what the monitor reads.
 *
 * The decoder path this stands in for is the only one live detection reads directly; a
 * hardware buffer with no planes takes the reduction path instead, which needs a device.
 */
class CTestVideoBuffer : public CVideoBuffer
{
public:
  CTestVideoBuffer(unsigned int width, unsigned int height) : CVideoBuffer(0), m_width(width)
  {
    m_pixFormat = AV_PIX_FMT_YUV420P;
    m_y.assign(static_cast<size_t>(width) * height, 16);
    m_u.assign(static_cast<size_t>(width / 2) * (height / 2), 128);
    m_v.assign(static_cast<size_t>(width / 2) * (height / 2), 128);
  }

  void GetPlanes(uint8_t* (&planes)[YuvImage::MAX_PLANES]) override
  {
    planes[0] = m_y.data();
    planes[1] = m_u.data();
    planes[2] = m_v.data();
  }

  void GetStrides(int (&strides)[YuvImage::MAX_PLANES]) override
  {
    strides[0] = static_cast<int>(m_width);
    strides[1] = static_cast<int>(m_width / 2);
    strides[2] = static_cast<int>(m_width / 2);
  }

  //! \brief Fill rows [y0, y1) of columns [x0, x1) with picture: mid grey, textured enough
  //! that the detector reads range in it rather than another bar.
  void Picture(unsigned int x0, unsigned int y0, unsigned int x1, unsigned int y1)
  {
    for (unsigned int y = y0; y < y1; ++y)
    {
      for (unsigned int x = x0; x < x1; ++x)
        m_y[static_cast<size_t>(y) * m_width + x] = static_cast<uint8_t>(60 + ((x + y * 7) % 160));
    }
  }

private:
  unsigned int m_width;
  std::vector<uint8_t> m_y;
  std::vector<uint8_t> m_u;
  std::vector<uint8_t> m_v;
};

/*!
 * \brief Describe \p picture as \p stereoMode carrying \p buffer, displayed at \p displayWidth
 *        by \p displayHeight.
 *
 * Filled in place because a VideoPicture cannot be copied - it owns a reference to its buffer,
 * and handing one out by value would release the buffer twice.
 */
void Describe(VideoPicture& picture,
              CTestVideoBuffer& buffer,
              unsigned int width,
              unsigned int height,
              const std::string& stereoMode,
              unsigned int displayWidth,
              unsigned int displayHeight)
{
  picture.iWidth = width;
  picture.iHeight = height;
  picture.iDisplayWidth = displayWidth;
  picture.iDisplayHeight = displayHeight;
  picture.stereoMode = stereoMode;
  picture.pixelFormat = AV_PIX_FMT_YUV420P;
  picture.color_range = 0;
  picture.colorBits = 8;

  // Held rather than owned: ~VideoPicture releases it, and a buffer with no pool is content to
  // be released as often as it is acquired.
  buffer.Acquire();
  picture.videoBuffer = &buffer;
}

class TestLiveGeometryMonitor : public ::testing::Test
{
protected:
  void SetUp() override
  {
    const auto settings = CServiceBroker::GetSettingsComponent()->GetSettings();
    m_extractWas = settings->GetBool(CSettings::SETTING_VIDEOSCREEN_EXTRACTCONTENTGEOMETRY);
    m_liveWas = settings->GetBool(CSettings::SETTING_VIDEOSCREEN_LIVECONTENTGEOMETRY);
    ASSERT_TRUE(settings->SetBool(CSettings::SETTING_VIDEOSCREEN_EXTRACTCONTENTGEOMETRY, true));
    ASSERT_TRUE(settings->SetBool(CSettings::SETTING_VIDEOSCREEN_LIVECONTENTGEOMETRY, true));

    CServiceBroker::GetDataCacheCore().GetPlayTimes(m_startWas, m_currentWas, m_minWas, m_maxWas);

    m_queue.Init();
    m_processInfo.reset(CProcessInfo::CreateInstance());
    m_monitor = std::make_unique<CLiveGeometryMonitor>(m_queue, *m_processInfo);
  }

  void TearDown() override
  {
    const auto settings = CServiceBroker::GetSettingsComponent()->GetSettings();
    settings->SetBool(CSettings::SETTING_VIDEOSCREEN_EXTRACTCONTENTGEOMETRY, m_extractWas);
    settings->SetBool(CSettings::SETTING_VIDEOSCREEN_LIVECONTENTGEOMETRY, m_liveWas);

    CServiceBroker::GetDataCacheCore().SetPlayTimes(m_startWas, m_currentWas, m_minWas, m_maxWas);
  }

  //! \brief Turn live detection off the way a viewer does, before the stream is opened - the
  //! settings are re-read on a cadence, and opening a stream is what forces the first read.
  void TurnLiveDetectionOff()
  {
    ASSERT_TRUE(CServiceBroker::GetSettingsComponent()->GetSettings()->SetBool(
        CSettings::SETTING_VIDEOSCREEN_LIVECONTENTGEOMETRY, false));
  }

  //! \brief Put the playhead \p seconds into a title \p durationSeconds long. This is what the
  //! lead-in and lead-out exclusions are measured against, and so what decides whether a shape
  //! served now is remembered for the title.
  void PlayheadAt(double seconds, double durationSeconds)
  {
    CServiceBroker::GetDataCacheCore().SetPlayTimes(0, static_cast<int64_t>(seconds * 1000.0), 0,
                                                    static_cast<int64_t>(durationSeconds * 1000.0));
  }

  //! \brief Open a stream that live detection is permitted on.
  void OpenStream()
  {
    CDVDStreamInfo hint;
    hint.liveContentGeometry = true;
    m_monitor->OnStreamOpened(hint);
  }

  //! \brief The suite's standard frame: a mono 1080p picture letterboxed to its 2.35 body.
  //! Owned by the fixture, because a VideoPicture cannot be copied out.
  VideoPicture& LetterboxedMono()
  {
    m_frameBuffer.Picture(0, 132, 1920, 948);
    Describe(m_framePicture, m_frameBuffer, 1920, 1080, "mono", 1920, 1080);
    return m_framePicture;
  }

  CRectInt Feed(const VideoPicture& picture)
  {
    CDVDStreamInfo hints;
    return m_monitor->OnPicture(picture, hints, DVD_PLAYSPEED_NORMAL);
  }

  //! \brief Everything the monitor has posted since this was last called, in order. Draining,
  //! so a test that reads the queue between two frames sees only what the second one sent.
  std::vector<LiveGeometryUpdate> Drain()
  {
    std::vector<LiveGeometryUpdate> updates;
    std::shared_ptr<CDVDMsg> message;
    while (m_queue.Get(message, std::chrono::milliseconds(0)) == MSGQ_OK)
    {
      if (message->IsType(CDVDMsg::PLAYER_CONTENT_GEOMETRY))
        updates.push_back(
            std::static_pointer_cast<CDVDMsgType<LiveGeometryUpdate>>(message)->m_value);
    }
    return updates;
  }

  //! \brief The last shape the monitor posted, ignoring the withdrawals a stream change sends.
  std::optional<LiveGeometryUpdate> LastPosted()
  {
    std::optional<LiveGeometryUpdate> last;
    for (const LiveGeometryUpdate& update : Drain())
    {
      if (!update.clear)
        last = update;
    }
    return last;
  }

  CDVDMessageQueue m_queue{"test"};
  std::unique_ptr<CProcessInfo> m_processInfo;
  std::unique_ptr<CLiveGeometryMonitor> m_monitor;

  //! \brief The buffer before the picture, so the picture releases its hold first.
  CTestVideoBuffer m_frameBuffer{1920, 1080};
  VideoPicture m_framePicture;
  bool m_extractWas{false};
  bool m_liveWas{false};

  // The data cache is the suite's, not this fixture's, and the playhead is read from it.
  time_t m_startWas{0};
  int64_t m_currentWas{0};
  int64_t m_minWas{0};
  int64_t m_maxWas{0};
};

//! \brief The ratio a served rectangle names, which is what the room is driven from - the
//! rectangle is in coded pixels and those are not square on a packed stereoscopic frame.
float DisplayRatio(const CRectInt& rect, float par)
{
  return rect.Height() > 0 ? static_cast<float>(rect.Width()) * par / rect.Height() : 0.0f;
}

} // namespace

//! The baseline, and the proof that the harness reads a frame at all: a plainly letterboxed
//! 16:9 frame with square pixels is served as the scope shape it is.
TEST_F(TestLiveGeometryMonitor, ALetterboxedFrameIsServedAsItsRatio)
{
  VideoPicture& picture = LetterboxedMono();

  OpenStream();
  const CRectInt inForce = Feed(picture);

  const std::optional<LiveGeometryUpdate> posted = LastPosted();
  ASSERT_TRUE(posted.has_value()) << "nothing was served for a frame that plainly has bars";
  EXPECT_EQ(posted->rect, inForce) << "the picture was stamped with something else";
  EXPECT_EQ(1920, posted->rect.Width());
  EXPECT_NEAR(2.35f, DisplayRatio(posted->rect, 1.0f), 0.01f);
}

/*!
 * The defect this file was written for. On a half side-by-side frame the region measured is one
 * 960-wide view, and that view is displayed at the whole frame's 16:9 - so its pixels are twice
 * as wide and a 960x816 picture in it is 2.35, not 1.18. Taking the pixel aspect from the
 * packing instead classified the same picture as Movietone, which is a real entry and so was
 * not refused - it was served, and the room was driven to it.
 */
TEST_F(TestLiveGeometryMonitor, AHalfSideBySideViewIsReadAtItsOwnPixelAspect)
{
  CTestVideoBuffer buffer(1920, 1080);
  buffer.Picture(0, 132, 960, 948); // left view
  buffer.Picture(960, 132, 1920, 948); // right view

  VideoPicture picture;
  Describe(picture, buffer, 1920, 1080, "left_right", 1920, 1080);

  OpenStream();
  const CRectInt inForce = Feed(picture);

  const std::optional<LiveGeometryUpdate> posted = LastPosted();
  ASSERT_TRUE(posted.has_value()) << "nothing was served for one view of a packed frame";
  EXPECT_EQ(posted->rect, inForce);
  EXPECT_EQ(960, posted->rect.Width()) << "the packing was measured rather than the view";
  EXPECT_NEAR(2.35f, DisplayRatio(posted->rect, 2.0f), 0.01f)
      << "the view was read at the packing's pixel aspect";
}

//! Nothing may be read from an item live detection is not permitted on - the policy rides in
//! on the hint, and the monitor is dormant rather than merely quiet.
TEST_F(TestLiveGeometryMonitor, AStreamThatDoesNotPermitReadingIsNotRead)
{
  VideoPicture& picture = LetterboxedMono();

  CDVDStreamInfo hint;
  hint.liveContentGeometry = false;
  m_monitor->OnStreamOpened(hint);

  EXPECT_TRUE(Feed(picture).IsEmpty());
  EXPECT_FALSE(LastPosted().has_value());
}

//! Trick play scrubs across the timeline, and the shape must not chase it.
TEST_F(TestLiveGeometryMonitor, NothingIsReadDuringTrickPlay)
{
  VideoPicture& picture = LetterboxedMono();

  OpenStream();

  CDVDStreamInfo hints;
  EXPECT_TRUE(m_monitor->OnPicture(picture, hints, DVD_PLAYSPEED_NORMAL * 4).IsEmpty());
  EXPECT_FALSE(LastPosted().has_value());
}

//! The stream permits reading and the viewer has said not to. Distinct from the hint above:
//! that one is a property of the item, this one is the setting behind the whole feature, and
//! it is read on a cadence rather than at the call - so what is being pinned is that opening a
//! stream forces the read rather than inheriting whatever the last title left behind.
TEST_F(TestLiveGeometryMonitor, NothingIsReadWhileTheViewerHasLiveDetectionOff)
{
  VideoPicture& picture = LetterboxedMono();

  TurnLiveDetectionOff();
  OpenStream();

  EXPECT_TRUE(Feed(picture).IsEmpty());
  EXPECT_FALSE(LastPosted().has_value());
}

//! A ratio declared by hand outranks every measurement, so a reading could not change what is
//! served - and taking one anyway spends a frame access on a value nothing will look at.
TEST_F(TestLiveGeometryMonitor, NothingIsReadWhileADeclaredRatioPinsTheShape)
{
  VideoPicture& picture = LetterboxedMono();

  CVideoSettings settings;
  settings.m_declaredAspect = 2.35f;
  m_processInfo->SetVideoSettings(settings);

  OpenStream();

  EXPECT_TRUE(Feed(picture).IsEmpty());
  EXPECT_FALSE(LastPosted().has_value());
}

//! A shape once served is stamped onto every picture that follows it, including the ones no
//! reading was taken from - that is what carries it to the renderer.
TEST_F(TestLiveGeometryMonitor, TheShapeInForceIsStampedOnEveryPicture)
{
  VideoPicture& picture = LetterboxedMono();

  OpenStream();
  const CRectInt served = Feed(picture);
  ASSERT_FALSE(served.IsEmpty());

  CDVDStreamInfo hints;
  EXPECT_EQ(served, m_monitor->OnPicture(picture, hints, DVD_PLAYSPEED_NORMAL * 4))
      << "a picture read from nothing still carries what is in force";
}

/*!
 * A shape is measured against the frame it was found in, so when the frame changes underneath
 * it - a resolution switch, or a stereoscopic stream changing packing - what was served stops
 * describing anything. It has to be withdrawn rather than left standing until a new reading
 * happens to replace it, because in between it is driving the room to the old stream's shape.
 */
TEST_F(TestLiveGeometryMonitor, AResolutionChangeWithdrawsWhatTheOldStreamServed)
{
  CTestVideoBuffer wide(1920, 1080);
  wide.Picture(0, 132, 1920, 948);

  VideoPicture first;
  Describe(first, wide, 1920, 1080, "mono", 1920, 1080);

  OpenStream();
  ASSERT_FALSE(Feed(first).IsEmpty()) << "nothing was served to withdraw";
  Drain();

  CTestVideoBuffer narrow(1280, 720);
  narrow.Picture(0, 88, 1280, 632);

  VideoPicture second;
  Describe(second, narrow, 1280, 720, "mono", 1280, 720);

  Feed(second);

  const std::vector<LiveGeometryUpdate> updates = Drain();
  EXPECT_TRUE(std::any_of(updates.begin(), updates.end(),
                          [](const LiveGeometryUpdate& update) { return update.clear; }))
      << "the old stream's shape was left standing over the new one";
}

/*!
 * The opening exclusion, and the reason it exists: a distributor ident is a real shape, held
 * long enough to confirm, and it is not the film's. It is served while it is on screen - the
 * room should follow what is actually being shown - but it must not enter the title's permanent
 * record, or every later playback opens to the ident's shape.
 */
TEST_F(TestLiveGeometryMonitor, AShapeFoundInTheLeadInIsServedButNotRemembered)
{
  VideoPicture& picture = LetterboxedMono();

  PlayheadAt(30.0, 2700.0); // half a minute into a 45 minute title, inside the two minute lead-in
  OpenStream();
  ASSERT_FALSE(Feed(picture).IsEmpty());

  const std::optional<LiveGeometryUpdate> posted = LastPosted();
  ASSERT_TRUE(posted.has_value()) << "the shape on screen was not served";
  EXPECT_TRUE(posted->found.empty()) << "an ident's shape was written into the title's record";
}

//! The same reading, past the exclusion: this one is the film, and it is what the next playback
//! opens to.
TEST_F(TestLiveGeometryMonitor, AShapeFoundInTheBodyIsRemembered)
{
  VideoPicture& picture = LetterboxedMono();

  PlayheadAt(1200.0, 2700.0);
  OpenStream();
  ASSERT_FALSE(Feed(picture).IsEmpty());

  const std::optional<LiveGeometryUpdate> posted = LastPosted();
  ASSERT_TRUE(posted.has_value());
  ASSERT_EQ(1u, posted->found.size()) << "the film's own shape was not remembered";
  EXPECT_EQ(posted->rect, posted->found.front());
}
