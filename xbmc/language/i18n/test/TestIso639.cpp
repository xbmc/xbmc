/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "language/i18n/Iso639.h"

#include <gtest/gtest.h>

using namespace KODI::LANGUAGE::I18N;

TEST(TestIso639, NamesTheAlpha3CodeOfAnAlpha2One)
{
  EXPECT_EQ(CIso639::Alpha2ToAlpha3B("en"), "eng");
  EXPECT_EQ(CIso639::Alpha2ToAlpha3B("fr"), "fre"); // the bibliographic form, where the two differ
}

// Croatian and Serbian lost their distinct bibliographic codes in 2008, and hrv and srp became
// both forms. While the T-to-B map still sent them to scr and scc, hrv resolved through the entry
// TableLanguageCodes holds for Serbo-Croatian and came back as sh, and srp reached nothing at all.
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
  EXPECT_EQ(CIso639::Alpha2ToAlpha3B("mo"), "rum");
}

TEST(TestIso639, TakesOnlyAnAlpha2CodeForTheAlpha3One)
{
  EXPECT_FALSE(CIso639::Alpha2ToAlpha3B("eng").has_value());
  EXPECT_FALSE(CIso639::Alpha2ToAlpha3B("ac").has_value());
  EXPECT_FALSE(CIso639::Alpha2ToAlpha3B("").has_value());
}

TEST(TestIso639, NamesTheAlpha2CodeOfAnAlpha3One)
{
  EXPECT_EQ(CIso639::Alpha3ToAlpha2("eng"), "en");
  EXPECT_EQ(CIso639::Alpha3ToAlpha2("tib"), "bo"); // bibliographic
  EXPECT_EQ(CIso639::Alpha3ToAlpha2("bod"), "bo"); // terminological
}

TEST(TestIso639, IgnoresCaseAndSurroundingSpace)
{
  EXPECT_EQ(CIso639::Alpha3ToAlpha2(" eng "), "en");
  EXPECT_EQ(CIso639::Alpha3ToAlpha2("ENG"), "en");
  EXPECT_EQ(CIso639::Alpha2ToAlpha3B(" EN "), "eng");
}

TEST(TestIso639, TakesOnlyAnAlpha3CodeForTheAlpha2One)
{
  EXPECT_FALSE(CIso639::Alpha3ToAlpha2("en").has_value());
  EXPECT_FALSE(CIso639::Alpha3ToAlpha2("zzz").has_value());
  EXPECT_FALSE(CIso639::Alpha3ToAlpha2("").has_value());
}

TEST(TestIso639, AnswersNothingWhereIso6391AssignsNoCode)
{
  // ISO 639-2 codes many more languages than ISO 639-1 does, and most have no alpha-2 counterpart
  EXPECT_FALSE(CIso639::Alpha3ToAlpha2("ady").has_value());
  EXPECT_FALSE(CIso639::Alpha3ToAlpha2("und").has_value());
}
