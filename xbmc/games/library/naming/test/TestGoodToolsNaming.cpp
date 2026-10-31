/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "games/library/naming/GoodToolsNaming.h"

#include <gtest/gtest.h>

using namespace KODI;
using namespace GAME;
using namespace GAME::GOODTOOLS;

TEST(TestGoodToolsNaming, ReadsACountry)
{
  EXPECT_EQ(CGoodToolsNaming::Region("U")->ToString(), "US");
  EXPECT_EQ(CGoodToolsNaming::Region("UK")->ToString(), "GB");
  EXPECT_EQ(CGoodToolsNaming::Region("As")->ToString(), "142");
  EXPECT_EQ(CGoodToolsNaming::Region("D")->ToString(), "NL");
  EXPECT_EQ(CGoodToolsNaming::Region("W")->market, "World");
  EXPECT_FALSE(CGoodToolsNaming::Region("u").has_value());
}

TEST(TestGoodToolsNaming, ReadsAFlag)
{
  EXPECT_EQ(CGoodToolsNaming::ParseFlag("!")->type, FlagType::VERIFIED);
  EXPECT_EQ(CGoodToolsNaming::ParseFlag("!p")->type, FlagType::PENDING);
  EXPECT_EQ(CGoodToolsNaming::ParseFlag("a")->type, FlagType::ALTERNATE_DUMP);
  EXPECT_EQ(CGoodToolsNaming::ParseFlag("h2")->number, 2);
  EXPECT_EQ(CGoodToolsNaming::ParseFlag("t1")->type, FlagType::TRAINED);
  EXPECT_FALSE(CGoodToolsNaming::ParseFlag("FD").has_value());
  EXPECT_FALSE(CGoodToolsNaming::ParseFlag("bad sectors").has_value());
}

TEST(TestGoodToolsNaming, ReadsATranslationsLanguage)
{
  const std::optional<Flag> flag = CGoodToolsNaming::ParseFlag("T+Ger1.00_Translator");
  ASSERT_TRUE(flag.has_value());
  EXPECT_EQ(flag->type, FlagType::TRANSLATION);
  EXPECT_EQ(flag->language, "de");
  EXPECT_EQ(CGoodToolsNaming::ParseFlag("T-Spa")->type, FlagType::OLD_TRANSLATION);
}

TEST(TestGoodToolsNaming, ReadsTheLanguageCodesIsoDoesNotGive)
{
  EXPECT_EQ(CGoodToolsNaming::TranslationLanguage("Fre"), "fr");
  EXPECT_EQ(CGoodToolsNaming::TranslationLanguage("Gre"), "el");
  EXPECT_EQ(CGoodToolsNaming::TranslationLanguage("Jap"), "ja");
  EXPECT_EQ(CGoodToolsNaming::TranslationLanguage("Bra"), "pt");
  EXPECT_EQ(CGoodToolsNaming::TranslationLanguage("Rom"), "ro");
  EXPECT_TRUE(CGoodToolsNaming::TranslationLanguage("Xyz").empty());
}

TEST(TestGoodToolsNaming, ReadsALanguageCount)
{
  EXPECT_TRUE(CGoodToolsNaming::IsLanguageCount("M5"));
  EXPECT_FALSE(CGoodToolsNaming::IsLanguageCount("MB"));
  EXPECT_FALSE(CGoodToolsNaming::IsLanguageCount("M"));
}
