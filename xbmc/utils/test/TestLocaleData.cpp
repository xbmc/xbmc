/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/LocaleData.h"
//#include "application/Application.h"
#include "LangInfo.h"

#include <gtest/gtest.h>

/*!
 * \brief Finds an ISO639_1 code for a language id
 *
 * \param  langId Either an ISO-639 code, language addon name or
 *         native language name from the ISO-639 table
 * \return the ISO-639-1 code for the given langId
 *         or an empty string if no match is made.
 */
// static const std::string GetISO639_1(std::string_view langId);

TEST(TestLocaleData, GetISO639_1)
{
  std::string testString;
  std::string result;
  std::string expectedResult;

  testString = "en";
  expectedResult = "en";
  result = CLocaleData::GetISO639_1(testString);
  EXPECT_STREQ(expectedResult.c_str(), result.c_str());

  testString = "eng";
  expectedResult = "en";
  result = CLocaleData::GetISO639_1(testString);
  EXPECT_STREQ(expectedResult.c_str(), result.c_str());

  testString = " eNG ";
  expectedResult = "en";
  result = CLocaleData::GetISO639_1(testString);
  EXPECT_STREQ(expectedResult.c_str(), result.c_str());

  testString = "englisH  ";
  expectedResult = "en";
  result = CLocaleData::GetISO639_1(testString);
  EXPECT_STREQ(expectedResult.c_str(), result.c_str());

  // there is no ISO639_1 code for Filipino, but there are
  // ISO639_2T/B and ISO639_3 codes
  testString = "filipino";
  expectedResult = "";
  result = CLocaleData::GetISO639_1(testString);
  EXPECT_STREQ(expectedResult.c_str(), result.c_str());
}

/*!
 * \brief Finds an ISO639_2B code for a language id
 *
 * \param  langId Either an ISO-639 code, language addon name or
 *         native language name from the ISO-639 table
 * \return the ISO-639-2B code for the given langId
 *         or an empty string if no match is made.
 */
// static const std::string GetISO639_2B(std::string_view langId);
TEST(TestLocaleData, GetISO639_2B)
{
  std::string testString;
  std::string result;
  std::string expectedResult;

  testString = "eng";
  expectedResult = "eng";
  result = CLocaleData::GetISO639_2B(testString);
  EXPECT_STREQ(expectedResult.c_str(), result.c_str());

  // In the test environment, it seems en_GB is the only one available
  testString = "English";
  expectedResult = "eng";
  result = CLocaleData::GetISO639_2B(testString); // 'official' name in ISO639 table
  EXPECT_STREQ(expectedResult.c_str(), result.c_str());

  testString = "fil";
  expectedResult = "fil";
  result = CLocaleData::GetISO639_2B(testString);
  EXPECT_STREQ(expectedResult.c_str(), result.c_str());

  testString = " FiLIPino ";
  expectedResult = "fil";
  result = CLocaleData::GetISO639_2B(testString);
  EXPECT_STREQ(expectedResult.c_str(), result.c_str());

  // In the test environment, it seems en_GB is the only one available
  testString = "English";
  expectedResult = "eng";
  result = CLocaleData::GetISO639_2B(testString); // 'official' name in ISO639 table
  EXPECT_STREQ(expectedResult.c_str(), result.c_str());

  testString = "fil";
  expectedResult = "fil";
  result = CLocaleData::GetISO639_2B(testString);
  EXPECT_STREQ(expectedResult.c_str(), result.c_str());
}

/*!
 * \brief Finds an ISO639_2T code for a language id
 *
 * \param  langId Either an ISO-639 code, language addon name or
 *         native language name from the ISO-639 table
 * \return the ISO-639-2T code for the given langId
 *         or an empty string if no match is made.
 */
// static const std::string GetISO639_2T(std::string_view langId);

TEST(TestLocaleData, GetISO639_2T)
{
  std::string testString;
  std::string result;
  std::string expectedResult;

  testString = "eng";
  expectedResult = "eng";
  result = CLocaleData::GetISO639_2T(testString);
  EXPECT_STREQ(expectedResult.c_str(), result.c_str());

  // Here the 2B and 2T codes are different

  testString = "Tibetan";
  expectedResult = "tib";
  result = CLocaleData::GetISO639_2B(testString);
  EXPECT_STREQ(expectedResult.c_str(), result.c_str());

  testString = " Tibetan ";
  expectedResult = "bod";
  result = CLocaleData::GetISO639_2T(testString);
  EXPECT_STREQ(expectedResult.c_str(), result.c_str());

  testString = "FiLIPino";
  expectedResult = "fil";
  result = CLocaleData::GetISO639_2T(testString);
  EXPECT_STREQ(expectedResult.c_str(), result.c_str());
}

/*!
 * \brief Finds an ISO639_3 code for a language id
 *
 * \param  langId Either an ISO-639 code, language addon name or
 *         native language name from the ISO-639 table
 * \return the ISO-639-3 code for the given langId
 *         or an empty string if no match is made.
 */
// static const std::string GetISO639_3(std::string_view langId);

TEST(TestLocaleData, GetISO639_3)
{
  std::string testString;
  std::string result;
  std::string expectedResult;

  testString = "eng";
  expectedResult = "eng";
  result = CLocaleData::GetISO639_3(testString);
  EXPECT_STREQ(expectedResult.c_str(), result.c_str());

  testString = "filipino";
  expectedResult = "fil";
  result = CLocaleData::GetISO639_3(testString);
  EXPECT_STREQ(expectedResult.c_str(), result.c_str());
}

/*!
 * \brief Finds an ISO-3166-1-Alpha2 code for a region id
 *
 * \param  regionId Either an ISO-3166-1 code, language addon name or
 *         native region name from the ISO-3166-1 table
 * \return the ISO-3166-1-Alpha2 code for the given regionId
 *         or an empty string if no match is made.
 */
// static const std::string GetISO3166_1_Alpha2(std::string_view regionID = "");

TEST(TestLocaleData, GetISO3166_1_Alpha2)
{
  std::string testString;
  std::string result;
  std::string expectedResult;

  testString = "GB";
  expectedResult = "gb";
  result = CLocaleData::GetISO3166_1_Alpha2(testString);
  EXPECT_STREQ(expectedResult.c_str(), result.c_str());

  testString = " Gb ";
  expectedResult = "gb";
  result = CLocaleData::GetISO3166_1_Alpha2(testString);
  EXPECT_STREQ(expectedResult.c_str(), result.c_str());
}

/*!
 * \brief Finds an ISO-3166-1-Alpha3 code for a region id
 *
 * \param  regionId Either an ISO-3166-1 code, language addon name or
 *         native region name from the ISO-3166-1 table
 * \return the ISO-3166-1-Alpha3 code for the given regionId
 *         or an empty string if no match is made.
 */
// static const std::string GetISO3166_1_Alpha3(std::string_view regionID = "");

TEST(TestLocaleData, GetISO3166_1_Alpha3)
{
  std::string testString;
  std::string result;
  std::string expectedResult;

  testString = " Gb ";
  expectedResult = "gbr";
  result = CLocaleData::GetISO3166_1_Alpha3(testString);
  EXPECT_STREQ(expectedResult.c_str(), result.c_str());

  testString = "English";
  expectedResult = "gbr";
  result = CLocaleData::GetISO3166_1_Alpha3(testString);
  EXPECT_STREQ(expectedResult.c_str(), result.c_str());

  testString = "Kiribati";
  expectedResult = "kir";
  result = CLocaleData::GetISO3166_1_Alpha3(testString);
  EXPECT_STREQ(expectedResult.c_str(), result.c_str());
}

/*!
 * \brief Gets basic c++ - style locale information for a language addon
 *
 * \param languageAddonName addon name from the resource.language.xx/addon.xml
 *        file that defines this language to Kodi.
 *
 *        Ex: resource.language.et_ee/addon.xml  defines a name of "Estonian"
 *
 * \return BasicLocaleInfo object that wraps locale-related info
 */
// static const BasicLocaleInfo GetCLocaleInfoForLocaleName(std::string languageAddonName = "");

TEST(TestLocaleData, GetCLocaleInfoForLocaleName)
{
  std::string testString;
  BasicLocaleInfo result;

  testString = " engLish ";
  result = CLocaleData::GetCLocaleInfoForLocaleName(testString);
  EXPECT_STREQ(result.GetName().c_str(), "English");
  EXPECT_STREQ(result.GetCodeset().c_str(), "UTF-8");
  EXPECT_STREQ(result.GetLanguageCode().c_str(), "en");
  EXPECT_STREQ(result.GetRegionCode().c_str(), "GB");
  EXPECT_STREQ(result.GetModifier().c_str(), "");
}

/*!
 * \brief Creates a POSIX style Locale name suitable for use with std::locale
 *
 * Resulting string is composed:
 *   ToLower(<languageId>)_ToUpper(<regionId>).ToUpper(<encodingId>)@<modifierId>
 *
 * \param languageId language id, typically iso6639_1, iso6639_2T, iso6639_2B or
 *                   iso6639_3 format
 * \param regionId [OPT] region or country code, typically in iso3166_1_Alpha1
 *                       or iso3166_2_Alpha2 format
 *  \param encodingId [OPT] specifies the encoding, such as UTF-8. Defaults to
 *         Kodi's encoding for the platform
 *  \param modifierId [OPT] modifier to the locale. specifies some variant of
 *                          the locale, such as Latin or Cyrillic for a country
 *                          that uses either alphabet for the same language
 *  \return C++ Locale name representing the addons locale.
 *
 *  Example: en_US.UTF-8 or sr_RS.UTF-8@latin
 */
// static const std::string BuildCLocaleName(const std::string_view languageId,
//     const std::string_view regionId = "", const std::string_view encodingId = "",
//     const std::string_view modifierId = "");

TEST(TestLocaleData, BuildCLocaleName)
{
  std::string languageId;
  std::string regionId;
  std::string encodingId;
  std::string modifierId;

  std::string result;
  std::string expectedResult;

  languageId = "";
  expectedResult = "";
  result = CLocaleData::BuildCLocaleName(languageId);
  EXPECT_STREQ(expectedResult.c_str(), result.c_str());

  languageId = "En";
  regionId = "gB";
  encodingId = "Utf-8";
  modifierId = "";
  expectedResult = "en_GB.UTF-8";
  result = CLocaleData::BuildCLocaleName(languageId, regionId, encodingId, modifierId);
  EXPECT_STREQ(expectedResult.c_str(), result.c_str());

  languageId = "fil";
  regionId = "";
  encodingId = "UTF-8";
  modifierId = "";
  expectedResult = "fil.UTF-8";
  result = CLocaleData::BuildCLocaleName(languageId, regionId, encodingId, modifierId);
  EXPECT_STREQ(expectedResult.c_str(), result.c_str());

  languageId = "fil";
  regionId = "ph";
  encodingId = "UTF-8";
  modifierId = "";
  expectedResult = "fil_PH.UTF-8";
  result = CLocaleData::BuildCLocaleName(languageId, regionId, encodingId, modifierId);
  EXPECT_STREQ(expectedResult.c_str(), result.c_str());
}

/*!
 * \brief Creates a POSIX style Locale name suitable for use with std::locale
 *
 * Same function as above version except all arguments come from BasicLocaleInfo
 * object.
 *
 * Resulting string is composed:
 *   ToLower(<languageId>)_ToUpper(<regionId>).ToUpper(<encodingId>)@<modifierId>
 *
 * \param basicInfo supplies all of the information to create the Locale Name
 *
 * \return C++ Locale name representing the addons locale.
 *
 *  Example: en_US.UTF-8 or sr_RS.UTF-8@latin
 */
// static const std::string BuildCLocaleName(BasicLocaleInfo basicInfo);

TEST(TestLocaleData, BuildCLocaleName_2)
{
  std::string languageId;
  std::string regionId;
  std::string encodingId;
  std::string modifierId;
  std::string expectedResult;

  std::string testString;
  BasicLocaleInfo result;
  std::string result2;

  testString = " engLish ";
  result = CLocaleData::GetCLocaleInfoForLocaleName(testString);
  EXPECT_STREQ(result.GetName().c_str(), "English");
  EXPECT_STREQ(result.GetCodeset().c_str(), "UTF-8");
  EXPECT_STREQ(result.GetLanguageCode().c_str(), "en");
  EXPECT_STREQ(result.GetRegionCode().c_str(), "GB");
  EXPECT_STREQ(result.GetModifier().c_str(), "");

  languageId = "";
  expectedResult = "en_GB.UTF-8";
  result2 = CLocaleData::BuildCLocaleName(result);
  EXPECT_STREQ(expectedResult.c_str(), result2.c_str());
}

/*!
 * \brief Verifies if the locale generated by BuildCLocaleName is valid
 *        on this machine as currently configured.
 *
 *        Simply creates a std::locale instance using the Locale string
 *        from BuildCLocaleName. If the creation fails, then the Locale is
 *        invalid.
 *
 *        Note that failure can be due to any reason, including the language
 *        or country codes not installed or configured.
 *
 * \return true if a locale can be created from the generated locale name
 *         false, otherwise
 */
// static bool IsLocaleValid(const std::string& localeName);

TEST(TestLocaleData, IsLocaleValid)
{
  std::string testString;

  bool result;
  bool expectedResult;

  // Can only test locales that are guaranteed to be on test
  // machine.

  testString = "en_GB.UTF-8";
  expectedResult = true;
  result = CLocaleData::IsLocaleValid(testString);
  EXPECT_EQ(expectedResult, result);
}

/*!
 * \brief Generate a Locale name for the current language
 *        suitable for use with std::locale
 *
 *        Ex: en_US.UTF-8 or sr_RS.UTF-8@latin
 *
 * \return locale name
 */
// static const std::string GetCurrentCLocaleName();

/*
 * Can not test without core on SetLanguage. Due to other
 * initialization required.
 *
TEST(TestLocaleData, GetCurrentCLocaleName)
{
  std::string languageId;
  std::string regionId;
  std::string encodingId;
  std::string modifierId;

  std::string result;
  std::string expectedResult;

  // EXPECT_True(g_application::LoadLanguage(true));
  bool rc = g_langInfo.SetLanguage("English", true);
  EXPECT_TRUE(rc);

  // EXPECT_TRUE(CLangInfo::Load("English"));

  expectedResult = "en_GB.UTF-8";
  result = CLocaleData::GetCurrentCLocaleName();
  EXPECT_STREQ(expectedResult.c_str(), result.c_str());
}
 */

/*!
 * \brief Finds the 'native' language name for a language id
 *
 * \param  langId Either an ISO-639 code, language addon name or
 *         native language name from the ISO-639 table
 * \return the language name for the given langId from the ISO-639 table
 *         or an empty string if no match is made.
 */
// static const std::string GetLanguageName(std::string_view langId);

TEST(TestLocaleData, GetLanguageName)
{
  std::string languageId;
  std::string regionId;
  std::string encodingId;
  std::string modifierId;

  std::string result;
  std::string expectedResult;

  expectedResult = "English";
  result = CLocaleData::GetLanguageName("en");
  EXPECT_STREQ(expectedResult.c_str(), result.c_str());

  // Use language addon name here. English is only one available
  // in Test environment. Haven't figured out how to add more at this
  // time.

  expectedResult = "English";
  result = CLocaleData::GetLanguageName(" EnGlish ");
  EXPECT_STREQ(expectedResult.c_str(), result.c_str());

  expectedResult = "Rajasthani";
  result = CLocaleData::GetLanguageName(" raJ ");
  EXPECT_STREQ(expectedResult.c_str(), result.c_str());

  languageId = " FrR ";
  expectedResult = "Northern Frisian";
  result = CLocaleData::GetLanguageName(languageId);
  EXPECT_STREQ(expectedResult.c_str(), result.c_str());
}

/*!
 * \brief Given an ISO3166_1 region id or name,
 *
 *        This function also serves as a filter/validator for other functions.
 *
 * \param regionId [OPT] language addon name or ISO3166_1 code.
 *                 When missing, the region of the current language is used.
 *
 *  \return the 'native' region name from the IS03166_1 table, or empty string
 *          when there is no match.
 */
// static const std::string GetRegionName(std::string_view regionId);

TEST(TestLocaleData, GetRegionName)
{
  std::string languageId;
  std::string regionId;
  std::string encodingId;
  std::string modifierId;

  std::string result;
  std::string expectedResult;

  expectedResult = "";
  result = CLocaleData::GetRegionName("en");
  EXPECT_STREQ(expectedResult.c_str(), result.c_str());

  // Use language addon name here. English is only one available
  // in Test environment. Haven't figured out how to add more at this
  // time.

  expectedResult = "United States of America (the)";
  result = CLocaleData::GetRegionName("us");
  EXPECT_STREQ(expectedResult.c_str(), result.c_str());

  expectedResult = "Ukraine";
  result = CLocaleData::GetRegionName("Ua");
  EXPECT_STREQ(expectedResult.c_str(), result.c_str());

  expectedResult = "Liechtenstein";
  result = CLocaleData::GetRegionName("Lie");
  EXPECT_STREQ(expectedResult.c_str(), result.c_str());

  expectedResult = "United Kingdom of Great Britain and Northern Ireland (the)";
  result = CLocaleData::GetRegionName("gBr");
  EXPECT_STREQ(expectedResult.c_str(), result.c_str());

  expectedResult = "United Kingdom of Great Britain and Northern Ireland (the)";
  result = CLocaleData::GetRegionName("United Kingdom of Great Britain and Northern Ireland (the)");
  EXPECT_STREQ(expectedResult.c_str(), result.c_str());
}

/*!
 *  \brief Determines if two English language names represent the same language.
 *   \param[in] lang1 The first language string to compare given as english language name.
 *   \param[in] lang2 The second language string to compare given as english language name.
 *   \return true if the two language strings represent the same language, false otherwise.
 *   For example "Abkhaz" and "Abkhazian" represent the same language.
 */
// static bool CompareFullLanguageNames(const std::string_view lang1,
//    const std::string_view lang2);
