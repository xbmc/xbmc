/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ServiceBroker.h"

#include <gtest/gtest.h>

int main(int argc, char** argv)
{
  CServiceBroker::CreateLogging();

  ::testing::InitGoogleTest(&argc, argv);
  const int result = RUN_ALL_TESTS();

  CServiceBroker::DestroyLogging();
  return result;
}
