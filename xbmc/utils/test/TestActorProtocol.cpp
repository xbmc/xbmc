/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/ActorProtocol.h"

#include <string>
#include <vector>

#include <gtest/gtest.h>

using namespace Actor;

namespace
{
constexpr int SIGNAL_A = 1;
constexpr int SIGNAL_B = 2;
constexpr int SIGNAL_C = 3;

void SendOut(Protocol& protocol, int signal, const std::string& text)
{
  protocol.SendOutMessage(signal, text.c_str(), text.size() + 1);
}

std::vector<std::pair<int, std::string>> ReceiveAllOut(Protocol& protocol)
{
  std::vector<std::pair<int, std::string>> received;
  Message* msg;
  while (protocol.ReceiveOutMessage(&msg))
  {
    received.emplace_back(msg->signal, reinterpret_cast<const char*>(msg->data));
    msg->Release();
  }
  return received;
}
} // namespace

TEST(TestActorProtocol, PurgeOutRemovesOnlyMatchingSignal)
{
  Protocol protocol("test");
  SendOut(protocol, SIGNAL_A, "a1");
  SendOut(protocol, SIGNAL_B, "b1");
  SendOut(protocol, SIGNAL_A, "a2");
  SendOut(protocol, SIGNAL_B, "b2");

  EXPECT_EQ(2u, protocol.PurgeOut(SIGNAL_A));

  const std::vector<std::pair<int, std::string>> expected{{SIGNAL_B, "b1"}, {SIGNAL_B, "b2"}};
  EXPECT_EQ(expected, ReceiveAllOut(protocol));
  EXPECT_EQ(0u, protocol.PurgeOut(SIGNAL_A));
}

TEST(TestActorProtocol, PurgeOutPassesRemovedMessagesToCallback)
{
  Protocol protocol("test");
  SendOut(protocol, SIGNAL_A, "a1");
  SendOut(protocol, SIGNAL_B, "b1");
  SendOut(protocol, SIGNAL_A, "a longer payload than the internal message buffer");

  std::vector<std::string> purged;
  const auto collect = [&purged](const Message& msg)
  { purged.emplace_back(reinterpret_cast<const char*>(msg.data)); };

  EXPECT_EQ(2u, protocol.PurgeOut(SIGNAL_A, collect));

  const std::vector<std::string> expected{"a1",
                                          "a longer payload than the internal message buffer"};
  EXPECT_EQ(expected, purged);
}

TEST(TestActorProtocol, PurgeOutReturnsMessagesToFreeQueue)
{
  Protocol protocol("test");
  Message* first;
  protocol.SendOutMessage(SIGNAL_A);
  ASSERT_TRUE(protocol.ReceiveOutMessage(&first));
  protocol.SendOutMessage(SIGNAL_A, nullptr, 0, first);
  protocol.SendOutMessage(SIGNAL_B);

  protocol.PurgeOut(SIGNAL_A);
  protocol.SendOutMessage(SIGNAL_C);

  Message* msg;
  ASSERT_TRUE(protocol.ReceiveOutMessage(&msg));
  EXPECT_EQ(SIGNAL_B, msg->signal);
  msg->Release();
  ASSERT_TRUE(protocol.ReceiveOutMessage(&msg));
  EXPECT_EQ(SIGNAL_C, msg->signal);
  EXPECT_EQ(first, msg);
  msg->Release();
}

TEST(TestActorProtocol, PurgeInReturnsMessagesToFreeQueue)
{
  Protocol protocol("test");
  Message* first;
  protocol.SendInMessage(SIGNAL_A);
  ASSERT_TRUE(protocol.ReceiveInMessage(&first));
  protocol.SendInMessage(SIGNAL_A, nullptr, 0, first);
  protocol.SendInMessage(SIGNAL_B);

  protocol.PurgeIn(SIGNAL_A);
  protocol.SendInMessage(SIGNAL_C);

  Message* msg;
  ASSERT_TRUE(protocol.ReceiveInMessage(&msg));
  EXPECT_EQ(SIGNAL_B, msg->signal);
  msg->Release();
  ASSERT_TRUE(protocol.ReceiveInMessage(&msg));
  EXPECT_EQ(SIGNAL_C, msg->signal);
  EXPECT_EQ(first, msg);
  msg->Release();
  EXPECT_FALSE(protocol.ReceiveInMessage(&msg));
}
