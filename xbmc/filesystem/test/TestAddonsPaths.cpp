/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "URL.h"
#include "filesystem/AddonsPaths.h"

#include <gtest/gtest.h>

using namespace KODI;

TEST(TestAddonsPaths, EndpointIsTheHostName)
{
  for (const char* path :
       {ADDONS::ALL, ADDONS::USER, ADDONS::DEPENDENCIES, ADDONS::DISABLED, ADDONS::OUTDATED,
        ADDONS::RUNNING, ADDONS::REPOS, ADDONS::SOURCES, ADDONS::SEARCH, ADDONS::RECENTLY_UPDATED,
        ADDONS::DOWNLOADING, ADDONS::MORE, ADDONS::DEFAULT_BINARY_ADDONS_SOURCE})
    EXPECT_EQ(ADDONS::EndpointOf(path), CURL(path).GetHostName()) << path;
}
