/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "LangCodeExpander.h"

#include "LangInfo.h"
#include "utils/RegExp.h"
#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML.h"
#include "utils/i18n/Iso639_1.h"
#include "utils/i18n/TableISO3166_1.h"
#include "utils/i18n/TableISO639_2.h"
#include "utils/i18n/TableLanguageCodes.h"
#include "utils/log.h"

#include <algorithm>
#include <string_view>

using namespace KODI::UTILS::I18N;

CLangCodeExpander::CLangCodeExpander() = default;

CLangCodeExpander::~CLangCodeExpander() = default;

void CLangCodeExpander::Clear()
{
  m_mapUser.clear();
}

void CLangCodeExpander::LoadUserCodes(const TiXmlElement* pRootElement)
{
  if (pRootElement != NULL)
  {
    m_mapUser.clear();

    std::string sShort, sLong;

    const TiXmlNode* pLangCode = pRootElement->FirstChild("code");
    while (pLangCode != NULL)
    {
      const TiXmlNode* pShort = pLangCode->FirstChildElement("short");
      const TiXmlNode* pLong = pLangCode->FirstChildElement("long");
      if (pShort != NULL && pLong != NULL)
      {
        sShort = pShort->FirstChild()->Value();
        sLong = pLong->FirstChild()->Value();
        StringUtils::ToLower(sShort);

        m_mapUser[sShort] = sLong;
      }

      pLangCode = pLangCode->NextSibling();
    }
  }
}

bool CLangCodeExpander::Lookup(const std::string& code, std::string& desc)
{
  if (LookupInUserMap(code, desc))
    return true;

  if (LookupInISO639Tables(code, desc))
    return true;

  if (LookupInLangAddons(code, desc))
    return true;

  // Language code with subtag is supported only with language addons
  // or with user defined map, then if not found we fallback by obtaining
  // the primary code description only and appending the remaining
  int iSplit = code.find('-');
  if (iSplit > 0)
  {
    std::string primaryTagDesc;
    const bool hasPrimaryTagDesc = Lookup(code.substr(0, iSplit), primaryTagDesc);
    std::string subtagCode = code.substr(iSplit + 1);
    if (hasPrimaryTagDesc)
    {
      if (!primaryTagDesc.empty())
        desc = primaryTagDesc;
      else
        desc = code.substr(0, iSplit);

      if (!subtagCode.empty())
        desc += " - " + subtagCode;

      return true;
    }
  }

  return false;
}

bool CLangCodeExpander::Lookup(const int code, std::string& desc)
{
  char lang[3];
  lang[2] = 0;
  lang[1] = (code & 0xFF);
  lang[0] = (code >> 8) & 0xFF;

  return Lookup(lang, desc);
}

bool CLangCodeExpander::ConvertISO6391ToISO6392B(const std::string& strISO6391,
                                                 std::string& strISO6392B,
                                                 bool checkWin32Locales /*= false*/)
{
  // not a 2 char code
  if (strISO6391.length() != 2)
    return false;

  std::string strISO6391Lower(strISO6391);
  StringUtils::ToLower(strISO6391Lower);
  StringUtils::Trim(strISO6391Lower);

  auto it = std::ranges::lower_bound(LanguageCodes, strISO6391Lower, {}, &ISO639::iso639_1);
  if (it != LanguageCodes.end() && it->iso639_1 == strISO6391Lower)
  {
    if (checkWin32Locales && !it->win_id.empty())
      strISO6392B = it->win_id;
    else
      strISO6392B = it->iso639_2b;
    return true;
  }
  return false;
}

bool CLangCodeExpander::ConvertToISO6392B(const std::string& strCharCode,
                                          std::string& strISO6392B,
                                          bool checkWin32Locales /* = false */)
{

  //first search in the user defined map
  if (LookupUserCode(strCharCode, strISO6392B))
    return true;

  if (strCharCode.size() == 2)
    return g_LangCodeExpander.ConvertISO6391ToISO6392B(strCharCode, strISO6392B, checkWin32Locales);

  if (strCharCode.size() == 3)
  {
    std::string charCode(strCharCode);
    StringUtils::ToLower(charCode);

    if (std::ranges::binary_search(LanguageCodesByIso639_2b, charCode, {}, &ISO639::iso639_2b) ||
        (checkWin32Locales &&
         std::ranges::binary_search(LanguageCodesByWin_Id, charCode, {}, &ISO639::win_id)) ||
        std::ranges::binary_search(TableISO3166_1ByAlpha3, charCode, {}, &ISO3166_1::alpha3))
    {
      strISO6392B = charCode;
      return true;
    }
  }
  else if (strCharCode.size() > 3)
  {
    std::string_view name = strCharCode;

    auto itt = std::ranges::lower_bound(
        TableISO639_2AllNames, name, [](std::string_view a, std::string_view b)
        { return StringUtils::CompareNoCase(a, b) < 0; }, &LCENTRY::name);
    if (itt != TableISO639_2AllNames.end() && StringUtils::EqualsNoCase(itt->name, name))
    {
      // Map T to B code for the few languages that have differences
      auto itb = std::ranges::lower_bound(ISO639_2_TB_Mappings, itt->code, {},
                                          &ISO639_2_TB::terminological);
      if (itb != ISO639_2_TB_Mappings.end() && itb->terminological == itt->code)
        strISO6392B = LongCodeToString(itb->bibliographic);
      else
        strISO6392B = LongCodeToString(itt->code);

      return true;
    }

    // Try search on language addons
    strISO6392B = g_langInfo.ConvertEnglishNameToAddonLocale(strCharCode);
    if (!strISO6392B.empty())
      return true;
  }
  return false;
}

bool CLangCodeExpander::ConvertToISO6392T(const std::string& strCharCode,
                                          std::string& strISO6392T,
                                          bool checkWin32Locales /* = false */)
{
  std::string strISO6392B;
  if (!ConvertToISO6392B(strCharCode, strISO6392B, checkWin32Locales))
    return false;

  {
    auto it =
        std::ranges::lower_bound(LanguageCodesByIso639_2b, strISO6392B, {}, &ISO639::iso639_2b);
    if (it != LanguageCodesByIso639_2b.end() && it->iso639_2b == strISO6392B &&
        !it->iso639_2t.empty())
    {
      strISO6392T = it->iso639_2t;
      return true;
    }
  }

  if (checkWin32Locales)
  {
    auto it = std::ranges::lower_bound(LanguageCodesByWin_Id, strISO6392B, {}, &ISO639::win_id);
    if (it != LanguageCodesByWin_Id.end() && it->win_id == strISO6392B && !it->iso639_2t.empty())
    {
      strISO6392T = it->iso639_2t;
      return true;
    }
  }
  return false;
}


bool CLangCodeExpander::LookupUserCode(const std::string& desc, std::string& userCode)
{
  for (STRINGLOOKUPTABLE::const_iterator it = m_mapUser.begin(); it != m_mapUser.end(); ++it)
  {
    if (StringUtils::EqualsNoCase(desc, it->first) || StringUtils::EqualsNoCase(desc, it->second))
    {
      userCode = it->first;
      return true;
    }
  }
  return false;
}

#ifdef TARGET_WINDOWS
bool CLangCodeExpander::ConvertISO31661Alpha2ToISO31661Alpha3(const std::string& strISO31661Alpha2,
                                                              std::string& strISO31661Alpha3)
{
  if (strISO31661Alpha2.length() != 2)
    return false;

  std::string lower(strISO31661Alpha2);
  StringUtils::ToLower(lower);
  StringUtils::Trim(lower);

  auto it = std::ranges::lower_bound(TableISO3166_1, lower, {}, &ISO3166_1::alpha2);
  if (it != TableISO3166_1.end() && it->alpha2 == lower)
  {
    strISO31661Alpha3 = it->alpha3;
    return true;
  }
  return true;
}

bool CLangCodeExpander::ConvertWindowsLanguageCodeToISO6392B(
    const std::string& strWindowsLanguageCode, std::string& strISO6392B)
{
  if (strWindowsLanguageCode.length() != 3)
    return false;

  std::string lower(strWindowsLanguageCode);
  StringUtils::ToLower(lower);

  auto it = std::ranges::lower_bound(LanguageCodesByWin_Id, lower, {}, &ISO639::win_id);
  if (it != LanguageCodesByWin_Id.end() && it->win_id == lower)
  {
    strISO6392B = it->iso639_2b;
    return true;
  }

  if (std::ranges::binary_search(LanguageCodesByIso639_2b, lower, {}, &ISO639::iso639_2b))
  {
    strISO6392B = lower;
    return true;
  }
  return false;
}
#endif

bool CLangCodeExpander::ConvertToISO6391(const std::string& lang, std::string& code)
{
  if (lang.empty())
    return false;

  //first search in the user defined map
  if (LookupUserCode(lang, code))
    return true;

  if (lang.length() == 2)
  {
    std::string tmp;
    if (Lookup(lang, tmp))
    {
      code = lang;
      return true;
    }
  }
  else if (lang.length() == 3)
  {
    std::string lower(lang);
    StringUtils::ToLower(lower);

    {
      auto it = std::ranges::lower_bound(LanguageCodesByIso639_2b, lower, {}, &ISO639::iso639_2b);
      if (it != LanguageCodesByIso639_2b.end() && it->iso639_2b == lower)
      {
        code = it->iso639_1;
        return true;
      }
    }
    {
      auto it = std::ranges::lower_bound(LanguageCodesByWin_Id, lower, {}, &ISO639::win_id);
      if (it != LanguageCodesByWin_Id.end() && it->win_id == lower)
      {
        code = it->iso639_1;
        return true;
      }
    }
    {
      auto it = std::ranges::lower_bound(TableISO3166_1ByAlpha3, lower, {}, &ISO3166_1::alpha3);
      if (it != TableISO3166_1ByAlpha3.end() && it->alpha3 == lower)
      {
        code = it->alpha2;
        return true;
      }
    }
  }

  // check if lang is full language name
  std::string tmp;
  if (ReverseLookup(lang, tmp))
  {
    if (tmp.length() == 2)
    {
      code = tmp;
      return true;
    }

    if (tmp.length() == 3)
    {
      // there's only an iso639-2 code that is identical to the language name, e.g. Yao
      if (StringUtils::EqualsNoCase(tmp, lang))
        return false;

      return ConvertToISO6391(tmp, code);
    }
  }

  return false;
}

bool CLangCodeExpander::ReverseLookup(const std::string& desc, std::string& code)
{
  if (desc.empty())
    return false;

  std::string descTmp(desc);
  StringUtils::Trim(descTmp);

  // First find to user-defined languages
  for (STRINGLOOKUPTABLE::const_iterator it = m_mapUser.begin(); it != m_mapUser.end(); ++it)
  {
    if (StringUtils::EqualsNoCase(descTmp, it->second))
    {
      code = it->first;
      return true;
    }
  }

  std::string_view name = descTmp;
  {
    auto ret = CIso639_1::LookupByName(name);
    if (ret)
    {
      code = *ret;
      return true;
    }
  }
  {
    auto it = std::ranges::lower_bound(
        TableISO639_2AllNames, name, [](std::string_view a, std::string_view b)
        { return StringUtils::CompareNoCase(a, b) < 0; }, &LCENTRY::name);
    if (it != TableISO639_2AllNames.end() && StringUtils::EqualsNoCase(it->name, name))
    {
      code = LongCodeToString(it->code);
      return true;
    }
  }

  // Find on language addons
  code = g_langInfo.ConvertEnglishNameToAddonLocale(descTmp);
  if (!code.empty())
    return true;

  return false;
}

bool CLangCodeExpander::LookupInUserMap(const std::string& code, std::string& desc)
{
  if (code.empty())
    return false;

  // make sure we convert to lowercase before trying to find it
  std::string sCode(code);
  StringUtils::ToLower(sCode);
  StringUtils::Trim(sCode);

  STRINGLOOKUPTABLE::iterator it = m_mapUser.find(sCode);
  if (it != m_mapUser.end())
  {
    desc = it->second;
    return true;
  }

  return false;
}

bool CLangCodeExpander::LookupInLangAddons(const std::string& code, std::string& desc)
{
  if (code.empty())
    return false;

  std::string sCode{code};
  StringUtils::Trim(sCode);
  StringUtils::ToLower(sCode);
  StringUtils::Replace(sCode, '-', '_');

  desc = g_langInfo.GetEnglishLanguageName(sCode);
  return !desc.empty();
}

bool CLangCodeExpander::LookupInISO639Tables(const std::string& code, std::string& desc)
{
  if (code.empty())
    return false;

  std::string sCode(code);
  StringUtils::ToLower(sCode);
  StringUtils::Trim(sCode);
  uint32_t longCode = StringToLongCode(sCode);

  if (sCode.length() == 2)
  {
    auto ret = CIso639_1::LookupByCode(longCode);
    if (ret)
    {
      desc = *ret;
      return true;
    }
  }
  else if (sCode.length() == 3)
  {
    // Map B to T for the few codes that have differences
    uint32_t longTcode = longCode;

    auto itt = std::ranges::lower_bound(ISO639_2_TB_MappingsByB, longCode, {},
                                        &ISO639_2_TB::bibliographic);
    if (itt != ISO639_2_TB_MappingsByB.end() && longCode == itt->bibliographic)
      longTcode = itt->terminological;

    // Lookup the T code
    auto it = std::ranges::lower_bound(TableISO639_2ByCode, longTcode, {}, &LCENTRY::code);
    if (it != TableISO639_2ByCode.end() && longTcode == it->code)
    {
      desc = it->name;
      return true;
    }
  }
  return false;
}

bool CLangCodeExpander::CompareFullLanguageNames(const std::string& lang1, const std::string& lang2)
{
  if (StringUtils::EqualsNoCase(lang1, lang2))
    return true;

  std::string expandedLang1, expandedLang2, code1, code2;

  if (!ReverseLookup(lang1, code1))
    return false;

  code1 = lang1;
  if (!ReverseLookup(lang2, code2))
    return false;

  code2 = lang2;
  Lookup(expandedLang1, code1);
  Lookup(expandedLang2, code2);

  return StringUtils::EqualsNoCase(expandedLang1, expandedLang2);
}

std::vector<std::string> CLangCodeExpander::GetLanguageNames(
    LANGFORMATS format /* = CLangCodeExpander::ISO_639_1 */,
    LANG_LIST list /* = LANG_LIST::DEFAULT */)
{
  std::map<std::string, std::string> langMap;

  if (format == CLangCodeExpander::ISO_639_2)
  {
    // ISO 639-2/T codes
    std::ranges::transform(TableISO639_2AllNames, std::inserter(langMap, langMap.end()),
                           [](const LCENTRY& e) {
                             return std::make_pair(LongCodeToString(e.code), std::string{e.name});
                           });

    // ISO 639-2/B codes that are different from the T code.
    for (const ISO639_2_TB& tb : ISO639_2_TB_Mappings)
    {
      const std::string bCode = LongCodeToString(tb.bibliographic);

      // Lookup the 639-2/T code
      //! @todo Maybe could be avoided by building a constexpr 639-2/B to name array at compile time.
      //! Is it worth the effort and memory though.
      auto it =
          std::ranges::lower_bound(TableISO639_2ByCode, tb.terminological, {}, &LCENTRY::code);
      if (it != TableISO639_2ByCode.end() && tb.terminological == it->code)
      {
        langMap[bCode] = std::string(it->name) + " (non-preferred)";
      }
      else
      {
        CLog::LogF(LOGERROR,
                   "unable to find a name for the ISO 639-2/B code {} using 639-2/T code {}", bCode,
                   LongCodeToString(tb.terminological));
      }
    }
  }
  else
  {
    CIso639_1::ListLanguages(langMap);
  }

  if (list == LANG_LIST::INCLUDE_ADDONS || list == LANG_LIST::INCLUDE_ADDONS_USERDEFINED)
  {
    g_langInfo.GetAddonsLanguageCodes(langMap);
  }

  // User-defined languages can override existing ones
  if (list == LANG_LIST::INCLUDE_USERDEFINED || list == LANG_LIST::INCLUDE_ADDONS_USERDEFINED)
  {
    for (const auto& value : m_mapUser)
    {
      langMap[value.first] = value.second;
    }
  }

  // Sort by name and remove duplicates
  std::set<std::string, sortstringbyname> languages;
  for (const auto& lang : langMap)
  {
    languages.insert(lang.second);
  }

  return std::vector<std::string>(languages.begin(), languages.end());
}

bool CLangCodeExpander::CompareISO639Codes(const std::string& code1, const std::string& code2)
{
  if (StringUtils::EqualsNoCase(code1, code2))
    return true;

  std::string expandedLang1;
  if (!Lookup(code1, expandedLang1))
    return false;

  std::string expandedLang2;
  if (!Lookup(code2, expandedLang2))
    return false;

  return StringUtils::EqualsNoCase(expandedLang1, expandedLang2);
}

std::string CLangCodeExpander::ConvertToISO6392B(const std::string& lang)
{
  if (lang.empty())
    return lang;

  std::string two, three;
  if (ConvertToISO6391(lang, two))
  {
    if (ConvertToISO6392B(two, three))
      return three;
  }

  return lang;
}

std::string CLangCodeExpander::ConvertToISO6392T(const std::string& lang)
{
  if (lang.empty())
    return lang;

  std::string two, three;
  if (ConvertToISO6391(lang, two))
  {
    if (ConvertToISO6392T(two, three))
      return three;
  }

  return lang;
}

std::string CLangCodeExpander::FindLanguageCodeWithSubtag(const std::string& str)
{
  CRegExp regLangCode;
  if (regLangCode.RegComp("\\{(([A-Za-z]{2,3})-([A-Za-z]{2}|[0-9]{3}|[A-Za-z]{4}))\\}") &&
      regLangCode.RegFind(str) >= 0)
  {
    return regLangCode.GetMatch(1);
  }
  return "";
}
