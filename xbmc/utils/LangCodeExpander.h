/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <map>
#include <string>
#include <vector>

class TiXmlElement;

class CLangCodeExpander
{
public:
  CLangCodeExpander();
  ~CLangCodeExpander();

  enum LANGFORMATS
  {
    ISO_639_1,
    ISO_639_2,
    ENGLISH_NAME
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

  void LoadUserCodes(const TiXmlElement* pRootElement);
  void Clear();

  bool Lookup(const std::string& code, std::string& desc);
  bool Lookup(const int code, std::string& desc);

  /** \brief Determines if two english language names represent the same language.
  *   \param[in] lang1 The first language string to compare given as english language name.
  *   \param[in] lang2 The second language string to compare given as english language name.
  *   \return true if the two language strings represent the same language, false otherwise.
  *   For example "Abkhaz" and "Abkhazian" represent the same language.
  */
  bool CompareFullLanguageNames(const std::string& lang1, const std::string& lang2);

  /** \brief Determines if two languages given as ISO 639-1, ISO 639-2/T, or ISO 639-2/B codes represent the same language.
  *   \param[in] code1 The first language to compare given as ISO 639-1, ISO 639-2/T, or ISO 639-2/B code.
  *   \param[in] code2 The second language to compare given as ISO 639-1, ISO 639-2/T, or ISO 639-2/B code.
  *   \return true if the two language codes represent the same language, false otherwise.
  *   For example "ger", "deu" and "de" represent the same language.
  */
  bool CompareISO639Codes(const std::string& code1, const std::string& code2);

  /** \brief Converts a language given as 2-Char (ISO 639-1),
  *          3-Char (ISO 639-2/T or ISO 639-2/B),
  *          or full english name string to a 2-Char (ISO 639-1) code.
  *   \param[out] code The 2-Char language code of the given language lang.
  *   \param[in] lang The language that should be converted.
  *   \return true if the conversion succeeded, false otherwise.
  */
  bool ConvertToISO6391(const std::string& lang, std::string& code);

  /** \brief Converts a language given as 2-Char (ISO 639-1),
  *          3-Char (ISO 639-2/T or ISO 639-2/B),
  *          or full english name string to a 3-Char ISO 639-2/B code.
  *   \param[in] lang The language that should be converted.
  *   \return The 3-Char ISO 639-2/B code of lang if that code exists, lang otherwise.
  */
  std::string ConvertToISO6392B(const std::string& lang);

  /** \brief Converts a language given as 2-Char (ISO 639-1) to a 3-Char (ISO 639-2/T) code.
  *   \param[in] strISO6391 The language that should be converted.
  *   \param[out] strISO6392B The 3-Char (ISO 639-2/B) language code of the given language strISO6391.
  *   \param[in] checkWin32Locales Whether to also check WIN32 specific language codes.
  *   \return true if the conversion succeeded, false otherwise.
  */
  static bool ConvertISO6391ToISO6392B(const std::string& strISO6391, std::string& strISO6392B, bool checkWin32Locales = false);

  /** \brief Converts a language given as 2-Char (ISO 639-1),
  *          3-Char (ISO 639-2/T or ISO 639-2/B),
  *          or full english name string to a 3-Char ISO 639-2/T code.
  *   \param[in] strCharCode The language that should be converted.
  *   \param[out] strISO6392B The 3-Char (ISO 639-2/B) language code of the given language strISO6391.
  *   \param[in] checkWin32Locales Whether to also check WIN32 specific language codes.
  *   \return true if the conversion succeeded, false otherwise.
  */
  bool ConvertToISO6392B(const std::string& strCharCode, std::string& strISO6392B, bool checkWin32Locales = false);

  /** \brief Converts a language given as 2-Char (ISO 639-1),
  *          3-Char (ISO 639-2/T or ISO 639-2/B),
  *          or full english name string to a 3-Char ISO 639-2/T code.
  *   \param[in] strCharCode The language that should be converted.
  *   \param[out] strISO6392T The 3-Char (ISO 639-2/T) language code of the given language strISO6391.
  *   \param[in] checkWin32Locales Whether to also check WIN32 specific language codes.
  *   \return true if the conversion succeeded, false otherwise.
  */
  bool ConvertToISO6392T(const std::string& strCharCode, std::string& strISO6392T, bool checkWin32Locales = false);

  /** \brief Converts a language given as 2-Char (ISO 639-1),
  *          3-Char (ISO 639-2/T or ISO 639-2/B),
  *          or full english name string to a 3-Char ISO 639-2/T code.
  *   \param[in] lang The language that should be converted.
  *   \return The 3-Char ISO 639-2/T code of lang if that code exists, lang otherwise.
  */
  std::string ConvertToISO6392T(const std::string& lang);

  /*
   * \brief Find a language code with subtag (e.g. zh-tw, zh-Hans) in to a string.
   *        This function find a limited set of IETF BCP47 specs, so:
   *        language tag + region subtag, or, language tag + script subtag.
   *        The language code can be found if wrapped by curly brackets e.g. {pt-br}.
   * \param str The string where find the language code.
   * \return The language code found in the string, otherwise empty string
   */
  static std::string FindLanguageCodeWithSubtag(const std::string& str);

#ifdef TARGET_WINDOWS
  static bool ConvertISO31661Alpha2ToISO31661Alpha3(const std::string& strISO31661Alpha2, std::string& strISO31661Alpha3);
  static bool ConvertWindowsLanguageCodeToISO6392B(const std::string& strWindowsLanguageCode, std::string& strISO6392B);
#endif

  /*
   * \brief Get the list of language names.
   * \param format [OPT] The format type.
   * \param list [OPT] The type of language list to retrieve.
   * \return The languages
   */
  std::vector<std::string> GetLanguageNames(LANGFORMATS format = ISO_639_1,
                                            LANG_LIST list = LANG_LIST::DEFAULT);

protected:
  /*
   * \brief Converts a language code given as a long, see #MAKECODE(a, b, c, d)
   *        to its string representation.
   * \param[in] code The language code given as a long, see #MAKECODE(a, b, c, d).
   * \return The string representation of the given language code code.
   */
  static std::string CodeToString(long code);

  static bool LookupInISO639Tables(const std::string& code, std::string& desc);

  /*
   * \brief Looks up the language description for given language code
   *        in to the installed language addons.
   * \param[in] code The language code for which description is looked for.
   * \param[out] desc The english language name.
   * \return true if the language description was found, false otherwise.
   */
  static bool LookupInLangAddons(const std::string& code, std::string& desc);

  bool LookupInUserMap(const std::string& code, std::string& desc);

  /** \brief Looks up the ISO 639-1, ISO 639-2/T, or ISO 639-2/B, whichever it finds first,
  *          code of the given english language name.
  *   \param[in] desc The english language name for which a code is looked for.
  *   \param[out] code The ISO 639-1, ISO 639-2/T, or ISO 639-2/B code of the given language desc.
  *   \return true if the a code was found, false otherwise.
  */
  bool ReverseLookup(const std::string& desc, std::string& code);


  /** \brief Looks up the user defined code of the given code or language name.
  *   \param[in] desc The language code or name that should be converted.
  *   \param[out] userCode The user defined language code of the given language desc.
  *   \return true if desc was found, false otherwise.
  */
  bool LookupUserCode(const std::string& desc, std::string &userCode);

  typedef std::map<std::string, std::string> STRINGLOOKUPTABLE;
  STRINGLOOKUPTABLE m_mapUser;
};

extern CLangCodeExpander g_LangCodeExpander;
