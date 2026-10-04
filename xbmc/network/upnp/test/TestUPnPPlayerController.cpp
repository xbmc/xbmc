/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "network/upnp/UPnPPlayerController.h"

#include <chrono>

#include <gtest/gtest.h>

using namespace UPNP;
using namespace std::chrono_literals;

namespace
{
// No media controller is given, so each test delivers a reply by calling the action's handler
// directly, as Platinum does with the userdata.
class TestUPnPPlayerController : public ::testing::Test
{
protected:
  TestUPnPPlayerController() : m_device(new PLT_DeviceData()), m_controller(nullptr, m_device) {}

  PLT_DeviceDataReference m_device;
  CUPnPPlayerController m_controller;
};
} // unnamed namespace

TEST_F(TestUPnPPlayerController, AnActionIsReadableAfterItIsRetired)
{
  auto* action = m_controller.BeginAction();
  ASSERT_NE(nullptr, action);

  action->OnPlayResult(NPT_SUCCESS, m_device, action);
  m_controller.EndAction(*action);

  // Checked on the count, because reading a freed action would not fail.
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

  EXPECT_TRUE(first->Event().Wait(0ms));
  EXPECT_FALSE(second->Event().Wait(0ms));
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

  // Only the action begun last is still held; the others were freed on reply.
  EXPECT_EQ(1U, m_controller.HeldActionCount());
}
