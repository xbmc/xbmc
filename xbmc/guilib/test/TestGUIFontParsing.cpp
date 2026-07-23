/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "guilib/GUIFontParsing.h"

#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace
{
// Stands in for FontEntry. The real one holds a CGUIFont, which needs a GL
// context, so the erase logic is tested through the same template on a stub.
struct StubEntry
{
  std::string name;
  int info;
};

std::string StubName(const StubEntry& e)
{
  return e.name;
}
} // namespace

TEST(TestGUIFontParsing, EraseFirstByNameRemovesMatch)
{
  std::vector<StubEntry> entries{{"font12", 12}, {"font13", 13}, {"font14", 14}};

  EXPECT_TRUE(EraseFirstByName(entries, "font13", StubName));

  ASSERT_EQ(entries.size(), 2U);
  EXPECT_EQ(entries[0].name, "font12");
  EXPECT_EQ(entries[1].name, "font14");
}

TEST(TestGUIFontParsing, EraseFirstByNameKeepsNameAndInfoPaired)
{
  // Regression: the old code erased from m_vecFonts but not the index-aligned
  // m_vecFontInfo, so every later entry's info belonged to a different font.
  std::vector<StubEntry> entries{{"font12", 12}, {"font13", 13}, {"font14", 14}};

  EraseFirstByName(entries, "font12", StubName);

  for (const auto& e : entries)
    EXPECT_EQ(e.info, std::stoi(e.name.substr(4)));
}

TEST(TestGUIFontParsing, EraseFirstByNameIsCaseInsensitive)
{
  std::vector<StubEntry> entries{{"Font13", 13}};

  EXPECT_TRUE(EraseFirstByName(entries, "font13", StubName));
  EXPECT_TRUE(entries.empty());
}

TEST(TestGUIFontParsing, EraseFirstByNameReturnsFalseWhenAbsent)
{
  std::vector<StubEntry> entries{{"font12", 12}};

  EXPECT_FALSE(EraseFirstByName(entries, "font99", StubName));
  EXPECT_EQ(entries.size(), 1U);
}
