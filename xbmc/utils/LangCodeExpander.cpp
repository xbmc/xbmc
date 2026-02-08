/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "LangCodeExpander.h"

#include "LangInfo.h"
#include "ServiceBroker.h"
#include "utils/RegExp.h"
#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML.h"
#include "utils/i18n/Bcp47.h"
#include "utils/i18n/Bcp47Registry/SubTagRegistryManager.h"
#include "utils/i18n/Iso3166_1.h"
#include "utils/i18n/Iso639.h"
#include "utils/i18n/Iso639_1.h"
#include "utils/i18n/Iso639_2.h"
#include "utils/i18n/TableLanguageCodes.h"
#include "utils/log.h"

#include <algorithm>
#include <cassert>

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

  if (auto tag = CBcp47::ParseTag(code); tag.has_value())
  {
    desc = tag.value().Format(Bcp47FormattingStyle::FORMAT_ENGLISH);
    return true;
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

bool CLangCodeExpander::ConvertISO6392ToISO6391(std::string iso6392, std::string& iso6391)
{
  StringUtils::Trim(iso6392);
  StringUtils::ToLower(iso6392);

  return ConvertISO6392ToISO6391Internal(iso6392, iso6391);
}

bool CLangCodeExpander::ConvertISO6392ToISO6391Internal(const std::string& iso6392,
                                                        std::string& iso6391)
{
  if (iso6392.length() != 3)
    return false;

  assert(
      std::ranges::none_of(iso6392, [](char c) { return StringUtils::isasciiuppercaseletter(c); }));
  assert(!std::isspace(iso6392.front()) && !std::isspace(iso6392.back()));

  const std::string bCode = CIso639_2::TCodeToBCode(iso6392).value_or(iso6392);

  const auto it = std::ranges::lower_bound(LanguageCodesByIso639_2b, bCode, {}, &ISO639::iso639_2b);
  if (it != LanguageCodesByIso639_2b.end() && it->iso639_2b == bCode)
  {
    iso6391 = it->iso639_1;
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

    if (std::ranges::binary_search(LanguageCodesByIso639_2b, charCode, {}, &ISO639::iso639_2b))
    {
      strISO6392B = charCode;
      return true;
    }

    if (const auto bCode{CIso639_2::TCodeToBCode(charCode)}; bCode.has_value())
    {
      strISO6392B = bCode.value();
      return true;
    }

    if (checkWin32Locales)
    {
      const auto it =
          std::ranges::lower_bound(LanguageCodesByWin_Id, charCode, {}, &ISO639::win_id);
      if (it != LanguageCodesByWin_Id.end() && it->win_id == charCode)
      {
        strISO6392B = it->iso639_2b;
        return true;
      }
    }

    // Match against country last to avoid stealing possible matches from previous conditions
    //! @todo what's this legacy logic for?
    if (CIso3166_1::ContainsAlpha3(charCode))
    {
      strISO6392B = charCode;
      return true;
    }
  }
  else if (strCharCode.size() > 3)
  {
    if (const auto tCode = CIso639_2::LookupByName(strCharCode); tCode.has_value())
    {
      // Map T to B code for the few languages that have differences
      const auto bCode{CIso639_2::TCodeToBCode(*tCode)};
      strISO6392B = bCode.value_or(*tCode);
      return true;
    }

    // Try search on language addons
    if (const std::string addonLang = g_langInfo.ConvertEnglishNameToAddonLocale(strCharCode);
        !addonLang.empty())
    {
      strISO6392B = addonLang;
      return true;
    }
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

  auto ret = CIso3166_1::Alpha2ToAlpha3(lower);
  if (ret)
  {
    strISO31661Alpha3 = *ret;
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
      auto ret = CIso3166_1::Alpha3ToAlpha2(lower);
      if (ret)
      {
        code = *ret;
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

  if (const auto ret = CIso639_1::LookupByName(descTmp); ret.has_value())
  {
    code = *ret;
    return true;
  }

  if (const auto ret = CIso639_2::LookupByName(descTmp); ret.has_value())
  {
    code = *ret;
    return true;
  }

  const CSubTagRegistryManager& registry{CServiceBroker::GetSubTagRegistry()};
  if (const auto ret = registry.GetLanguageSubTags().LookupByDescription(descTmp); ret.has_value())
  {
    code = ret->m_subTag;
    return true;
  }

  // Find on language addons
  if (const std::string addonLang = g_langInfo.ConvertEnglishNameToAddonLocale(descTmp);
      !addonLang.empty())
  {
    code = addonLang;
    return true;
  }
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

  if (sCode.length() == 2)
  {
    auto ret = CIso639_1::LookupByCode(StringToLongCode(sCode));
    if (ret)
    {
      desc = *ret;
      return true;
    }
  }
  else if (sCode.length() == 3)
  {
    uint32_t longCode = StringToLongCode(sCode);

    // Map B to T for the few codes that have differences
    auto tCode = CIso639_2::BCodeToTCode(longCode);
    if (tCode.has_value())
      longCode = *tCode;

    // Lookup the T code
    auto ret = CIso639_2::LookupByCode(longCode);
    if (ret)
    {
      desc = *ret;
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
    CIso639_2::ListLanguages(langMap);
  else
    CIso639_1::ListLanguages(langMap);

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

bool CLangCodeExpander::ConvertToBcp47(const std::string& text, std::string& bcp47Lang)
{
  std::string code{text};
  StringUtils::Trim(code);
  StringUtils::ToLower(code);

  // Search in the user defined map. Bypasses all other validations.
  if (LookupUserCode(code, bcp47Lang))
    return true;

  if (auto tag = CBcp47::ParseTag(code); tag.has_value())
  {
    if (tag->IsValid())
    {
      tag->Canonicalize();
      bcp47Lang = tag->Format();
      return true;
    }
    else if (ConvertISO6392ToISO6391Internal(code, bcp47Lang))
    {
      // Alpha-2 codes are likely to be registered but there is no guarantee.
      tag = CBcp47::ParseTag(bcp47Lang);
      if (tag.has_value() && tag->IsValid())
      {
        bcp47Lang = tag->Format();

        return true;
      }
    }
  }

  // Search in the language addons
  if (std::string dummy; LookupInLangAddons(code, dummy))
  {
    bcp47Lang = code;
    return true;
  }

  // Not formatted as BCP 47 / unknown code, this could be an English language name
  if (ReverseLookup(code, bcp47Lang))
  {
    // Could be an alpha-3 ISO639 code but BCP47 requires the alpha-2
    ConvertISO6392ToISO6391(bcp47Lang, bcp47Lang);
    return true;
  }

  return false;
}

std::string CLangCodeExpander::FindLanguageCodeWithSubtag(const std::string& str)
{
  std::size_t begin = str.find("{");

  if (begin == std::string::npos)
    return "";

  std::size_t end = str.find("}", begin + 1);

  if (end == std::string::npos)
    return "";

  const auto tag = CBcp47::ParseTag(str.substr(begin + 1, end - begin - 1));

  if (tag.has_value())
    return tag->Format();

  return "";
}
