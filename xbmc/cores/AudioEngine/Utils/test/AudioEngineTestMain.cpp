/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "application/AppEnvironment.h"
#include "application/AppParams.h"

#include <memory>

#include <gtest/gtest.h>

int main(int argc, char** argv)
{
  const auto appParams = std::make_shared<CAppParams>();
  appParams->SetUserDirectoriesLocation(UserDirectoriesLocation::TEST);
  CAppEnvironment::SetUp(appParams);

  ::testing::InitGoogleTest(&argc, argv);
  const int result = RUN_ALL_TESTS();

  CAppEnvironment::TearDown();
  return result;
}
