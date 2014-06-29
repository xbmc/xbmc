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

  enum LANGFORMATS
  {
    ISO_639_1,
    ISO_639_2,
    ENGLISH_NAME
  };

  CLangCodeExpander(void);
  ~CLangCodeExpander(void);

  bool Lookup(std::string& desc, const std::string& code);
  bool Lookup(std::string& desc, const int code);

  /** \brief Determines if two english language names represent the same language.
  *   \param[in] lang1 The first language string to compare given as english language name.
  *   \param[in] lang2 The second language string to compare given as english language name.
  *   \return true if the two language strings represent the same language, false otherwise.
  *   For example "Abkhaz" and "Abkhazian" represent the same language.
  */ 
  bool CompareFullLangNames(const std::string& lang1, const std::string& lang2);

  /** \brief Determines if two languages given as ISO 639-1, ISO 639-2/T, or ISO 639-2/B codes represent the same language.
  *   \param[in] code1 The first language to compare given as ISO 639-1, ISO 639-2/T, or ISO 639-2/B code.
  *   \param[in] code2 The second language to compare given as ISO 639-1, ISO 639-2/T, or ISO 639-2/B code.
  *   \return true if the two language codes represent the same language, false otherwise.
  *   For example "ger", "deu" and "de" represent the same language.
  */ 
  bool CompareLangCodes(const std::string& code1, const std::string& code2);

  /** \brief Converts a language given as 2-Char (ISO 639-1),
  *          3-Char (ISO 639-2/T or ISO 639-2/B),
  *          or full english name string to a 2-Char (ISO 639-1) code.  
  *   \param[out] code The 2-Char language code of the given language lang.
  *   \param[in] lang The language that should be converted.
  *   \param[in] checkXbmcLocales Try to find in XBMC specific locales
  *   \return true if the conversion succeeded, false otherwise. 
  */ 
  bool ConvertToTwoCharCode(std::string& code, const std::string& lang, bool checkXbmcLocales = true);

  /** \brief Converts a language given as 2-Char (ISO 639-1),
  *          3-Char (ISO 639-2/T or ISO 639-2/B),
  *          or full english name string to a 3-Char ISO 639-2/T code.
  *   \param[in] lang The language that should be converted.
  *   \return The 3-Char ISO 639-2/T code of lang if that code exists, lang otherwise.
  */
  std::string ConvertToISO6392T(const std::string& lang);
  static bool ConvertTwoToThreeCharCode(std::string& strThreeCharCode, const std::string& strTwoCharCode, bool checkWin32Locales = false);
  static bool ConvertToThreeCharCode(std::string& strThreeCharCode, const std::string& strCharCode, bool checkXbmcLocales = true, bool checkWin32Locales = false);

#ifdef TARGET_WINDOWS
  static bool ConvertLinuxToWindowsRegionCodes(const std::string& strTwoCharCode, std::string& strThreeCharCode);
  static bool ConvertWindowsToGeneralCharCode(const std::string& strWindowsCharCode, std::string& strThreeCharCode);
#endif

  void LoadUserCodes(const TiXmlElement* pRootElement);
  void Clear();

  static std::vector<std::string> GetLanguageNames(LANGFORMATS format = ISO_639_1);
protected:

  /** \brief Converts a language code given as a long, see #MAKECODE(a, b, c, d)
  *          to its string representation.
  *   \param[in] code The language code given as a long, see #MAKECODE(a, b, c, d).
  *   \param[out] ret The string representation of the given language code code.
  */ 
  static void CodeToString(long code, std::string& ret);

  typedef std::map<std::string, std::string> STRINGLOOKUPTABLE;
  STRINGLOOKUPTABLE m_mapUser;

  static bool LookupInDb(std::string& desc, const std::string& code);
  bool LookupInMap(std::string& desc, const std::string& code);

  /** \brief Looks up the ISO 639-1, ISO 639-2/T, or ISO 639-2/B, whichever it finds first,
  *          code of the given english language name.
  *   \param[in] desc The english language name for which a code is looked for.
  *   \param[out] code The ISO 639-1, ISO 639-2/T, or ISO 639-2/B code of the given language desc.
  *   \return true if the a code was found, false otherwise.
  */ 
  bool ReverseLookup(const std::string& desc, std::string& code);
};

extern CLangCodeExpander g_LangCodeExpander;
