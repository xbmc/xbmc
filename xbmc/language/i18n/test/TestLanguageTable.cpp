/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "language/i18n/LanguageTable.h"

#include <gtest/gtest.h>

using namespace KODI::LANGUAGE::I18N;

namespace
{

//! The table is process-wide, so a test that declares languages has to put it back afterwards
class LanguageTableTest : public testing::Test
{
protected:
  void TearDown() override { CLanguageTable::GetInstance().Reset(); }

  CLanguageTable& Table() const { return CLanguageTable::GetInstance(); }
};

} // namespace

TEST_F(LanguageTableTest, NamesIso6391Codes)
{
  EXPECT_EQ(Table().NameOf("en"), "English");
  EXPECT_EQ(Table().NameOf("fr"), "French");
}

TEST_F(LanguageTableTest, NamesIso6392CodesInBothForms)
{
  EXPECT_EQ(Table().NameOf("eng"), "English");
  EXPECT_EQ(Table().NameOf("fre"), "French");
  EXPECT_EQ(Table().NameOf("fra"), "French");
}

TEST_F(LanguageTableTest, NamesDeprecatedCodes)
{
  // Media tagged with the withdrawn spelling still has to be understood
  EXPECT_EQ(Table().NameOf("iw"), "Hebrew");
}

TEST_F(LanguageTableTest, IgnoresCaseAndSurroundingSpace)
{
  EXPECT_EQ(Table().NameOf(" EN "), "English");
  EXPECT_EQ(Table().CodeOf(" ENGLISH "), "en");
}

TEST_F(LanguageTableTest, AnswersUnknownCodesWithNothing)
{
  EXPECT_FALSE(Table().NameOf("").has_value());
  EXPECT_FALSE(Table().NameOf("zzz").has_value());
  EXPECT_FALSE(Table().CodeOf("").has_value());
  EXPECT_FALSE(Table().CodeOf("Not A Language").has_value());

  // qaa to qtz is reserved for local use, so no language is named by any of them
  EXPECT_FALSE(Table().NameOf("qaa").has_value());
}

TEST_F(LanguageTableTest, PrefersTheAlpha2CodeOfANamedLanguage)
{
  EXPECT_EQ(Table().CodeOf("English"), "en");

  // Only a language without an ISO 639-1 code is answered by its alpha-3 one
  EXPECT_EQ(Table().CodeOf("Adygei"), "ady");
}

TEST_F(LanguageTableTest, KnowsTheAlternativeNamesIso6392Records)
{
  // Valencian is an additional name of cat, and only ISO 639-2 records it, so the alpha-3 is the
  // code it answers with even though the language also has the alpha-2 ca
  EXPECT_EQ(Table().CodeOf("Valencian"), "cat");
}

TEST_F(LanguageTableTest, DeclaredLanguagesAreFoundLikeAnyOther)
{
  // A regional variant the ISO 639 tables collapse into one language is the reason the feature
  // exists, so it is the case worth pinning
  Table().Declare({{"es-419", "Spanish - Latin America"}});

  EXPECT_EQ(Table().NameOf("es-419"), "Spanish - Latin America");
  EXPECT_EQ(Table().CodeOf("Spanish - Latin America"), "es-419");
}

TEST_F(LanguageTableTest, ADeclarationReplacesWhatACodeNamed)
{
  Table().Declare({{"fr", "My French"}});

  EXPECT_EQ(Table().NameOf("fr"), "My French");
  EXPECT_EQ(Table().CodeOf("My French"), "fr");

  // Naming a code differently does not withdraw the name the standard gave it
  EXPECT_EQ(Table().CodeOf("French"), "fr");
}

TEST_F(LanguageTableTest, ADeclarationMayRenameALanguageOntoAnother)
{
  // The wiki documents this, warning that add-ons may be confused by it. It is the user's call.
  Table().Declare({{"en-GB", "French"}});

  EXPECT_EQ(Table().CodeOf("French"), "en-gb");
}

TEST_F(LanguageTableTest, ResetRestoresTheStandardLanguages)
{
  Table().Declare({{"fr", "My French"}});
  Table().Reset();

  EXPECT_EQ(Table().NameOf("fr"), "French");
  EXPECT_EQ(Table().CodeOf("French"), "fr");
  EXPECT_FALSE(Table().CodeOf("My French").has_value());
}

TEST_F(LanguageTableTest, ListsTheLanguagesByTheirAlpha2Code)
{
  std::map<std::string, std::string> languages;
  Table().List(languages);

  EXPECT_EQ(languages["en"], "English");
  EXPECT_FALSE(languages.contains("eng"));
}

TEST_F(LanguageTableTest, ListsDeclaredLanguagesWhateverNotationTheyUse)
{
  Table().Declare({{"es-419", "Spanish - Latin America"}, {"en", "My English"}});

  std::map<std::string, std::string> languages;
  Table().List(languages);

  EXPECT_EQ(languages["es-419"], "Spanish - Latin America");
  EXPECT_EQ(languages["en"], "My English");
}
