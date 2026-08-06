/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ServiceBroker.h"
#include "storage/MediaManager.h"

#include <gtest/gtest.h>

TEST(TestMediaManager, IsAvailableInTheTestEnvironment)
{
  EXPECT_NO_FATAL_FAILURE(CServiceBroker::GetMediaManager());
}

//! The call CDVDFactoryInputStream::CreateInputStream makes.
TEST(TestMediaManager, TranslateDevicePathDoesNotRequireInitialize)
{
  EXPECT_NO_FATAL_FAILURE(CServiceBroker::GetMediaManager().TranslateDevicePath(""));
}
