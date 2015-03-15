#pragma once
/*
 *      Copyright (C) 2005-2013 Team XBMC
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
  *          or full english name string to a 3-Char ISO 639-2/T code.
  *   \param[in] lang The language that should be converted.
  *   \return The 3-Char ISO 639-2/T code of lang if that code exists, lang otherwise.
  */
  std::string ConvertToISO6392T(const std::string& lang);

  /** \brief Converts a language given as 2-Char (ISO 639-1) to a 3-Char (ISO 639-2/T) code.
  *   \param[in] strISO6391 The language that should be converted.
  *   \param[out] strISO6392T The 3-Char (ISO 639-2/T) language code of the given language strISO6391.
  *   \param[in] checkWin32Locales Whether to also check WIN32 specific language codes.
  *   \return true if the conversion succeeded, false otherwise.
  */
  static bool ConvertISO6391ToISO6392T(const std::string& strISO6391, std::string& strISO6392T, bool checkWin32Locales = false);

  /** \brief Converts a language given as 2-Char (ISO 639-1),
  *          3-Char (ISO 639-2/T or ISO 639-2/B),
  *          or full english name string to a 3-Char ISO 639-2/T code.
  *   \param[in] strCharCode The language that should be converted.
  *   \param[out] strISO6392T The 3-Char (ISO 639-2/T) language code of the given language strISO6391.
  *   \param[in] checkWin32Locales Whether to also check WIN32 specific language codes.
  *   \return true if the conversion succeeded, false otherwise.
  */
  bool ConvertToISO6392T(const std::string& strCharCode, std::string& strISO6392T, bool checkWin32Locales = false);

#ifdef TARGET_WINDOWS
  static bool ConvertISO36111Alpha2ToISO36111Alpha3(const std::string& strISO36111Alpha2, std::string& strISO36111Alpha3);
  static bool ConvertWindowsLanguageCodeToISO6392T(const std::string& strWindowsLanguageCode, std::string& strISO6392T);
#endif

  std::vector<std::string> GetLanguageNames(LANGFORMATS format = ISO_639_1, bool customNames = false);
protected:

  /** \brief Converts a language code given as a long, see #MAKECODE(a, b, c, d)
  *          to its string representation.
  *   \param[in] code The language code given as a long, see #MAKECODE(a, b, c, d).
  *   \param[out] ret The string representation of the given language code code.
  */ 
  static void CodeToString(long code, std::string& ret);

  static bool LookupInISO639Tables(const std::string& code, std::string& desc);
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
