/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "cores/VideoPlayer/RadioRDSLanguages.h"
#include "language/LanguageTag.h"

#include <string>

#include <gtest/gtest.h>

using KODI::LANGUAGE::CLanguageTag;

namespace
{

// A broadcaster names a language by index, so a cell that names nothing readable takes a
// language off the air entirely, with nothing to log and nobody to ask
TEST(TestRadioRDSLanguages, EveryAssignedIndexNamesALanguage)
{
  for (unsigned int index = 0; index < KODI::RDS::LANGUAGE_INDEX_COUNT; ++index)
  {
    const std::string_view code{KODI::RDS::LanguageCode(index)};
    if (code.empty())
      continue;

    const CLanguageTag language{KODI::RDS::Language(index)};
    EXPECT_TRUE(language.IsValid())
        << "index " << index << " holds '" << code << "', which names no language";
    EXPECT_FALSE(language.IsUndetermined()) << "index " << index << " holds '" << code << "'";
  }
}

TEST(TestRadioRDSLanguages, AnIndexNamingNoLanguageIsUndetermined)
{
  // 0x00 is reserved, as is the block from 0x2C to 0x44
  for (const unsigned int index : {0x00U, 0x2CU, 0x3FU, 0x44U})
  {
    EXPECT_TRUE(KODI::RDS::LanguageCode(index).empty()) << "index " << index;
    EXPECT_TRUE(KODI::RDS::Language(index).IsUndetermined()) << "index " << index;
  }

  // and so is anything past the table
  EXPECT_TRUE(KODI::RDS::LanguageCode(KODI::RDS::LANGUAGE_INDEX_COUNT).empty());
  EXPECT_TRUE(KODI::RDS::Language(KODI::RDS::LANGUAGE_INDEX_COUNT).IsUndetermined());
  EXPECT_TRUE(KODI::RDS::Language(0xFFFFU).IsUndetermined());
}

// Cells pinned against EBU Tech 3244 Annex J. From 0x45 the standard runs reverse-alphabetically
// by English name, which fixes which language each index names independently of the code
// printed for it.
TEST(TestRadioRDSLanguages, NamesTheLanguageTheStandardAssignsToAnIndex)
{
  EXPECT_EQ(KODI::RDS::Language(0x09).ToString(), "en");
  EXPECT_EQ(KODI::RDS::Language(0x08).ToString(), "de");
  EXPECT_EQ(KODI::RDS::Language(0x0F).ToString(), "fr");

  // 0x0E is Faroese, whose ISO 639-2 code is fao
  EXPECT_EQ(KODI::RDS::LanguageCode(0x0E), "fao");
  EXPECT_TRUE(KODI::RDS::Language(0x0E).Matches(CLanguageTag::Parse("fo")));

  // 0x61 is Malay, whose ISO 639-2/B code is may
  EXPECT_EQ(KODI::RDS::LanguageCode(0x61), "may");
  EXPECT_TRUE(KODI::RDS::Language(0x61).Matches(CLanguageTag::Parse("ms")));

  // 0x64 is Laotian, between Macedonian and Korean in the reverse-alphabetical run
  EXPECT_EQ(KODI::RDS::LanguageCode(0x64), "lao");
  EXPECT_EQ(KODI::RDS::Language(0x64).ToString(), "lo");

  // The standard names some languages twice, once in each block, and the two agree
  EXPECT_EQ(KODI::RDS::Language(0x22), KODI::RDS::Language(0x60)); // Romanian
  EXPECT_EQ(KODI::RDS::Language(0x1D), KODI::RDS::Language(0x2A)); // Dutch, as dut and nld
}

// The standard prints ISO 639-2/B codes, two of which ISO has since withdrawn. A withdrawn code
// names nothing, so the table holds the code that survived for the same language.
TEST(TestRadioRDSLanguages, HoldsTheSurvivingCodeWhereTheStandardPrintsAWithdrawnOne)
{
  // 0x54 is Serbo-Croat, printed as scc, withdrawn 2008-06-28. Serbian has its own index at
  // 0x24, so the survivor is the ISO 639-3 macrolanguage hbs, not srp
  EXPECT_EQ(KODI::RDS::LanguageCode(0x54), "hbs");
  EXPECT_EQ(KODI::RDS::Language(0x54).ToString(), "sh");
  EXPECT_NE(KODI::RDS::Language(0x54), KODI::RDS::Language(0x24));

  // 0x60 is Moldavian, printed as mol, withdrawn as a language distinct from Romanian
  EXPECT_EQ(KODI::RDS::LanguageCode(0x60), "rum");
  EXPECT_EQ(KODI::RDS::Language(0x60).ToString(), "ro");
}

// The standard names two languages ISO 639-2 has no code for, and ISO 639-3 has
TEST(TestRadioRDSLanguages, HoldsAnIso6393CodeWhereIso6392HasNone)
{
  EXPECT_EQ(KODI::RDS::Language(0x55).ToString(), "rue"); // Rusyn
  EXPECT_EQ(KODI::RDS::Language(0x73).ToString(), "prs"); // Dari
}

} // unnamed namespace
