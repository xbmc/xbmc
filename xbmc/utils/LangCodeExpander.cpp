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
#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML.h"
#include "utils/i18n/Bcp47.h"
#include "utils/i18n/Bcp47Registry/SubTagRegistryManager.h"
#include "utils/i18n/Iso639.h"
#include "utils/i18n/Iso639_1.h"
#include "utils/i18n/Iso639_2.h"
#include "utils/i18n/TableLanguageCodes.h"
#include "utils/log.h"

#include <algorithm>
#include <array>
#include <optional>
#include <string_view>

using namespace KODI::UTILS::I18N;

constexpr std::size_t MAX_BCP47_ENGLISH_NAME_LENGTH = 30;

namespace
{
//! Kodi's own codes for Brazilian Portuguese. No standard assigns them, so BCP 47 has no subtag
//! for them and only an addon can say what they mean.
constexpr std::array<std::string_view, 2> UNREGISTERED_CODES{"pb", "pob"};

/*!
 * \brief The BCP 47 language subtag for a bare ISO 639 code, taken from the ISO 639 tables.
 *
 * RFC 5646 draws its primary language subtags from ISO 639, and a bare code carries no script,
 * region or variant for the subtag registry to arbitrate, so the static tables settle it.
 *
 * \param[in] code A trimmed, lower-cased ISO 639-1 or ISO 639-2 code.
 * \return The subtag, or nullopt where the tables do not hold the language, in which case the
 *         registry is the only thing that can recognize it.
 */
std::optional<std::string> Bcp47SubTagFromIso639(const std::string& code)
{
  if (std::ranges::find(UNREGISTERED_CODES, code) != UNREGISTERED_CODES.end())
    return std::nullopt;

  // BCP 47 registers the alpha-2 code for every language that has one, so an alpha-2 is already
  // the subtag - except for the five ISO 639-1 withdrew, whose current spelling is reached by
  // going out to the alpha-3 the language kept and back
  if (code.length() == 2)
  {
    if (std::string alpha3; CLangCodeExpander::ConvertISO6391ToISO6392B(code, alpha3))
    {
      if (std::string alpha2; CLangCodeExpander::ConvertISO6392ToISO6391(alpha3, alpha2))
        return alpha2;
    }

    return std::nullopt;
  }

  if (code.length() != 3)
    return std::nullopt;

  if (std::string alpha2; CLangCodeExpander::ConvertISO6392ToISO6391(code, alpha2))
    return alpha2;

  // A language with no alpha-2 is registered under its ISO 639-2/T code. Only languages that do
  // have an alpha-2 spell their bibliographic and terminology codes apart, so a code reaching
  // here is already the /T form
  if (CIso639_2::LookupByCode(code).has_value())
    return code;

  return std::nullopt;
}
} // namespace

CLangCodeExpander::STRINGLOOKUPTABLE& CLangCodeExpander::GetUserCodes()
{
  static STRINGLOOKUPTABLE userCodes;
  return userCodes;
}

void CLangCodeExpander::Clear()
{
  GetUserCodes().clear();
}

void CLangCodeExpander::LoadUserCodes(const TiXmlElement* pRootElement)
{
  if (pRootElement != nullptr)
  {
    GetUserCodes().clear();

    std::string sShort, sLong;

    const TiXmlNode* pLangCode = pRootElement->FirstChild("code");
    while (pLangCode != nullptr)
    {
      const TiXmlNode* pShort = pLangCode->FirstChildElement("short");
      const TiXmlNode* pLong = pLangCode->FirstChildElement("long");
      if (pShort != nullptr && pShort->FirstChild() != nullptr && pLong != nullptr &&
          pLong->FirstChild() != nullptr)
      {
        sShort = pShort->FirstChild()->Value();
        sLong = pLong->FirstChild()->Value();
        StringUtils::ToLower(sShort);

        GetUserCodes()[sShort] = sLong;
      }

      pLangCode = pLangCode->NextSibling();
    }
  }
}

bool CLangCodeExpander::Lookup(const std::string& code, std::string& desc)
{
  static_assert(MAX_BCP47_ENGLISH_NAME_LENGTH > 3);

  if (LookupInUserMap(code, desc))
    return true;

  if (LookupInISO639Tables(code, desc))
    return true;

  if (const auto addonName = LookupInLangAddons(code); addonName.has_value())
  {
    desc = *addonName;
    return true;
  }

  if (auto tag = CBcp47::ParseTag(code); tag.has_value())
  {
    desc = tag.value().Format(Bcp47FormattingStyle::FORMAT_ENGLISH);
    if (desc.size() > MAX_BCP47_ENGLISH_NAME_LENGTH)
    {
      desc.resize(MAX_BCP47_ENGLISH_NAME_LENGTH - 3);
      desc.append("... [");
      desc.append(tag.value().Format(Bcp47FormattingStyle::FORMAT_BCP47));
      desc.push_back(']');
    }
    return true;
  }

  return false;
}

bool CLangCodeExpander::ConvertISO6391ToISO6392B(const std::string& strISO6391,
                                                 std::string& strISO6392B)
{
  // not a 2 char code
  if (strISO6391.length() != 2)
    return false;

  std::string strISO6391Lower(strISO6391);
  StringUtils::ToLower(strISO6391Lower);
  StringUtils::Trim(strISO6391Lower);

  const auto it = std::ranges::lower_bound(LanguageCodes, strISO6391Lower, {}, &ISO639::iso639_1);
  if (it != LanguageCodes.end() && it->iso639_1 == strISO6391Lower)
  {
    strISO6392B = it->iso639_2b;
    return true;
  }

  const auto deprecated =
      std::ranges::lower_bound(DeprecatedLanguageCodes, strISO6391Lower, {}, &ISO639::iso639_1);
  if (deprecated != DeprecatedLanguageCodes.end() && deprecated->iso639_1 == strISO6391Lower)
  {
    strISO6392B = deprecated->iso639_2b;
    return true;
  }
  return false;
}

bool CLangCodeExpander::ConvertISO6392ToISO6391(std::string iso6392, std::string& iso6391)
{
  StringUtils::Trim(iso6392);
  StringUtils::ToLower(iso6392);

  if (iso6392.length() != 3)
    return false;

  const std::string bCode = CIso639_2::TCodeToBCode(iso6392).value_or(iso6392);

  const auto it = std::ranges::lower_bound(LanguageCodesByIso639_2b, bCode, {}, &ISO639::iso639_2b);
  if (it != LanguageCodesByIso639_2b.end() && it->iso639_2b == bCode && !it->iso639_1.empty())
  {
    iso6391 = it->iso639_1;
    return true;
  }
  return false;
}

bool CLangCodeExpander::ConvertToISO6392B(const std::string& strCharCode, std::string& strISO6392B)
{
  // A user defined mapping bypasses all other validations, so it is matched as it was given
  if (LookupUserCode(strCharCode, strISO6392B))
    return true;

  // Nothing below is case sensitive, and the length is what tells the notations apart: two
  // characters is an ISO 639-1 code, three an ISO 639-2 one, and anything longer a name
  std::string code{strCharCode};
  StringUtils::ToLower(code);

  if (code.size() == 2)
    return ConvertISO6391ToISO6392B(code, strISO6392B);

  if (code.size() == 3)
  {
    if (std::ranges::binary_search(LanguageCodesByIso639_2b, code, {}, &ISO639::iso639_2b))
    {
      strISO6392B = code;
      return true;
    }

    if (const auto bCode{CIso639_2::TCodeToBCode(code)}; bCode.has_value())
    {
      strISO6392B = *bCode;
      return true;
    }

    // The table searched above holds only the languages that also have an ISO 639-1 code. A code
    // that is itself an ISO 639-2 code is already the wanted one - had a differing B form existed,
    // the conversion above would have found it.
    if (CIso639_2::LookupByCode(code).has_value())
    {
      strISO6392B = code;
      return true;
    }

    return false;
  }

  if (code.size() > 3)
  {
    if (const auto tCode = CIso639_2::LookupByName(code); tCode.has_value())
    {
      // Map T to B code for the few languages that have differences
      strISO6392B = CIso639_2::TCodeToBCode(*tCode).value_or(*tCode);
      return true;
    }

    // Try search on language addons
    if (const std::string addonLang = g_langInfo.ConvertEnglishNameToAddonLocale(code);
        !addonLang.empty())
    {
      strISO6392B = addonLang;
      return true;
    }
  }

  return false;
}

bool CLangCodeExpander::LookupUserCode(const std::string& desc, std::string& userCode)
{
  const auto it = std::ranges::find_if(GetUserCodes(),
                                       [&desc](const auto& it)
                                       {
                                         return StringUtils::EqualsNoCase(desc, it.first) ||
                                                StringUtils::EqualsNoCase(desc, it.second);
                                       });
  if (it != GetUserCodes().end())
  {
    userCode = it->first;
    return true;
  }
  return false;
}

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
    // Handles the B and T forms alike
    if (ConvertISO6392ToISO6391(lang, code))
      return true;
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

bool CLangCodeExpander::ReverseLookupInTables(const std::string& desc, std::string& code)
{
  if (desc.empty())
    return false;

  std::string descTmp(desc);
  StringUtils::Trim(descTmp);

  // First find to user-defined languages
  const auto it = std::ranges::find_if(GetUserCodes(), [&descTmp](const auto& it)
                                       { return StringUtils::EqualsNoCase(descTmp, it.second); });
  if (it != GetUserCodes().end())
  {
    code = it->first;
    return true;
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

  return false;
}

bool CLangCodeExpander::ReverseLookup(const std::string& desc, std::string& code)
{
  if (desc.empty())
    return false;

  if (ReverseLookupInTables(desc, code))
    return true;

  std::string descTmp(desc);
  StringUtils::Trim(descTmp);

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

  const auto it = GetUserCodes().find(sCode);
  if (it != GetUserCodes().end())
  {
    desc = it->second;
    return true;
  }

  return false;
}

std::optional<std::string> CLangCodeExpander::LookupInLangAddons(const std::string& code)
{
  if (code.empty())
    return std::nullopt;

  std::string sCode{code};
  StringUtils::Trim(sCode);
  StringUtils::ToLower(sCode);
  StringUtils::Replace(sCode, '-', '_');

  std::string desc{g_langInfo.GetEnglishLanguageName(sCode)};
  if (desc.empty())
    return std::nullopt;

  return desc;
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
    const auto ret = CIso639_1::LookupByCode(StringToLongCode(sCode));
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
    const auto tCode = CIso639_2::BCodeToTCode(longCode);
    if (tCode.has_value())
      longCode = *tCode;

    // Lookup the T code
    const auto ret = CIso639_2::LookupByCode(longCode);
    if (ret)
    {
      desc = *ret;
      return true;
    }
  }
  return false;
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
    std::ranges::copy(GetUserCodes(), std::inserter(langMap, langMap.end()));
  }

  // Sort by name and remove duplicates
  std::set<std::string, sortstringbyname> languages;
  std::ranges::transform(langMap, std::inserter(languages, languages.end()),
                         [](const auto& lang) { return lang.second; });

  return {languages.begin(), languages.end()};
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

std::string CLangCodeExpander::AsISO6392B(const std::string& lang)
{
  if (lang.empty())
    return lang;

  // A user defined mapping bypasses all other validations, so it is matched on the whole value
  if (std::string userCode; LookupUserCode(lang, userCode))
    return userCode;

  // A bare code is already its own primary language subtag, so a tag parse has nothing to strip
  if (lang.length() == 2 || lang.length() == 3)
  {
    if (std::string code; ConvertToISO6392B(lang, code))
      return code;
  }

  // Region, script and variant subtags have no ISO 639 equivalent, so a tag is represented by its
  // primary language subtag. Anything a tag cannot be made of - an English name, most obviously -
  // is converted as it stands.
  const auto tag = CBcp47::ParseTag(lang);
  const std::string& language = tag.has_value() && tag->IsValid() ? tag->GetLanguage() : lang;

  if (std::string code; ConvertToISO6392B(language, code))
    return code;

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

  if (const auto subTag = Bcp47SubTagFromIso639(code); subTag.has_value())
  {
    bcp47Lang = *subTag;
    return true;
  }

  // Tried after the code, so that a name spelled like another language's code - "Ga" names gaa,
  // but ga is Irish - resolves as the code
  if (std::string iso639; ReverseLookupInTables(code, iso639))
  {
    if (const auto subTag = Bcp47SubTagFromIso639(iso639); subTag.has_value())
    {
      bcp47Lang = *subTag;
      return true;
    }
  }

  auto tag = CBcp47::ParseTag(code);

  // A valid tag is already the answer, in canonical form
  if (tag.has_value() && tag->IsValid())
  {
    tag->Canonicalize();
    bcp47Lang = tag->Format();
    return true;
  }

  // Well formed but not registered is how an ISO 639-2/B code parses. Its alpha-2 sibling is
  // what BCP 47 registers where the language has one.
  if (tag.has_value())
  {
    if (std::string alpha2; ConvertISO6392ToISO6391(code, alpha2))
    {
      // Alpha-2 codes are likely to be registered but there is no guarantee.
      if (const auto alpha2Tag = CBcp47::ParseTag(alpha2);
          alpha2Tag.has_value() && alpha2Tag->IsValid())
      {
        bcp47Lang = alpha2Tag->Format();
        return true;
      }

      // Kodi's table and the subtag registry disagree, which is a data bug rather than bad input
      CLog::LogF(LOGERROR,
                 "'{}' has the ISO 639-1 code '{}', which is not a registered BCP 47 language "
                 "subtag",
                 code, alpha2);
    }
  }

  // A code a language addon answers for is served as it stands
  if (LookupInLangAddons(code).has_value())
  {
    bcp47Lang = code;
    return true;
  }

  // Not formatted as BCP 47 / unknown code, this could be an English language name
  if (std::string iso639; ReverseLookup(code, iso639))
  {
    // BCP 47 uses the alpha-2 code where the language has one, and the alpha-3 otherwise
    if (std::string alpha2; ConvertISO6392ToISO6391(iso639, alpha2))
      bcp47Lang = alpha2;
    else
      bcp47Lang = iso639;

    return true;
  }

  return false;
}

std::string CLangCodeExpander::AsBcp47(const std::string& lang)
{
  if (lang.empty())
    return lang;

  if (std::string bcp47; ConvertToBcp47(lang, bcp47))
    return bcp47;

  return lang;
}
