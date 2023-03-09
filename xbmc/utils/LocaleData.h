/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "Locale.h"

#include <map>
#include <string>
#include <string_view>
#include <vector>

class TiXmlElement;

class BasicLocaleInfo
{
  /*
   * Wrapper class to contain some Locale related info. These are frequently
   * used together to build a Locale name, or other purposes. So rather
   * than use separate calls for each piece of info, just get them all at
   * once. Also includes access methods as well as one that will generate
   * a Linux/Posix standard Locale name. Other platforms may require
   * other methods.
   */
private:
  std::string m_languageName;
  std::string m_languageCode;
  std::string m_region;
  std::string m_codesetName;
  std::string m_modifier;

public:
  /*!
   * I get the sense that this field was used prior to Unicode migration
   * but is now generally boring since it should mean UTF-8. Currently
   * this is an empty string from Locale for every addon locale.
   *
   * However, we still need to know what string to put in the Local Name to
   * feed to the OS. Is a ".UTF-8" suffix required, or forbidden?
   * So for now forcing this to be UTF-8 indicating the real desired
   * encoding. Then, leave it up to BuildCLocaleName to generate the
   * platform specific string. That is the dream anyway, but need to
   * learn the rules first. For now, this is only for informing Python
   * addons of what the language settings are for Kodi.
   *
   * Also note that other encodings may still be required by kodi for
   * different purposes. Perhaps Windows GUI doesn't support Unicode, eh?
   * (I think this is changing).
   */

  inline static const std::string DEFAULT_CODESET = "UTF-8";
  BasicLocaleInfo() {}
  BasicLocaleInfo(const std::string languageName,
                  const std::string languageCode,
                  const std::string region,
                  const std::string codesetName,
                  const std::string modifier)
    : m_languageName(languageName),
      m_languageCode(languageCode),
      m_region(region),
      m_codesetName(codesetName),
      m_modifier(modifier)
  {
  }

  /*!
   * \brief Returns the English name of this language/locale, as defined by Kodi
   *
   * \return Name from resource.language file
   */
  const std::string GetName() { return m_languageName; }

  /*!
   * \brief Gets the ISO639 Code as defined by the addon
   *
   * \return ISO639 code: one of (639-1, 639-2T, 639-2B, 639-3)
   */
  const std::string GetLanguageCode() { return m_languageCode; }

  /*!
   * \brief Gets the ISO3166_1 Region/Country/Territory code defined by addon
   *
   *
   * \return ISO3166_1 code: either alpha-1 or alpha2
   */
  const std::string GetRegionCode() { return m_region; }

  /*!
   * \brief Gets the codeset/encoding for the addon
   *
   *        Note: the addon.xml file does not define this, or Kodi ignores it
   *        The value here is hard-coded to UTF-8
   * \return 'UTF-8'
   */
  const std::string GetCodeset()
  {
    if (m_codesetName.empty())
      m_codesetName = DEFAULT_CODESET;
    return m_codesetName;
  }

  /*!
   * \brief Gets any locale modifier value for this addon
   *
   *        Note that these are rarely used. One use may be for a country
   *        that uses two alphabets for the same language. One example is
   *        'latin' for serbia. They have a Latin and Cyrillic flavor
   *
   * \return Any modifier defined, or an empty string
   */
  const std::string GetModifier() { return m_modifier; }
};
static const BasicLocaleInfo EmptyBasicLocaleInfo{"", "", "", "", ""};

class CLocaleData
{
public:
  CLocaleData();
  ~CLocaleData();

  enum LANGFORMATS
  {
    ISO_639_1,
    ISO_639_2,
    ENGLISH_NAME
  };

  /*!
   * \brief Finds an ISO639_1 code for a language id
   *
   * \param  langId Either an ISO-639 code, language addon name or
   *         native language name from the ISO-639 table
   * \return the ISO-639-1 code for the given langId
   *         or an empty string if no match is made.
   */
  static const std::string GetISO639_1(std::string_view langId);

  /*!
   * \brief Finds an ISO639_2B code for a language id
   *
   * \param  langId Either an ISO-639 code, language addon name or
   *         native language name from the ISO-639 table
   * \return the ISO-639-2B code for the given langId
   *         or an empty string if no match is made.
   */
  static const std::string GetISO639_2B(std::string_view langId);

  /*!
   * \brief Finds an ISO639_2T code for a language id
   *
   * \param  langId Either an ISO-639 code, language addon name or
   *         native language name from the ISO-639 table
   * \return the ISO-639-2T code for the given langId
   *         or an empty string if no match is made.
   */
  static const std::string GetISO639_2T(std::string_view langId);

  /*!
   * \brief Finds an ISO639_3 code for a language id
   *
   * \param  langId Either an ISO-639 code, language addon name or
   *         native language name from the ISO-639 table
   * \return the ISO-639-3 code for the given langId
   *         or an empty string if no match is made.
   */
  static const std::string GetISO639_3(std::string_view langId);

  /*!
   * \brief Finds the 'native' language name for a language id
   *
   * \param  langId Either an ISO-639 code, language addon name or
   *         native language name from the ISO-639 table
   * \return the language name for the given langId from the ISO-639 table
   *         or an empty string if no match is made.
   */
  static const std::string GetLanguageName(std::string_view langId);

  /*!
   * \brief Finds an ISO-3166-1-Alpha2 code for a region id
   *
   * \param  regionId Either an ISO-3166-1 code, language addon name or
   *         native region name from the ISO-3166-1 table
   * \return the ISO-3166-1-Alpha2 code for the given regionId
   *         or an empty string if no match is made.
   */
  static const std::string GetISO3166_1_Alpha2(std::string_view regionID = "");

  /*!
   * \brief Finds an ISO-3166-1-Alpha3 code for a region id
   *
   * \param  regionId Either an ISO-3166-1 code, language addon name or
   *         native region name from the ISO-3166-1 table
   * \return the ISO-3166-1-Alpha3 code for the given regionId
   *         or an empty string if no match is made.
   */
  static const std::string GetISO3166_1_Alpha3(std::string_view regionID = "");

  static const std::vector<std::string> GetLanguageAddonNames(
      std::string_view languageAddonName = "");
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
  static const BasicLocaleInfo GetCLocaleInfoForLocaleName(std::string_view languageAddonName = "");

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
  static const std::string GetRegionName(std::string_view regionId);

  /*!
   * \brief Generate a Locale name for the current language
   *        suitable for use with std::locale
   *
   *        Ex: en_US.UTF-8 or sr_RS.UTF-8@latin
   *
   * \return locale name
   */
  static const std::string GetCurrentCLocaleName();

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
  static const std::string BuildCLocaleName(const std::string_view languageId,
                                            const std::string_view regionId = "",
                                            const std::string_view encodingId = "",
                                            const std::string_view modifierId = "");

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
  static const std::string BuildCLocaleName(BasicLocaleInfo basicInfo);

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
  static bool IsLocaleValid(const std::string& localeName);

  /*!
   *  \brief Determines if two english language names represent the same language.
  *   \param[in] lang1 The first language string to compare given as english language name.
  *   \param[in] lang2 The second language string to compare given as english language name.
  *   \return true if the two language strings represent the same language, false otherwise.
  *   For example "Abkhaz" and "Abkhazian" represent the same language.
  */
  static bool CompareFullLanguageNames(const std::string_view lang1, const std::string_view lang2);
};
