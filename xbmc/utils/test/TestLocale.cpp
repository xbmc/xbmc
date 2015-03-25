/*
 *      Copyright (C) 2015 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <gtest/gtest.h>

#include "utils/Locale.h"
#include "utils/StringUtils.h"

static const std::string TerritorySeparator = "_";
static const std::string CodesetSeparator = ".";
static const std::string ModifierSeparator = "@";

static const std::string LanguageCodeEnglish = "en";
static const std::string TerritoryCodeBritain = "GB";
static const std::string CodesetUtf8 = "UTF-8";
static const std::string ModifierLatin = "latin";

TEST(TestLocale, DefaultLocale)
{
  CLocale locale;
  ASSERT_FALSE(locale.IsValid());
  ASSERT_STREQ("", locale.GetLanguageCode().c_str());
  ASSERT_STREQ("", locale.GetTerritoryCode().c_str());
  ASSERT_STREQ("", locale.GetCodeset().c_str());
  ASSERT_STREQ("", locale.GetModifier().c_str());
  ASSERT_STREQ("", locale.ToString().c_str());
}

TEST(TestLocale, LanguageLocale)
{
  CLocale locale(LanguageCodeEnglish);
  ASSERT_TRUE(locale.IsValid());
  ASSERT_STREQ(LanguageCodeEnglish.c_str(), locale.GetLanguageCode().c_str());
  ASSERT_STREQ("", locale.GetTerritoryCode().c_str());
  ASSERT_STREQ("", locale.GetCodeset().c_str());
  ASSERT_STREQ("", locale.GetModifier().c_str());
  ASSERT_STREQ(LanguageCodeEnglish.c_str(), locale.ToString().c_str());
  ASSERT_STREQ(LanguageCodeEnglish.c_str(), locale.ToStringLC().c_str());
  ASSERT_STREQ(LanguageCodeEnglish.c_str(), locale.ToShortString().c_str());
  ASSERT_STREQ(LanguageCodeEnglish.c_str(), locale.ToShortStringLC().c_str());
}

TEST(TestLocale, LanguageTerritoryLocale)
{
  const std::string strLocale = LanguageCodeEnglish + TerritorySeparator + TerritoryCodeBritain;
  std::string strLocaleLC = strLocale;
  StringUtils::ToLower(strLocaleLC);

  CLocale locale(LanguageCodeEnglish, TerritoryCodeBritain);
  ASSERT_TRUE(locale.IsValid());
  ASSERT_STREQ(LanguageCodeEnglish.c_str(), locale.GetLanguageCode().c_str());
  ASSERT_STREQ(TerritoryCodeBritain.c_str(), locale.GetTerritoryCode().c_str());
  ASSERT_STREQ("", locale.GetCodeset().c_str());
  ASSERT_STREQ("", locale.GetModifier().c_str());
  ASSERT_STREQ(strLocale.c_str(), locale.ToString().c_str());
  ASSERT_STREQ(strLocaleLC.c_str(), locale.ToStringLC().c_str());
  ASSERT_STREQ(strLocale.c_str(), locale.ToShortString().c_str());
  ASSERT_STREQ(strLocaleLC.c_str(), locale.ToShortStringLC().c_str());
}

TEST(TestLocale, LanguageCodesetLocale)
{
  const std::string strLocale = LanguageCodeEnglish + CodesetSeparator + CodesetUtf8;
  std::string strLocaleLC = strLocale;
  StringUtils::ToLower(strLocaleLC);

  CLocale locale(LanguageCodeEnglish, "", CodesetUtf8);
  ASSERT_TRUE(locale.IsValid());
  ASSERT_STREQ(LanguageCodeEnglish.c_str(), locale.GetLanguageCode().c_str());
  ASSERT_STREQ("", locale.GetTerritoryCode().c_str());
  ASSERT_STREQ(CodesetUtf8.c_str(), locale.GetCodeset().c_str());
  ASSERT_STREQ("", locale.GetModifier().c_str());
  ASSERT_STREQ(strLocale.c_str(), locale.ToString().c_str());
  ASSERT_STREQ(strLocaleLC.c_str(), locale.ToStringLC().c_str());
  ASSERT_STREQ(LanguageCodeEnglish.c_str(), locale.ToShortString().c_str());
  ASSERT_STREQ(LanguageCodeEnglish.c_str(), locale.ToShortStringLC().c_str());
}

TEST(TestLocale, LanguageModifierLocale)
{
  const std::string strLocale = LanguageCodeEnglish + ModifierSeparator + ModifierLatin;
  std::string strLocaleLC = strLocale;
  StringUtils::ToLower(strLocaleLC);

  CLocale locale(LanguageCodeEnglish, "", "", ModifierLatin);
  ASSERT_TRUE(locale.IsValid());
  ASSERT_STREQ(LanguageCodeEnglish.c_str(), locale.GetLanguageCode().c_str());
  ASSERT_STREQ("", locale.GetTerritoryCode().c_str());
  ASSERT_STREQ("", locale.GetCodeset().c_str());
  ASSERT_STREQ(ModifierLatin.c_str(), locale.GetModifier().c_str());
  ASSERT_STREQ(strLocale.c_str(), locale.ToString().c_str());
  ASSERT_STREQ(strLocaleLC.c_str(), locale.ToStringLC().c_str());
  ASSERT_STREQ(LanguageCodeEnglish.c_str(), locale.ToShortString().c_str());
  ASSERT_STREQ(LanguageCodeEnglish.c_str(), locale.ToShortStringLC().c_str());
}

TEST(TestLocale, LanguageTerritoryCodesetLocale)
{
  const std::string strLocaleShort = LanguageCodeEnglish + TerritorySeparator + TerritoryCodeBritain;
  std::string strLocaleShortLC = strLocaleShort;
  StringUtils::ToLower(strLocaleShortLC);
  const std::string strLocale = strLocaleShort + CodesetSeparator + CodesetUtf8;
  std::string strLocaleLC = strLocale;
  StringUtils::ToLower(strLocaleLC);

  CLocale locale(LanguageCodeEnglish, TerritoryCodeBritain, CodesetUtf8);
  ASSERT_TRUE(locale.IsValid());
  ASSERT_STREQ(LanguageCodeEnglish.c_str(), locale.GetLanguageCode().c_str());
  ASSERT_STREQ(TerritoryCodeBritain.c_str(), locale.GetTerritoryCode().c_str());
  ASSERT_STREQ(CodesetUtf8.c_str(), locale.GetCodeset().c_str());
  ASSERT_STREQ("", locale.GetModifier().c_str());
  ASSERT_STREQ(strLocale.c_str(), locale.ToString().c_str());
  ASSERT_STREQ(strLocaleLC.c_str(), locale.ToStringLC().c_str());
  ASSERT_STREQ(strLocaleShort.c_str(), locale.ToShortString().c_str());
  ASSERT_STREQ(strLocaleShortLC.c_str(), locale.ToShortStringLC().c_str());
}

TEST(TestLocale, LanguageTerritoryModifierLocale)
{
  const std::string strLocaleShort = LanguageCodeEnglish + TerritorySeparator + TerritoryCodeBritain;
  std::string strLocaleShortLC = strLocaleShort;
  StringUtils::ToLower(strLocaleShortLC);
  const std::string strLocale = strLocaleShort + ModifierSeparator + ModifierLatin;
  std::string strLocaleLC = strLocale;
  StringUtils::ToLower(strLocaleLC);

  CLocale locale(LanguageCodeEnglish, TerritoryCodeBritain, "", ModifierLatin);
  ASSERT_TRUE(locale.IsValid());
  ASSERT_STREQ(LanguageCodeEnglish.c_str(), locale.GetLanguageCode().c_str());
  ASSERT_STREQ(TerritoryCodeBritain.c_str(), locale.GetTerritoryCode().c_str());
  ASSERT_STREQ("", locale.GetCodeset().c_str());
  ASSERT_STREQ(ModifierLatin.c_str(), locale.GetModifier().c_str());
  ASSERT_STREQ(strLocale.c_str(), locale.ToString().c_str());
  ASSERT_STREQ(strLocaleLC.c_str(), locale.ToStringLC().c_str());
  ASSERT_STREQ(strLocaleShort.c_str(), locale.ToShortString().c_str());
  ASSERT_STREQ(strLocaleShortLC.c_str(), locale.ToShortStringLC().c_str());
}

TEST(TestLocale, LanguageTerritoryCodesetModifierLocale)
{
  const std::string strLocaleShort = LanguageCodeEnglish + TerritorySeparator + TerritoryCodeBritain;
  std::string strLocaleShortLC = strLocaleShort;
  StringUtils::ToLower(strLocaleShortLC);
  const std::string strLocale = strLocaleShort + CodesetSeparator + CodesetUtf8 + ModifierSeparator + ModifierLatin;
  std::string strLocaleLC = strLocale;
  StringUtils::ToLower(strLocaleLC);

  CLocale locale(LanguageCodeEnglish, TerritoryCodeBritain, CodesetUtf8, ModifierLatin);
  ASSERT_TRUE(locale.IsValid());
  ASSERT_STREQ(LanguageCodeEnglish.c_str(), locale.GetLanguageCode().c_str());
  ASSERT_STREQ(TerritoryCodeBritain.c_str(), locale.GetTerritoryCode().c_str());
  ASSERT_STREQ(CodesetUtf8.c_str(), locale.GetCodeset().c_str());
  ASSERT_STREQ(ModifierLatin.c_str(), locale.GetModifier().c_str());
  ASSERT_STREQ(strLocale.c_str(), locale.ToString().c_str());
  ASSERT_STREQ(strLocaleLC.c_str(), locale.ToStringLC().c_str());
  ASSERT_STREQ(strLocaleShort.c_str(), locale.ToShortString().c_str());
  ASSERT_STREQ(strLocaleShortLC.c_str(), locale.ToShortStringLC().c_str());
}

TEST(TestLocale, FullStringLocale)
{
  const std::string strLocaleShort = LanguageCodeEnglish + TerritorySeparator + TerritoryCodeBritain;
  std::string strLocaleShortLC = strLocaleShort;
  StringUtils::ToLower(strLocaleShortLC);
  const std::string strLocale = strLocaleShort + CodesetSeparator + CodesetUtf8 + ModifierSeparator + ModifierLatin;
  std::string strLocaleLC = strLocale;
  StringUtils::ToLower(strLocaleLC);

  CLocale locale(strLocale);
  ASSERT_TRUE(locale.IsValid());
  ASSERT_STREQ(LanguageCodeEnglish.c_str(), locale.GetLanguageCode().c_str());
  ASSERT_STREQ(TerritoryCodeBritain.c_str(), locale.GetTerritoryCode().c_str());
  ASSERT_STREQ(CodesetUtf8.c_str(), locale.GetCodeset().c_str());
  ASSERT_STREQ(ModifierLatin.c_str(), locale.GetModifier().c_str());
  ASSERT_STREQ(strLocale.c_str(), locale.ToString().c_str());
  ASSERT_STREQ(strLocaleLC.c_str(), locale.ToStringLC().c_str());
  ASSERT_STREQ(strLocaleShort.c_str(), locale.ToShortString().c_str());
  ASSERT_STREQ(strLocaleShortLC.c_str(), locale.ToShortStringLC().c_str());
}

TEST(TestLocale, FromString)
{
  std::string strLocale = "";
  CLocale locale = CLocale::FromString(strLocale);
  ASSERT_FALSE(locale.IsValid());
  ASSERT_STREQ(strLocale.c_str(), locale.ToString().c_str());

  strLocale = LanguageCodeEnglish;
  locale = CLocale::FromString(strLocale);
  ASSERT_TRUE(locale.IsValid());
  ASSERT_STREQ(strLocale.c_str(), locale.ToString().c_str());

  strLocale = LanguageCodeEnglish + TerritorySeparator + TerritoryCodeBritain;
  locale = CLocale::FromString(strLocale);
  ASSERT_TRUE(locale.IsValid());
  ASSERT_STREQ(strLocale.c_str(), locale.ToString().c_str());

  strLocale = LanguageCodeEnglish + CodesetSeparator + CodesetUtf8;
  locale = CLocale::FromString(strLocale);
  ASSERT_TRUE(locale.IsValid());
  ASSERT_STREQ(strLocale.c_str(), locale.ToString().c_str());

  strLocale = LanguageCodeEnglish + ModifierSeparator + ModifierLatin;
  locale = CLocale::FromString(strLocale);
  ASSERT_TRUE(locale.IsValid());
  ASSERT_STREQ(strLocale.c_str(), locale.ToString().c_str());

  strLocale = LanguageCodeEnglish + TerritorySeparator + TerritoryCodeBritain + CodesetSeparator + CodesetUtf8;
  locale = CLocale::FromString(strLocale);
  ASSERT_TRUE(locale.IsValid());
  ASSERT_STREQ(strLocale.c_str(), locale.ToString().c_str());

  strLocale = LanguageCodeEnglish + TerritorySeparator + TerritoryCodeBritain + ModifierSeparator + ModifierLatin;
  locale = CLocale::FromString(strLocale);
  ASSERT_TRUE(locale.IsValid());
  ASSERT_STREQ(strLocale.c_str(), locale.ToString().c_str());

  strLocale = LanguageCodeEnglish + TerritorySeparator + TerritoryCodeBritain + CodesetSeparator + CodesetUtf8 + ModifierSeparator + ModifierLatin;
  locale = CLocale::FromString(strLocale);
  ASSERT_TRUE(locale.IsValid());
  ASSERT_STREQ(strLocale.c_str(), locale.ToString().c_str());
}

TEST(TestLocale, EmptyLocale)
{
  ASSERT_FALSE(CLocale::Empty.IsValid());
  ASSERT_STREQ("", CLocale::Empty.GetLanguageCode().c_str());
  ASSERT_STREQ("", CLocale::Empty.GetTerritoryCode().c_str());
  ASSERT_STREQ("", CLocale::Empty.GetCodeset().c_str());
  ASSERT_STREQ("", CLocale::Empty.GetModifier().c_str());
  ASSERT_STREQ("", CLocale::Empty.ToString().c_str());
}

TEST(TestLocale, Equals)
{
  std::string strLocale = "";
  CLocale locale;
  ASSERT_TRUE(locale.Equals(strLocale));

  locale = CLocale(LanguageCodeEnglish);
  strLocale = LanguageCodeEnglish;
  ASSERT_TRUE(locale.Equals(strLocale));

  locale = CLocale(LanguageCodeEnglish, TerritoryCodeBritain);
  strLocale = LanguageCodeEnglish + TerritorySeparator + TerritoryCodeBritain;
  ASSERT_TRUE(locale.Equals(strLocale));

  locale = CLocale(LanguageCodeEnglish, "", CodesetUtf8);
  strLocale = LanguageCodeEnglish + CodesetSeparator + CodesetUtf8;
  ASSERT_TRUE(locale.Equals(strLocale));

  locale = CLocale(LanguageCodeEnglish, "", "", ModifierLatin);
  strLocale = LanguageCodeEnglish + ModifierSeparator + ModifierLatin;
  ASSERT_TRUE(locale.Equals(strLocale));

  locale = CLocale(LanguageCodeEnglish, TerritoryCodeBritain, CodesetUtf8);
  strLocale = LanguageCodeEnglish + TerritorySeparator + TerritoryCodeBritain + CodesetSeparator + CodesetUtf8;
  ASSERT_TRUE(locale.Equals(strLocale));

  locale = CLocale(LanguageCodeEnglish, TerritoryCodeBritain, "", ModifierLatin);
  strLocale = LanguageCodeEnglish + TerritorySeparator + TerritoryCodeBritain + ModifierSeparator + ModifierLatin;
  ASSERT_TRUE(locale.Equals(strLocale));

  locale = CLocale(LanguageCodeEnglish, TerritoryCodeBritain, CodesetUtf8, ModifierLatin);
  strLocale = LanguageCodeEnglish + TerritorySeparator + TerritoryCodeBritain + CodesetSeparator + CodesetUtf8 + ModifierSeparator + ModifierLatin;
  ASSERT_TRUE(locale.Equals(strLocale));
}
