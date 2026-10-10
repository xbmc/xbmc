/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "filesystem/SourcesDirectory.h"
#include "media/MediaSection.h"

#include <gtest/gtest.h>

using KODI::MEDIA::MEDIA_SECTIONS;
using KODI::MEDIA::MediaSection;
using XFILE::CSourcesDirectory;

TEST(TestSourcesDirectory, EverySectionReadsBackFromItsPath)
{
  for (const MediaSection section : MEDIA_SECTIONS)
    EXPECT_EQ(CSourcesDirectory::SectionOf(CSourcesDirectory::PathOf(section)), section);
}

TEST(TestSourcesDirectory, ASectionsPathNamesIt)
{
  EXPECT_EQ(CSourcesDirectory::PathOf(MediaSection::VIDEO), "sources://video/");
  EXPECT_EQ(CSourcesDirectory::PathOf(MediaSection::PROGRAMS), "sources://programs/");
}

TEST(TestSourcesDirectory, AnyOtherPathIsNoSection)
{
  EXPECT_FALSE(CSourcesDirectory::SectionOf("sources://"));
  EXPECT_FALSE(CSourcesDirectory::SectionOf("sources://add/"));
  EXPECT_FALSE(CSourcesDirectory::SectionOf("videodb://movies/"));
  EXPECT_FALSE(CSourcesDirectory::SectionOf("special://video/"));
}
