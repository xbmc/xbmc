/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <map>
#include <optional>
#include <string>
#include <vector>

class TiXmlElement;

class CLangCodeExpander
{
public:
  CLangCodeExpander() = delete;

  enum LANGFORMATS
  {
    ISO_639_1,
    ISO_639_2,
    ENGLISH_NAME,
    ISO_NAME
  };

  enum class LANG_LIST
  {
    // Standard ISO
    DEFAULT,
    // Standard ISO + Language addons
    INCLUDE_ADDONS,
    // Standard ISO + User defined
    // (User defined can override language name of existing codes)
    INCLUDE_USERDEFINED,
    // Standard ISO + Language addons + User defined
    // (User defined can override language name of existing codes)
    INCLUDE_ADDONS_USERDEFINED,
  };

  static void LoadUserCodes(const TiXmlElement* pRootElement);
  static void Clear();

  static bool Lookup(const std::string& code, std::string& desc);

  /** \brief Determines if two languages given as ISO 639-1, ISO 639-2/T, or ISO 639-2/B codes represent the same language.
  *   \param[in] code1 The first language to compare given as ISO 639-1, ISO 639-2/T, or ISO 639-2/B code.
  *   \param[in] code2 The second language to compare given as ISO 639-1, ISO 639-2/T, or ISO 639-2/B code.
  *   \return true if the two language codes represent the same language, false otherwise.
  *   For example "ger", "deu" and "de" represent the same language.
  */
  static bool CompareISO639Codes(const std::string& code1, const std::string& code2);

  /** \brief Converts a language given as 2-Char (ISO 639-1),
  *          3-Char (ISO 639-2/T or ISO 639-2/B),
  *          or full english name string to a 2-Char (ISO 639-1) code.
  *   \param[out] code The 2-Char language code of the given language lang.
  *   \param[in] lang The language that should be converted.
  *   \return true if the conversion succeeded, false otherwise.
  */
  static bool ConvertToISO6391(const std::string& lang, std::string& code);

  /*!
   * \brief Converts a language to a 3-Char ISO 639-2/B code, whatever notation it is given in:
   *        2-Char (ISO 639-1), 3-Char (ISO 639-2/T or ISO 639-2/B), BCP 47 language tag, or full
   *        English name.
   *
   * Narrowing: only the primary language subtag of a BCP 47 tag survives, as region, script and
   * variant subtags have no ISO 639 equivalent. Intended for interfaces with an ISO 639-2/B
   * contract; elsewhere the BCP 47 tag is carried whole.
   *
   * \note Always returns a value. ConvertToISO6392B reports whether the language was recognized
   *       at all.
   *
   * \param[in] lang The language that should be converted.
   * \return The 3-Char ISO 639-2/B code of lang if that code exists, lang otherwise. A language
   *         with no ISO 639-2 code (ISO 639-3/-5 only, ex: zyg) is returned unchanged.
   */
  static std::string AsISO6392B(const std::string& lang);

  /** \brief Converts a language given as 2-Char (ISO 639-1) to a 3-Char (ISO 639-2/B) code.
  *   \param[in] strISO6391 The language that should be converted.
  *   \param[out] strISO6392B The 3-Char (ISO 639-2/B) language code of the given language strISO6391.
  *   \return true if the conversion succeeded, false otherwise.
  */
  static bool ConvertISO6391ToISO6392B(const std::string& strISO6391, std::string& strISO6392B);

  /*!
   * \brief Converts a language given as 3-Char (ISO 639-2/B or /T) to a 2-Char (ISO 639-1) code.
   * \param[in] iso6392 The language that should be converted. Case-insensitive.
   * \param[out] iso6391 The 2-Char (ISO 639-1) language code of the given language.
   * \return true if the conversion succeeded, false otherwise.
   */
  static bool ConvertISO6392ToISO6391(std::string iso6392, std::string& iso6391);

  /** \brief Converts a language given as 2-Char (ISO 639-1),
  *          3-Char (ISO 639-2/T or ISO 639-2/B),
  *          or full english name string to a 3-Char ISO 639-2/B code.
  *   \param[in] strCharCode The language that should be converted.
  *   \param[out] strISO6392B The 3-Char (ISO 639-2/B) language code of the given language.
  *   \return true if the conversion succeeded, false otherwise.
  */
  static bool ConvertToISO6392B(const std::string& strCharCode, std::string& strISO6392B);

  /*
   * \brief Get the list of language names.
   * \param format [OPT] The format type.
   * \param list [OPT] The type of language list to retrieve.
   * \return The languages
   */
  static std::vector<std::string> GetLanguageNames(LANGFORMATS format = ISO_639_1,
                                                   LANG_LIST list = LANG_LIST::DEFAULT);

  /*
   * \brief Converts a language given as 2-Char (ISO 639-1),
   *        3-Char (ISO 639-2/T, ISO 639-2/B), BCP 47 language tag
   *        or full English name string to a BCP 47 tag.
   * \param[in] text The language to convert
   * \param[out] bcp47 The BCP47 language tag
   * \return true if the conversion succeeded, false otherwise.
   */
  static bool ConvertToBcp47(const std::string& text, std::string& bcp47);

  /*!
   * \brief Converts a language to a BCP 47 tag, whatever notation it is given in: 2-Char
   *        (ISO 639-1), 3-Char (ISO 639-2/T, ISO 639-2/B), BCP 47 language tag, or full English
   *        name.
   *
   * Widening: BCP 47 adds region, script and variant subtags to ISO 639, so nothing is lost.
   * Intended for the boundaries where a language enters the application.
   *
   * \note Always returns a value. ConvertToBcp47 reports whether the language was recognized at
   *       all.
   *
   * \param[in] lang The language that should be converted.
   * \return The BCP 47 tag of lang if one exists, lang otherwise.
   */
  static std::string AsBcp47(const std::string& lang);

protected:
  static bool LookupInISO639Tables(const std::string& code, std::string& desc);

  /*!
   * \brief Looks up the language description for the given language code in the installed
   *        language addons.
   * \param[in] code The language code for which a description is looked for.
   * \return The english language name, or nullopt when no addon answers for the code.
   */
  static std::optional<std::string> LookupInLangAddons(const std::string& code);

  static bool LookupInUserMap(const std::string& code, std::string& desc);

  /** \brief Looks up the ISO 639-1 or ISO 639-2/T, whichever it finds first, code of the given
             english language name.
  *   \param[in] desc The english language name for which a code is looked for.
  *   \param[out] code The ISO 639-1 or ISO 639-2/T code of the given language desc.
  *   \return true if a code was found, false otherwise.
  */
  static bool ReverseLookup(const std::string& desc, std::string& code);

  /** \brief Looks up the user defined code of the given code or language name.
  *   \param[in] desc The language code or name that should be converted.
  *   \param[out] userCode The user defined language code of the given language desc.
  *   \return true if desc was found, false otherwise.
  */
  static bool LookupUserCode(const std::string& desc, std::string& userCode);

  typedef std::map<std::string, std::string> STRINGLOOKUPTABLE;

  /*!
   * \brief The language codes a user defined in advancedsettings.xml, for media tagged with
   *        codes that are not in any standard.
   * \note Loaded once at startup and shared process-wide.
   */
  static STRINGLOOKUPTABLE& GetUserCodes();
};
