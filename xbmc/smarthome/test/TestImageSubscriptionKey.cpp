/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "smarthome/ImageSubscriptionKey.h"

#include <map>

#include <gtest/gtest.h>

using namespace KODI::SMART_HOME;

TEST(TestImageSubscriptionKey, EmptyTransportUsesCompressed)
{
  const ImageSubscriptionKey subscription = MakeImageSubscriptionKey("/camera/image", "");

  EXPECT_EQ(subscription.first, "/camera/image");
  EXPECT_EQ(subscription.second, "compressed");
}

TEST(TestImageSubscriptionKey, TransportIsPassedThrough)
{
  const ImageSubscriptionKey subscription = MakeImageSubscriptionKey("/camera/image", "raw");

  EXPECT_EQ(subscription.first, "/camera/image");
  EXPECT_EQ(subscription.second, "raw");
}

TEST(TestImageSubscriptionKey, TopicAndTransportAreIndependentIdentityComponents)
{
  std::map<ImageSubscriptionKey, unsigned int> subscriptions;
  ++subscriptions[MakeImageSubscriptionKey("/camera/image", "compressed")];
  ++subscriptions[MakeImageSubscriptionKey("/camera/image", "raw")];
  ++subscriptions[MakeImageSubscriptionKey("/camera/image", "compressed")];

  ASSERT_EQ(subscriptions.size(), 2);
  EXPECT_EQ(subscriptions.at({"/camera/image", "compressed"}), 2);
  EXPECT_EQ(subscriptions.at({"/camera/image", "raw"}), 1);
}
