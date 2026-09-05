/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "cores/IPlayerCallback.h"
#include "network/upnp/UPnPPlayerController.h"

#include <chrono>

#include <gtest/gtest.h>

using namespace UPNP;
using namespace std::chrono_literals;

namespace
{
class CNullPlayerCallback : public IPlayerCallback
{
public:
  void OnPlayBackEnded() override {}
  void OnPlayBackStarted(const CFileItem& file) override {}
  void OnPlayBackStopped() override {}
  void OnPlayBackError() override {}
  void OnQueueNextItem() override {}
};

// The controller reaches the network only through the media controller, which none of these tests
// give it: an action is driven by calling the reply back on it directly, which is all Platinum
// does with the userdata it was handed.
class TestUPnPPlayerController : public ::testing::Test
{
protected:
  TestUPnPPlayerController()
    : m_device(new PLT_DeviceData()),
      m_controller(nullptr, m_device, m_callback)
  {
  }

  PLT_DeviceDataReference m_device;
  CNullPlayerCallback m_callback;
  CUPnPPlayerController m_controller;
};
} // unnamed namespace

TEST_F(TestUPnPPlayerController, AnActionIsReadableAfterItIsRetired)
{
  auto* action = m_controller.BeginAction();
  ASSERT_NE(nullptr, action);

  action->OnPlayResult(NPT_SUCCESS, m_device, action);
  m_controller.EndAction(*action);

  // The caller reads the reply off the action after the wait, so retiring must not free it.
  // Asserted on the count rather than on the read, which would sail past a freed action.
  EXPECT_EQ(1U, m_controller.HeldActionCount());
  EXPECT_EQ(NPT_SUCCESS, action->GetStatus());
}

TEST_F(TestUPnPPlayerController, AReplyDoesNotReachAnotherAction)
{
  auto* first = m_controller.BeginAction();
  m_controller.EndAction(*first);
  auto* second = m_controller.BeginAction();
  ASSERT_NE(first, second);

  first->OnStopResult(NPT_FAILURE, m_device, first);

  // the reply belongs to the retired action, and must not release the wait on the current one
  EXPECT_TRUE(first->Event().Wait(std::chrono::milliseconds(0)));
  EXPECT_FALSE(second->Event().Wait(std::chrono::milliseconds(0)));
}

TEST_F(TestUPnPPlayerController, AnActionThatNeverRepliedIsNotASuccess)
{
  auto* action = m_controller.BeginAction();

  EXPECT_NE(NPT_SUCCESS, action->GetStatus());
}

TEST_F(TestUPnPPlayerController, TransportInfoBelongsToTheActionThatAskedForIt)
{
  auto* playing = m_controller.BeginAction();
  m_controller.EndAction(*playing);
  auto* stopped = m_controller.BeginAction();

  PLT_TransportInfo playingInfo;
  playingInfo.cur_transport_state = "PLAYING";
  playing->OnGetTransportInfoResult(NPT_SUCCESS, m_device, &playingInfo, playing);

  PLT_TransportInfo stoppedInfo;
  stoppedInfo.cur_transport_state = "STOPPED";
  stopped->OnGetTransportInfoResult(NPT_SUCCESS, m_device, &stoppedInfo, stopped);

  EXPECT_STREQ("PLAYING", playing->GetTransportState().GetChars());
  EXPECT_STREQ("STOPPED", stopped->GetTransportState().GetChars());
}

TEST_F(TestUPnPPlayerController, ASetNextAVTransportURIReplyCompletesItsAction)
{
  auto* action = m_controller.BeginAction();

  action->OnSetNextAVTransportURIResult(NPT_SUCCESS, m_device, action);

  EXPECT_TRUE(action->Event().Wait(std::chrono::milliseconds(0)));
  EXPECT_EQ(NPT_SUCCESS, action->GetStatus());
}

TEST_F(TestUPnPPlayerController, AFailedSendLeavesNoAction)
{
  CUPnPPlayerController::CAction* action = nullptr;
  const NPT_Result result =
      m_controller.Send(action, [](void* userdata) { return NPT_ERROR_INVALID_STATE; });

  EXPECT_TRUE(NPT_FAILED(result));
  EXPECT_EQ(nullptr, action);
}

TEST_F(TestUPnPPlayerController, RepliedActionsDoNotAccumulate)
{
  for (int i = 0; i < 20; ++i)
  {
    auto* action = m_controller.BeginAction();
    action->OnPlayResult(NPT_SUCCESS, m_device, action);
    m_controller.EndAction(*action);
  }

  // every action but the one begun last has replied and been retired, so all of them are gone
  EXPECT_EQ(1U, m_controller.HeldActionCount());
}
