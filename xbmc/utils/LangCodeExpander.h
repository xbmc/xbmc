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

#include "utils/StdString.h"
#include <map>

class TiXmlElement;

// TODO: move CLanguageTag declaration to new .h file

/* BCP 47 (RFC 5646) language tag */
class CLanguageTag
{
public:
  CLanguageTag(void);
  CLanguageTag(const std::string& languageTagString);
  CLanguageTag(const CLanguageTag& other);
  CLanguageTag& operator=(const CLanguageTag& other);
  bool operator==(const CLanguageTag& other) const;
  bool operator!=(const CLanguageTag& other) const;
  void Clear(void);
  bool IsValid(void) const;

  bool SetFromString(const std::string& languageTagString);
  std::string GetTagString(void) const;
  bool IsFromWellFormedString(void) const { return m_FromWellFormedString; }

  void SetLanguageSubtag(const std::string& languageSubtag);
  std::string GetLanguageSubtag(void) const { return m_LanguageSubtag; }
  std::string GetLanguageFullSubtag(void) const;

  void SetExtlangSubtag(const std::string& extlangSubtag);
  std::string GetExtlangSubtag(void) const { return m_ExtlangSubtag; }

  void SetScriptSubtag(const std::string& scriptSubtag);
  std::string GetScriptSubtag(void) const;

  void SetRegionSubtag(const std::string& regionSubtag);
  std::string GetRegionSubtag(void) const { return m_RegionSubtag; }

  void SetVariantSubtag(const std::string& variantSubtag);
  std::string GetVariantSubtag(void) const { return m_VariantSubtag; }

  void SetExtensionSubtag(const std::string& extensionSubtag);
  std::string GetExtensionSubtag(void) const { return m_ExtensionSubtag; }

  void SetPrivateuseSubtag(const std::string& privateuseSubtag);
  std::string GetPrivateuseSubtag(void) const { return m_PrivateuseSubtag; }

  bool DecodeTagToNames(void);
  bool IsDecodedToNamesWithoutErrors(void) const { return m_DecodedToNamesWithoutErrors; }

  std::string GetLanguageName(void);
  std::string GetExtlangName(void);
  std::string GetScriptName(void);
  std::string GetRegionName(void);
  std::string GetVariantName(void);
  std::string GetExtensionName(void);
private:
  bool m_FromWellFormedString;
  std::string m_LanguageSubtag;
  std::string m_ExtlangSubtag;
  std::string m_ScriptSubtag;
  std::string m_RegionSubtag;
  std::string m_VariantSubtag;
  std::string m_ExtensionSubtag;
  std::string m_PrivateuseSubtag;
  std::string m_DefaultScriptSubtag;
  void SetSubtagsDefaults(void);

  bool m_DecodedToNames;
  bool m_DecodedToNamesWithoutErrors;
  bool DecodeLanguageSubtag(void);
  std::string m_LanguageName;
  std::string m_ExtlangName;
  std::string m_ScriptName;
  std::string m_RegionName;
  /* TODO: implement variantName and extensionName when needed
  std::string variantName;
  std::string extensionName; */
  void ClearDecodedNames(void);
};

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

  bool Lookup(CStdString& desc, const CStdString& code);
  bool Lookup(CStdString& desc, const int code);

  /** \brief Converts a language given as 2-Char (ISO 639-1),
  *          3-Char (ISO 639-2/T or ISO 639-2/B),
  *          or full english name string to a 2-Char (ISO 639-1) code.  
  *   \param[out] code The 2-Char language code of the given language lang.
  *   \param[in] lang The language that should be converted.
  *   \return true if the conversion succeeded, false otherwise. 
  */ 
  bool ConvertToTwoCharCode(CStdString& code, const CStdString& lang);
#ifdef TARGET_WINDOWS
  bool ConvertTwoToThreeCharCode(CStdString& strThreeCharCode, const CStdString& strTwoCharCode, bool localeHack = false);
  bool ConvertToThreeCharCode(CStdString& strThreeCharCode, const CStdString& strCharCode, bool localeHack = false);
#else
  bool ConvertTwoToThreeCharCode(CStdString& strThreeCharCode, const CStdString& strTwoCharCode);
  bool ConvertToThreeCharCode(CStdString& strThreeCharCode, const CStdString& strCharCode);
#endif

#ifdef TARGET_WINDOWS
  static bool ConvertLinuxToWindowsRegionCodes(const std::string& strTwoCharCode, std::string& strThreeCharCode);
  bool ConvertWindowsToGeneralCharCode(const CStdString& strWindowsCharCode, CStdString& strThreeCharCode);

  /** \brief Convert win32 LCID to RFC 5646 language tag.
  *   \param[in] id The win32 LCID to convert.
  *   \param[out] languageTag The RFC 5646 language tag.
  *   \return true if conversion successful, false otherwise.
  */ 
  static bool ConvertWindowsLCIDtoLanguageTag(LCID id, std::string& languageTag);
#endif

  void LoadUserCodes(const TiXmlElement* pRootElement);
  void Clear();

  std::vector<std::string> GetLanguageNames(LANGFORMATS format = ISO_639_1) const;
  static bool LookupInDb(std::string& desc, const std::string& code);
protected:

  /** \brief Converts a language code given as a long, see #MAKECODE(a, b, c, d)
  *          to its string representation.
  *   \param[in] code The language code given as a long, see #MAKECODE(a, b, c, d).
  *   \param[out] ret The string representation of the given language code code.
  */ 
  void CodeToString(long code, CStdString& ret);

  typedef std::map<CStdString, CStdString> STRINGLOOKUPTABLE;
  STRINGLOOKUPTABLE m_mapUser;

  bool LookupInMap(CStdString& desc, const CStdString& code);

  /** \brief Looks up the ISO 639-1, ISO 639-2/T, or ISO 639-2/B, whichever it finds first,
  *          code of the given english language name.
  *   \param[in] desc The english language name for which a code is looked for.
  *   \param[out] code The ISO 639-1, ISO 639-2/T, or ISO 639-2/B code of the given language desc.
  *   \return true if the a code was found, false otherwise.
  */ 
  bool ReverseLookup(const CStdString& desc, CStdString& code);
};

extern CLangCodeExpander g_LangCodeExpander;
