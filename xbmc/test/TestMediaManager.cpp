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

//! The exact call that CDVDFactoryInputStream::CreateInputStream makes.
TEST(TestMediaManager, TranslateDevicePathDoesNotRequireInitialize)
{
  // The value is platform dependent and may legitimately be empty; that the call returns
  // at all is the point.
  EXPECT_NO_FATAL_FAILURE(CServiceBroker::GetMediaManager().TranslateDevicePath(""));
}
