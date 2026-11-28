/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "ServiceBroker.h"
#include "messaging/ApplicationMessenger.h"
#include "messaging/IMessageTarget.h"
#include "pvr/PVRPlaybackState.h"
#include "utils/ContentUtils.h"

#include <memory>

#include <gtest/gtest.h>

using namespace KODI::MESSAGING;

namespace
{
class CPlayRequests : public IMessageTarget
{
public:
  int GetMessageMask() override { return TMSG_MASK_PLAYLISTPLAYER; }

  void OnApplicationMessage(ThreadMessage* msg) override
  {
    if (msg->dwMessage != TMSG_MEDIA_PLAY)
      return;

    delete static_cast<CFileItem*>(msg->lpVoid);
    ++m_count;
  }

  int m_count{0};
};
} // namespace

class TestPVRPlaybackState : public testing::Test
{
protected:
  void SetUp() override
  {
    const auto messenger{std::make_shared<CApplicationMessenger>()};
    messenger->RegisterReceiver(&m_playRequests);
    CServiceBroker::RegisterAppMessenger(messenger);
  }

  void TearDown() override { CServiceBroker::RegisterAppMessenger(m_previousMessenger); }

  int PlayRequests()
  {
    CServiceBroker::GetAppMessenger()->ProcessMessages();
    return m_playRequests.m_count;
  }

  CPlayRequests m_playRequests;
  std::shared_ptr<CApplicationMessenger> m_previousMessenger{CServiceBroker::GetAppMessenger()};
};

TEST_F(TestPVRPlaybackState, BackendChannelChangePlaysNothingWhileNothingIsPlaying)
{
  const PVR::CPVRPlaybackState state;

  auto item{std::make_unique<CFileItem>("special://temp/video.mkv", false)};
  state.StartPlayback(item, ContentUtils::PlayMode::CHECK_AUTO_PLAY_NEXT_ITEM);
  ASSERT_EQ(1, PlayRequests()) << "a playback request must be visible to this test";

  state.OnBackendChannelChange(1, 101);
  EXPECT_EQ(1, PlayRequests());
}
