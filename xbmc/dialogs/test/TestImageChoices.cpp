/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "dialogs/ImageChoices.h"

#include <gtest/gtest.h>

using namespace KODI;

TEST(TestImageChoices, RemoteEntryCarriesItsIndex)
{
  EXPECT_EQ(ART::CHOICE::RemoteOf(12), "thumb://Remote12");
  EXPECT_EQ(ART::CHOICE::RemoteIndexOf(ART::CHOICE::RemoteOf(0)), 0u);
  EXPECT_EQ(ART::CHOICE::RemoteIndexOf(ART::CHOICE::RemoteOf(12)), 12u);
}

TEST(TestImageChoices, OnlyRemoteEntriesHaveAnIndex)
{
  EXPECT_FALSE(ART::CHOICE::RemoteIndexOf(ART::CHOICE::CURRENT));
  EXPECT_FALSE(ART::CHOICE::RemoteIndexOf(ART::CHOICE::REMOTE));
  EXPECT_FALSE(ART::CHOICE::RemoteIndexOf("thumb://Remote1x"));
  EXPECT_FALSE(ART::CHOICE::RemoteIndexOf("special://home/thumb.jpg"));
}
