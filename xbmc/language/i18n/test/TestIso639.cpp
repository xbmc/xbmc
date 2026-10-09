/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "language/i18n/Iso639.h"

#include <string>
#include <string_view>

#include <gtest/gtest.h>

using namespace KODI::LANGUAGE::I18N;

TEST(TestIso639, NamesTheAlpha3CodeOfAnAlpha2One)
{
  EXPECT_EQ(CIso639::Alpha2ToAlpha3B("en"), "eng");
  EXPECT_EQ(CIso639::Alpha2ToAlpha3B("fr"), "fre"); // the bibliographic form, where the two differ
}

// ISO withdrew the bibliographic codes scr and scc on 2008-06-28, leaving hrv and srp as both
// forms of Croatian and Serbian. Neither may be mapped through the Serbo-Croatian entry.
TEST(TestIso639, ResolvesTheLanguagesWhoseBibliographicCodeWasWithdrawn)
{
  EXPECT_EQ(CIso639::Alpha3ToAlpha2("hrv"), "hr");
  EXPECT_EQ(CIso639::Alpha3ToAlpha2("srp"), "sr");

  // and the codes that replaced them are what the language is named by now
  EXPECT_EQ(CIso639::Alpha2ToAlpha3B("hr"), "hrv");
  EXPECT_EQ(CIso639::Alpha2ToAlpha3B("sr"), "srp");
}

TEST(TestIso639, NamesTheAlpha3CodeOfAWithdrawnAlpha2One)
{
  // Media tagged with a spelling ISO 639-1 has withdrawn still has to be understood
  EXPECT_EQ(CIso639::Alpha2ToAlpha3B("iw"), "heb");
  EXPECT_EQ(CIso639::Alpha2ToAlpha3B("in"), "ind");
  EXPECT_EQ(CIso639::Alpha2ToAlpha3B("ji"), "yid");
  EXPECT_EQ(CIso639::Alpha2ToAlpha3B("jw"), "jav");
  EXPECT_EQ(CIso639::Alpha2ToAlpha3B("mo"), "rum");
}

// bh and sh were withdrawn with no alpha-2 code to replace them, so they still answer for their
// languages in both directions
TEST(TestIso639, ConvertsAWithdrawnAlpha2CodeWithoutAReplacementBothWays)
{
  EXPECT_EQ(CIso639::Alpha2ToAlpha3B("bh"), "bih");
  EXPECT_EQ(CIso639::Alpha2ToAlpha3B("sh"), "hbs");

  EXPECT_EQ(CIso639::Alpha3ToAlpha2("bih"), "bh");
  EXPECT_EQ(CIso639::Alpha3ToAlpha2("hbs"), "sh");
}

TEST(TestIso639, NamesTheCurrentAlpha2CodeOfALanguageNotAWithdrawnOne)
{
  EXPECT_EQ(CIso639::Alpha3ToAlpha2("heb"), "he");
  EXPECT_EQ(CIso639::Alpha3ToAlpha2("ind"), "id");
  EXPECT_EQ(CIso639::Alpha3ToAlpha2("yid"), "yi");
  EXPECT_EQ(CIso639::Alpha3ToAlpha2("jav"), "jv");
  EXPECT_EQ(CIso639::Alpha3ToAlpha2("rum"), "ro");
}

TEST(TestIso639, NamesTheAlpha2CodeOfAnAlpha3One)
{
  EXPECT_EQ(CIso639::Alpha3ToAlpha2("eng"), "en");

  // Either form of a language that spells its two differently
  EXPECT_EQ(CIso639::Alpha3ToAlpha2("tib"), CIso639::Alpha3ToAlpha2("bod"));
}

TEST(TestIso639, AnswersNothingForACodeOutsideTheStandard)
{
  // Each direction takes only the code length its standard assigns
  EXPECT_FALSE(CIso639::Alpha2ToAlpha3B("eng").has_value());
  EXPECT_FALSE(CIso639::Alpha2ToAlpha3B("ac").has_value());
  EXPECT_FALSE(CIso639::Alpha2ToAlpha3B("").has_value());

  EXPECT_FALSE(CIso639::Alpha3ToAlpha2("en").has_value());
  EXPECT_FALSE(CIso639::Alpha3ToAlpha2("zzz").has_value());
  EXPECT_FALSE(CIso639::Alpha3ToAlpha2("").has_value());

  // ISO 639-2 codes many more languages than ISO 639-1 does, and most have no alpha-2 counterpart
  for (const std::string_view code : {"und", "zxx", "mis", "mul"})
  {
    EXPECT_FALSE(CIso639::Alpha3ToAlpha2(code).has_value()) << code;
    EXPECT_FALSE(CIso639::Alpha2ToAlpha3B(code).has_value()) << code;
  }
}

TEST(TestIso639, MapsTheTwoFormsOfAnIso6392Code)
{
  EXPECT_EQ(CIso639::TCodeToBCode("bod"), "tib");
  EXPECT_EQ(CIso639::BCodeToTCode("tib"), "bod");

  // A language spelling both forms alike has no other form to map to
  EXPECT_FALSE(CIso639::TCodeToBCode("zha").has_value());
  EXPECT_FALSE(CIso639::BCodeToTCode("zha").has_value());
}
