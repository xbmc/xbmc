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

#include "LangCodeExpander.h"
#include "Util.h"
#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML.h"

#define MAKECODE(a, b, c, d)  ((((long)(a)) << 24) | (((long)(b)) << 16) | (((long)(c)) << 8) | (long)(d))
#define MAKETWOCHARCODE(a, b) ((((long)(a)) << 8) | (long)(b)) 

typedef struct LCENTRY
{
  long code;
  const char *name;
} LCENTRY;

extern const struct LCENTRY g_iso639_1[186];
extern const struct LCENTRY g_iso639_2[538];

struct ISO639
{
  const char* iso639_1;
  const char* iso639_2;
  const char* win_id;
};

struct ISO3166_1
{
  const char* alpha2;
  const char* alpha3;
};

// declared as extern to allow forward declaration
extern const ISO639 LanguageCodes[189];
extern const ISO3166_1 RegionCodes[246];

CLangCodeExpander::CLangCodeExpander()
{ }

CLangCodeExpander::~CLangCodeExpander()
{ }

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
  int iSplit = code.find("-");
  if (iSplit > 0)
  {
    std::string strLeft, strRight;
    const bool bLeft = Lookup(code.substr(0, iSplit), strLeft);
    const bool bRight = Lookup(code.substr(iSplit + 1), strRight);
    if (bLeft || bRight)
    {
      desc = "";
      if (strLeft.length() > 0)
        desc = strLeft;
      else
        desc = code.substr(0, iSplit);

      if (strRight.length() > 0)
      {
        desc += " - ";
        desc += strRight;
      }
      else
      {
        desc += " - ";
        desc += code.substr(iSplit + 1);
      }

      return true;
    }

    return false;
  }

  if (LookupInUserMap(code, desc))
    return true;

  if (LookupInISO639Tables(code, desc))
    return true;

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

bool CLangCodeExpander::ConvertISO6391ToISO6392T(const std::string& strISO6391, std::string& strISO6392T, bool checkWin32Locales /* = false */)
{
  // not a 2 char code
  if (strISO6391.length() != 2)
    return false;

  std::string strISO6391Lower(strISO6391);
  StringUtils::ToLower(strISO6391Lower);
  StringUtils::Trim(strISO6391Lower);

  for (unsigned int index = 0; index < ARRAY_SIZE(LanguageCodes); ++index)
  {
    if (strISO6391Lower == LanguageCodes[index].iso639_1)
    {
      if (checkWin32Locales && LanguageCodes[index].win_id)
      {
        strISO6392T = LanguageCodes[index].win_id;
        return true;
      }

      strISO6392T = LanguageCodes[index].iso639_2;
      return true;
    }
  }

  return false;
}

bool CLangCodeExpander::ConvertToISO6392T(const std::string& strCharCode, std::string& strISO6392T, bool checkWin32Locales /* = false */)
{

  //first search in the user defined map
  if (LookupUserCode(strCharCode, strISO6392T))
    return true;

  if (strCharCode.size() == 2)
    return g_LangCodeExpander.ConvertISO6391ToISO6392T(strCharCode, strISO6392T, checkWin32Locales);

  if (strCharCode.size() == 3)
  {
    std::string charCode(strCharCode); StringUtils::ToLower(charCode);
    for (unsigned int index = 0; index < ARRAY_SIZE(LanguageCodes); ++index)
    {
      if (charCode == LanguageCodes[index].iso639_2 ||
         (checkWin32Locales && LanguageCodes[index].win_id != NULL && charCode == LanguageCodes[index].win_id))
      {
        strISO6392T = charCode;
        return true;
      }
    }

    for (unsigned int index = 0; index < ARRAY_SIZE(RegionCodes); ++index)
    {
      if (charCode == RegionCodes[index].alpha3)
      {
        strISO6392T = charCode;
        return true;
      }
    }
  }
  else if (strCharCode.size() > 3)
  {
    for (unsigned int i = 0; i < sizeof(g_iso639_2) / sizeof(LCENTRY); i++)
    {
      if (StringUtils::EqualsNoCase(strCharCode, g_iso639_2[i].name))
      {
        CodeToString(g_iso639_2[i].code, strISO6392T);
        return true;
      }
    }
  }
  return false;
}

bool CLangCodeExpander::LookupUserCode(const std::string& desc, std::string &userCode)
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
bool CLangCodeExpander::ConvertISO36111Alpha2ToISO36111Alpha3(const std::string& strISO36111Alpha2, std::string& strISO36111Alpha3)
{
  if (strISO36111Alpha2.length() != 2)
    return false;

  std::string strLower(strISO36111Alpha2);
  StringUtils::ToLower(strLower);
  StringUtils::Trim(strLower);
  for (unsigned int index = 0; index < ARRAY_SIZE(RegionCodes); ++index)
  {
    if (strLower == RegionCodes[index].alpha2)
    {
      strISO36111Alpha3 = RegionCodes[index].alpha3;
      return true;
    }
  }

  return true;
}

bool CLangCodeExpander::ConvertWindowsLanguageCodeToISO6392T(const std::string& strWindowsLanguageCode, std::string& strISO6392T)
{
  if (strWindowsLanguageCode.length() != 3)
    return false;

  std::string strLower(strWindowsLanguageCode);
  StringUtils::ToLower(strLower);
  for (unsigned int index = 0; index < ARRAY_SIZE(LanguageCodes); ++index)
  {
    if ((LanguageCodes[index].win_id && strLower == LanguageCodes[index].win_id) ||
         strLower == LanguageCodes[index].iso639_2)
    {
      strISO6392T = LanguageCodes[index].iso639_2;
      return true;
    }
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
    std::string lower(lang); StringUtils::ToLower(lower);
    for (unsigned int index = 0; index < ARRAY_SIZE(LanguageCodes); ++index)
    {
      if (lower == LanguageCodes[index].iso639_2 || (LanguageCodes[index].win_id && lower == LanguageCodes[index].win_id))
      {
        code = LanguageCodes[index].iso639_1;
        return true;
      }
    }

    for (unsigned int index = 0; index < ARRAY_SIZE(RegionCodes); ++index)
    {
      if (lower == RegionCodes[index].alpha3)
      {
        code = RegionCodes[index].alpha2;
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
      return ConvertToISO6391(tmp, code);
  }

  return false;
}

bool CLangCodeExpander::ReverseLookup(const std::string& desc, std::string& code)
{
  if (desc.empty())
    return false;

  std::string descTmp(desc);
  StringUtils::Trim(descTmp);
  for (STRINGLOOKUPTABLE::const_iterator it = m_mapUser.begin(); it != m_mapUser.end(); ++it)
  {
    if (StringUtils::EqualsNoCase(descTmp, it->second))
    {
      code = it->first;
      return true;
    }
  }

  for (unsigned int i = 0; i < sizeof(g_iso639_1) / sizeof(LCENTRY); i++)
  {
    if (StringUtils::EqualsNoCase(descTmp, g_iso639_1[i].name))
    {
      CodeToString(g_iso639_1[i].code, code);
      return true;
    }
  }

  for (unsigned int i = 0; i < sizeof(g_iso639_2) / sizeof(LCENTRY); i++)
  {
    if (StringUtils::EqualsNoCase(descTmp, g_iso639_2[i].name))
    {
      CodeToString(g_iso639_2[i].code, code);
      return true;
    }
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

bool CLangCodeExpander::LookupInISO639Tables(const std::string& code, std::string& desc)
{
  if (code.empty())
    return false;

  long longcode;
  std::string sCode(code);
  StringUtils::ToLower(sCode);
  StringUtils::Trim(sCode);

  if (sCode.length() == 2)
  {
    longcode = MAKECODE('\0', '\0', sCode[0], sCode[1]);
    for (unsigned int i = 0; i < sizeof(g_iso639_1) / sizeof(LCENTRY); i++)
    {
      if (g_iso639_1[i].code == longcode)
      {
        desc = g_iso639_1[i].name;
        return true;
      }
    }
  }
  else if (sCode.length() == 3)
  {
    longcode = MAKECODE('\0', sCode[0], sCode[1], sCode[2]);
    for (unsigned int i = 0; i < sizeof(g_iso639_2) / sizeof(LCENTRY); i++)
    {
      if (g_iso639_2[i].code == longcode)
      {
        desc = g_iso639_2[i].name;
        return true;
      }
    }
  }
  return false;
}

void CLangCodeExpander::CodeToString(long code, std::string& ret)
{
  ret.clear();
  for (unsigned int j = 0; j < 4; j++)
  {
    char c = (char)code & 0xFF;
    if (c == '\0')
      return;

    ret.insert(0, 1, c);
    code >>= 8;
  }
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

std::vector<std::string> CLangCodeExpander::GetLanguageNames(LANGFORMATS format /* = CLangCodeExpander::ISO_639_1 */, bool customNames /* = false */)
{
  std::vector<std::string> languages;
  const LCENTRY *lang = g_iso639_1;
  size_t length = sizeof(g_iso639_1);
  if (format == CLangCodeExpander::ISO_639_2)
  {
    lang = g_iso639_2;
    length = sizeof(g_iso639_2);
  }
  length /= sizeof(LCENTRY);

  for (size_t i = 0; i < length; i++)
  {
    languages.push_back(lang->name);
    ++lang;
  }

  if (customNames)
  {
    for (STRINGLOOKUPTABLE::const_iterator it = m_mapUser.begin(); it != m_mapUser.end(); ++it)
      languages.push_back(it->second);
  }

  return languages;
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

extern const LCENTRY g_iso639_1[186] =
{
  { MAKECODE('\0','\0','c','c'), "Closed Caption" },
  { MAKECODE('\0','\0','a','a'), "Afar" },
  { MAKECODE('\0','\0','a','b'), "Abkhazian" },
  { MAKECODE('\0','\0','a','e'), "Avestan" },
  { MAKECODE('\0','\0','a','f'), "Afrikaans" },
  { MAKECODE('\0','\0','a','k'), "Akan" },
  { MAKECODE('\0','\0','a','m'), "Amharic" },
  { MAKECODE('\0','\0','a','n'), "Aragonese" },
  { MAKECODE('\0','\0','a','r'), "Arabic" },
  { MAKECODE('\0','\0','a','s'), "Assamese" },
  { MAKECODE('\0','\0','a','v'), "Avaric" },
  { MAKECODE('\0','\0','a','y'), "Aymara" },
  { MAKECODE('\0','\0','a','z'), "Azerbaijani" },
  { MAKECODE('\0','\0','b','a'), "Bashkir" },
  { MAKECODE('\0','\0','b','e'), "Belarusian" },
  { MAKECODE('\0','\0','b','g'), "Bulgarian" },
  { MAKECODE('\0','\0','b','h'), "Bihari" },
  { MAKECODE('\0','\0','b','i'), "Bislama" },
  { MAKECODE('\0','\0','b','m'), "Bambara" },
  { MAKECODE('\0','\0','b','n'), "Bengali; Bangla" },
  { MAKECODE('\0','\0','b','o'), "Tibetan" },
  { MAKECODE('\0','\0','b','r'), "Breton" },
  { MAKECODE('\0','\0','b','s'), "Bosnian" },
  { MAKECODE('\0','\0','c','a'), "Catalan" },
  { MAKECODE('\0','\0','c','e'), "Chechen" },
  { MAKECODE('\0','\0','c','h'), "Chamorro" },
  { MAKECODE('\0','\0','c','o'), "Corsican" },
  { MAKECODE('\0','\0','c','r'), "Cree" },
  { MAKECODE('\0','\0','c','s'), "Czech" },
  { MAKECODE('\0','\0','c','u'), "Church Slavic" },
  { MAKECODE('\0','\0','c','v'), "Chuvash" },
  { MAKECODE('\0','\0','c','y'), "Welsh" },
  { MAKECODE('\0','\0','d','a'), "Danish" },
  { MAKECODE('\0','\0','d','e'), "German" },
  { MAKECODE('\0','\0','d','v'), "Dhivehi" },
  { MAKECODE('\0','\0','d','z'), "Dzongkha" },
  { MAKECODE('\0','\0','e','e'), "Ewe" },
  { MAKECODE('\0','\0','e','l'), "Greek" },
  { MAKECODE('\0','\0','e','n'), "English" },
  { MAKECODE('\0','\0','e','o'), "Esperanto" },
  { MAKECODE('\0','\0','e','s'), "Spanish" },
  { MAKECODE('\0','\0','e','t'), "Estonian" },
  { MAKECODE('\0','\0','e','u'), "Basque" },
  { MAKECODE('\0','\0','f','a'), "Persian" },
  { MAKECODE('\0','\0','f','f'), "Fulah" },
  { MAKECODE('\0','\0','f','i'), "Finnish" },
  { MAKECODE('\0','\0','f','j'), "Fijian" },
  { MAKECODE('\0','\0','f','o'), "Faroese" },
  { MAKECODE('\0','\0','f','r'), "French" },
  { MAKECODE('\0','\0','f','y'), "Western Frisian" },
  { MAKECODE('\0','\0','g','a'), "Irish" },
  { MAKECODE('\0','\0','g','d'), "Scottish Gaelic" },
  { MAKECODE('\0','\0','g','l'), "Galician" },
  { MAKECODE('\0','\0','g','n'), "Guarani" },
  { MAKECODE('\0','\0','g','u'), "Gujarati" },
  { MAKECODE('\0','\0','g','v'), "Manx" },
  { MAKECODE('\0','\0','h','a'), "Hausa" },
  { MAKECODE('\0','\0','h','e'), "Hebrew" },
  { MAKECODE('\0','\0','h','i'), "Hindi" },
  { MAKECODE('\0','\0','h','o'), "Hiri Motu" },
  { MAKECODE('\0','\0','h','r'), "Croatian" },
  { MAKECODE('\0','\0','h','t'), "Haitian" },
  { MAKECODE('\0','\0','h','u'), "Hungarian" },
  { MAKECODE('\0','\0','h','y'), "Armenian" },
  { MAKECODE('\0','\0','h','z'), "Herero" },
  { MAKECODE('\0','\0','i','a'), "Interlingua" },
  { MAKECODE('\0','\0','i','d'), "Indonesian" },
  { MAKECODE('\0','\0','i','e'), "Interlingue" },
  { MAKECODE('\0','\0','i','g'), "Igbo" },
  { MAKECODE('\0','\0','i','i'), "Sichuan Yi" },
  { MAKECODE('\0','\0','i','k'), "Inupiat" },
  { MAKECODE('\0','\0','i','o'), "Ido" },
  { MAKECODE('\0','\0','i','s'), "Icelandic" },
  { MAKECODE('\0','\0','i','t'), "Italian" },
  { MAKECODE('\0','\0','i','u'), "Inuktitut" },
  { MAKECODE('\0','\0','j','a'), "Japanese" },
  { MAKECODE('\0','\0','j','v'), "Javanese" },
  { MAKECODE('\0','\0','k','a'), "Georgian" },
  { MAKECODE('\0','\0','k','g'), "Kongo" },
  { MAKECODE('\0','\0','k','i'), "Kikuyu" },
  { MAKECODE('\0','\0','k','j'), "Kuanyama" },
  { MAKECODE('\0','\0','k','k'), "Kazakh" },
  { MAKECODE('\0','\0','k','l'), "Kalaallisut" },
  { MAKECODE('\0','\0','k','m'), "Khmer" },
  { MAKECODE('\0','\0','k','n'), "Kannada" },
  { MAKECODE('\0','\0','k','o'), "Korean" },
  { MAKECODE('\0','\0','k','r'), "Kanuri" },
  { MAKECODE('\0','\0','k','s'), "Kashmiri" },
  { MAKECODE('\0','\0','k','u'), "Kurdish" },
  { MAKECODE('\0','\0','k','v'), "Komi" },
  { MAKECODE('\0','\0','k','w'), "Cornish" },
  { MAKECODE('\0','\0','k','y'), "Kirghiz" },
  { MAKECODE('\0','\0','l','a'), "Latin" },
  { MAKECODE('\0','\0','l','b'), "Luxembourgish" },
  { MAKECODE('\0','\0','l','g'), "Ganda" },
  { MAKECODE('\0','\0','l','i'), "Limburgan" },
  { MAKECODE('\0','\0','l','n'), "Lingala" },
  { MAKECODE('\0','\0','l','o'), "Lao" },
  { MAKECODE('\0','\0','l','t'), "Lithuanian" },
  { MAKECODE('\0','\0','l','u'), "Luba-Katanga" },
  { MAKECODE('\0','\0','l','v'), "Latvian, Lettish" },
  { MAKECODE('\0','\0','m','g'), "Malagasy" },
  { MAKECODE('\0','\0','m','h'), "Marshallese" },
  { MAKECODE('\0','\0','m','i'), "Maori" },
  { MAKECODE('\0','\0','m','k'), "Macedonian" },
  { MAKECODE('\0','\0','m','l'), "Malayalam" },
  { MAKECODE('\0','\0','m','n'), "Mongolian" },
  { MAKECODE('\0','\0','m','r'), "Marathi" },
  { MAKECODE('\0','\0','m','s'), "Malay" },
  { MAKECODE('\0','\0','m','t'), "Maltese" },
  { MAKECODE('\0','\0','m','y'), "Burmese" },
  { MAKECODE('\0','\0','n','a'), "Nauru" },
  { MAKECODE('\0','\0','n','b'), "Bokm\xC3\xA5l, Norwegian" },
  { MAKECODE('\0','\0','n','d'), "Ndebele, North" },
  { MAKECODE('\0','\0','n','e'), "Nepali" },
  { MAKECODE('\0','\0','n','g'), "Ndonga" },
  { MAKECODE('\0','\0','n','l'), "Dutch" },
  { MAKECODE('\0','\0','n','n'), "Norwegian Nynorsk" },
  { MAKECODE('\0','\0','n','o'), "Norwegian" },
  { MAKECODE('\0','\0','n','r'), "Ndebele, South" },
  { MAKECODE('\0','\0','n','v'), "Navajo" },
  { MAKECODE('\0','\0','n','y'), "Chichewa" },
  { MAKECODE('\0','\0','o','c'), "Occitan" },
  { MAKECODE('\0','\0','o','j'), "Ojibwa" },
  { MAKECODE('\0','\0','o','m'), "Oromo" },
  { MAKECODE('\0','\0','o','r'), "Oriya" },
  { MAKECODE('\0','\0','o','s'), "Ossetic" },
  { MAKECODE('\0','\0','p','a'), "Punjabi" },
  { MAKECODE('\0','\0','p','i'), "Pali" },
  { MAKECODE('\0','\0','p','l'), "Polish" },
  { MAKECODE('\0','\0','p','s'), "Pashto, Pushto" },
  { MAKECODE('\0','\0','p','t'), "Portuguese" },
  { MAKECODE('\0','\0','q','u'), "Quechua" },
  { MAKECODE('\0','\0','r','m'), "Romansh" },
  { MAKECODE('\0','\0','r','n'), "Kirundi" },
  { MAKECODE('\0','\0','r','o'), "Romanian" },
  { MAKECODE('\0','\0','r','u'), "Russian" },
  { MAKECODE('\0','\0','r','w'), "Kinyarwanda" },
  { MAKECODE('\0','\0','s','a'), "Sanskrit" },
  { MAKECODE('\0','\0','s','c'), "Sardinian" },
  { MAKECODE('\0','\0','s','d'), "Sindhi" },
  { MAKECODE('\0','\0','s','e'), "Northern Sami" },
  { MAKECODE('\0','\0','s','g'), "Sangho" },
  { MAKECODE('\0','\0','s','h'), "Serbo-Croatian" },
  { MAKECODE('\0','\0','s','i'), "Sinhalese" },
  { MAKECODE('\0','\0','s','k'), "Slovak" },
  { MAKECODE('\0','\0','s','l'), "Slovenian" },
  { MAKECODE('\0','\0','s','m'), "Samoan" },
  { MAKECODE('\0','\0','s','n'), "Shona" },
  { MAKECODE('\0','\0','s','o'), "Somali" },
  { MAKECODE('\0','\0','s','q'), "Albanian" },
  { MAKECODE('\0','\0','s','r'), "Serbian" },
  { MAKECODE('\0','\0','s','s'), "Swati" },
  { MAKECODE('\0','\0','s','t'), "Sesotho" },
  { MAKECODE('\0','\0','s','u'), "Sundanese" },
  { MAKECODE('\0','\0','s','v'), "Swedish" },
  { MAKECODE('\0','\0','s','w'), "Swahili" },
  { MAKECODE('\0','\0','t','a'), "Tamil" },
  { MAKECODE('\0','\0','t','e'), "Telugu" },
  { MAKECODE('\0','\0','t','g'), "Tajik" },
  { MAKECODE('\0','\0','t','h'), "Thai" },
  { MAKECODE('\0','\0','t','i'), "Tigrinya" },
  { MAKECODE('\0','\0','t','k'), "Turkmen" },
  { MAKECODE('\0','\0','t','l'), "Tagalog" },
  { MAKECODE('\0','\0','t','n'), "Tswana" },
  { MAKECODE('\0','\0','t','o'), "Tonga" },
  { MAKECODE('\0','\0','t','r'), "Turkish" },
  { MAKECODE('\0','\0','t','s'), "Tsonga" },
  { MAKECODE('\0','\0','t','t'), "Tatar" },
  { MAKECODE('\0','\0','t','w'), "Twi" },
  { MAKECODE('\0','\0','t','y'), "Tahitian" },
  { MAKECODE('\0','\0','u','g'), "Uighur" },
  { MAKECODE('\0','\0','u','k'), "Ukrainian" },
  { MAKECODE('\0','\0','u','r'), "Urdu" },
  { MAKECODE('\0','\0','u','z'), "Uzbek" },
  { MAKECODE('\0','\0','v','e'), "Venda" },
  { MAKECODE('\0','\0','v','i'), "Vietnamese" },
  { MAKECODE('\0','\0','v','o'), "Volapuk" },
  { MAKECODE('\0','\0','w','a'), "Walloon" },
  { MAKECODE('\0','\0','w','o'), "Wolof" },
  { MAKECODE('\0','\0','x','h'), "Xhosa" },
  { MAKECODE('\0','\0','y','i'), "Yiddish" },
  { MAKECODE('\0','\0','y','o'), "Yoruba" },
  { MAKECODE('\0','\0','z','a'), "Zhuang" },
  { MAKECODE('\0','\0','z','h'), "Chinese" },
  { MAKECODE('\0','\0','z','u'), "Zulu" },
};

extern const LCENTRY g_iso639_2[538] =
{
  { MAKECODE('\0','a','b','k'), "Abkhaz" },
  { MAKECODE('\0','a','b','k'), "Abkhazian" },
  { MAKECODE('\0','a','c','e'), "Achinese" },
  { MAKECODE('\0','a','c','h'), "Acoli" },
  { MAKECODE('\0','a','d','a'), "Adangme" },
  { MAKECODE('\0','a','d','y'), "Adygei" },
  { MAKECODE('\0','a','d','y'), "Adyghe" },
  { MAKECODE('\0','a','a','r'), "Afar" },
  { MAKECODE('\0','a','f','h'), "Afrihili" },
  { MAKECODE('\0','a','f','r'), "Afrikaans" },
  { MAKECODE('\0','a','f','a'), "Afro-Asiatic (Other)" },
  { MAKECODE('\0','a','k','a'), "Akan" },
  { MAKECODE('\0','a','k','k'), "Akkadian" },
  { MAKECODE('\0','a','l','b'), "Albanian" },
  { MAKECODE('\0','s','q','i'), "Albanian" },
  { MAKECODE('\0','a','l','e'), "Aleut" },
  { MAKECODE('\0','a','l','g'), "Algonquian languages" },
  { MAKECODE('\0','t','u','t'), "Altaic (Other)" },
  { MAKECODE('\0','a','m','h'), "Amharic" },
  { MAKECODE('\0','a','p','a'), "Apache languages" },
  { MAKECODE('\0','a','r','a'), "Arabic" },
  { MAKECODE('\0','a','r','g'), "Aragonese" },
  { MAKECODE('\0','a','r','c'), "Aramaic" },
  { MAKECODE('\0','a','r','p'), "Arapaho" },
  { MAKECODE('\0','a','r','n'), "Araucanian" },
  { MAKECODE('\0','a','r','w'), "Arawak" },
  { MAKECODE('\0','a','r','m'), "Armenian" },
  { MAKECODE('\0','h','y','e'), "Armenian" },
  { MAKECODE('\0','a','r','t'), "Artificial (Other)" },
  { MAKECODE('\0','a','s','m'), "Assamese" },
  { MAKECODE('\0','a','s','t'), "Asturian" },
  { MAKECODE('\0','a','t','h'), "Athapascan languages" },
  { MAKECODE('\0','a','u','s'), "Australian languages" },
  { MAKECODE('\0','m','a','p'), "Austronesian (Other)" },
  { MAKECODE('\0','a','v','a'), "Avaric" },
  { MAKECODE('\0','a','v','e'), "Avestan" },
  { MAKECODE('\0','a','w','a'), "Awadhi" },
  { MAKECODE('\0','a','y','m'), "Aymara" },
  { MAKECODE('\0','a','z','e'), "Azerbaijani" },
  { MAKECODE('\0','a','s','t'), "Bable" },
  { MAKECODE('\0','b','a','n'), "Balinese" },
  { MAKECODE('\0','b','a','t'), "Baltic (Other)" },
  { MAKECODE('\0','b','a','l'), "Baluchi" },
  { MAKECODE('\0','b','a','m'), "Bambara" },
  { MAKECODE('\0','b','a','i'), "Bamileke languages" },
  { MAKECODE('\0','b','a','d'), "Banda" },
  { MAKECODE('\0','b','n','t'), "Bantu (Other)" },
  { MAKECODE('\0','b','a','s'), "Basa" },
  { MAKECODE('\0','b','a','k'), "Bashkir" },
  { MAKECODE('\0','b','a','q'), "Basque" },
  { MAKECODE('\0','e','u','s'), "Basque" },
  { MAKECODE('\0','b','t','k'), "Batak (Indonesia)" },
  { MAKECODE('\0','b','e','j'), "Beja" },
  { MAKECODE('\0','b','e','l'), "Belarusian" },
  { MAKECODE('\0','b','e','m'), "Bemba" },
  { MAKECODE('\0','b','e','n'), "Bengali" },
  { MAKECODE('\0','b','e','r'), "Berber (Other)" },
  { MAKECODE('\0','b','h','o'), "Bhojpuri" },
  { MAKECODE('\0','b','i','h'), "Bihari" },
  { MAKECODE('\0','b','i','k'), "Bikol" },
  { MAKECODE('\0','b','y','n'), "Bilin" },
  { MAKECODE('\0','b','i','n'), "Bini" },
  { MAKECODE('\0','b','i','s'), "Bislama" },
  { MAKECODE('\0','b','y','n'), "Blin" },
  { MAKECODE('\0','n','o','b'), "Bokm\xC3\xA5l, Norwegian" },
  { MAKECODE('\0','b','o','s'), "Bosnian" },
  { MAKECODE('\0','b','r','a'), "Braj" },
  { MAKECODE('\0','b','r','e'), "Breton" },
  { MAKECODE('\0','b','u','g'), "Buginese" },
  { MAKECODE('\0','b','u','l'), "Bulgarian" },
  { MAKECODE('\0','b','u','a'), "Buriat" },
  { MAKECODE('\0','b','u','r'), "Burmese" },
  { MAKECODE('\0','m','y','a'), "Burmese" },
  { MAKECODE('\0','c','a','d'), "Caddo" },
  { MAKECODE('\0','c','a','r'), "Carib" },
  { MAKECODE('\0','s','p','a'), "Spanish" },
  { MAKECODE('\0','c','a','t'), "Catalan" },
  { MAKECODE('\0','c','a','u'), "Caucasian (Other)" },
  { MAKECODE('\0','c','e','b'), "Cebuano" },
  { MAKECODE('\0','c','e','l'), "Celtic (Other)" },
  { MAKECODE('\0','c','h','g'), "Chagatai" },
  { MAKECODE('\0','c','m','c'), "Chamic languages" },
  { MAKECODE('\0','c','h','a'), "Chamorro" },
  { MAKECODE('\0','c','h','e'), "Chechen" },
  { MAKECODE('\0','c','h','r'), "Cherokee" },
  { MAKECODE('\0','n','y','a'), "Chewa" },
  { MAKECODE('\0','c','h','y'), "Cheyenne" },
  { MAKECODE('\0','c','h','b'), "Chibcha" },
  { MAKECODE('\0','n','y','a'), "Chichewa" },
  { MAKECODE('\0','c','h','i'), "Chinese" },
  { MAKECODE('\0','z','h','o'), "Chinese" },
  { MAKECODE('\0','c','h','n'), "Chinook jargon" },
  { MAKECODE('\0','c','h','p'), "Chipewyan" },
  { MAKECODE('\0','c','h','o'), "Choctaw" },
  { MAKECODE('\0','z','h','a'), "Chuang" },
  { MAKECODE('\0','c','h','u'), "Church Slavonic" },
  { MAKECODE('\0','c','h','k'), "Chuukese" },
  { MAKECODE('\0','c','h','v'), "Chuvash" },
  { MAKECODE('\0','n','w','c'), "Classical Nepal Bhasa" },
  { MAKECODE('\0','n','w','c'), "Classical Newari" },
  { MAKECODE('\0','c','o','p'), "Coptic" },
  { MAKECODE('\0','c','o','r'), "Cornish" },
  { MAKECODE('\0','c','o','s'), "Corsican" },
  { MAKECODE('\0','c','r','e'), "Cree" },
  { MAKECODE('\0','m','u','s'), "Creek" },
  { MAKECODE('\0','c','r','p'), "Creoles and pidgins (Other)" },
  { MAKECODE('\0','c','p','e'), "English-based (Other)" },
  { MAKECODE('\0','c','p','f'), "French-based (Other)" },
  { MAKECODE('\0','c','p','p'), "Portuguese-based (Other)" },
  { MAKECODE('\0','c','r','h'), "Crimean Tatar" },
  { MAKECODE('\0','c','r','h'), "Crimean Turkish" },
  { MAKECODE('\0','h','r','v'), "Croatian" },
  { MAKECODE('\0','s','c','r'), "Croatian" },
  { MAKECODE('\0','c','u','s'), "Cushitic (Other)" },
  { MAKECODE('\0','c','z','e'), "Czech" },
  { MAKECODE('\0','c','e','s'), "Czech" },
  { MAKECODE('\0','d','a','k'), "Dakota" },
  { MAKECODE('\0','d','a','n'), "Danish" },
  { MAKECODE('\0','d','a','r'), "Dargwa" },
  { MAKECODE('\0','d','a','y'), "Dayak" },
  { MAKECODE('\0','d','e','l'), "Delaware" },
  { MAKECODE('\0','d','i','n'), "Dinka" },
  { MAKECODE('\0','d','i','v'), "Divehi" },
  { MAKECODE('\0','d','o','i'), "Dogri" },
  { MAKECODE('\0','d','g','r'), "Dogrib" },
  { MAKECODE('\0','d','r','a'), "Dravidian (Other)" },
  { MAKECODE('\0','d','u','a'), "Duala" },
  { MAKECODE('\0','d','u','t'), "Dutch" },
  { MAKECODE('\0','n','l','d'), "Dutch" },
  { MAKECODE('\0','d','u','m'), "Dutch, Middle (ca. 1050-1350)" },
  { MAKECODE('\0','d','y','u'), "Dyula" },
  { MAKECODE('\0','d','z','o'), "Dzongkha" },
  { MAKECODE('\0','e','f','i'), "Efik" },
  { MAKECODE('\0','e','g','y'), "Egyptian (Ancient)" },
  { MAKECODE('\0','e','k','a'), "Ekajuk" },
  { MAKECODE('\0','e','l','x'), "Elamite" },
  { MAKECODE('\0','e','n','g'), "English" },
  { MAKECODE('\0','e','n','m'), "English, Middle (1100-1500)" },
  { MAKECODE('\0','a','n','g'), "English, Old (ca.450-1100)" },
  { MAKECODE('\0','m','y','v'), "Erzya" },
  { MAKECODE('\0','e','p','o'), "Esperanto" },
  { MAKECODE('\0','e','s','t'), "Estonian" },
  { MAKECODE('\0','e','w','e'), "Ewe" },
  { MAKECODE('\0','e','w','o'), "Ewondo" },
  { MAKECODE('\0','f','a','n'), "Fang" },
  { MAKECODE('\0','f','a','t'), "Fanti" },
  { MAKECODE('\0','f','a','o'), "Faroese" },
  { MAKECODE('\0','f','i','j'), "Fijian" },
  { MAKECODE('\0','f','i','l'), "Filipino" },
  { MAKECODE('\0','f','i','n'), "Finnish" },
  { MAKECODE('\0','f','i','u'), "Finno-Ugrian (Other)" },
  { MAKECODE('\0','d','u','t'), "Flemish" },
  { MAKECODE('\0','n','l','d'), "Flemish" },
  { MAKECODE('\0','f','o','n'), "Fon" },
  { MAKECODE('\0','f','r','e'), "French" },
  { MAKECODE('\0','f','r','a'), "French" },
  { MAKECODE('\0','f','r','m'), "French, Middle (ca.1400-1600)" },
  { MAKECODE('\0','f','r','o'), "French, Old (842-ca.1400)" },
  { MAKECODE('\0','f','r','y'), "Frisian" },
  { MAKECODE('\0','f','u','r'), "Friulian" },
  { MAKECODE('\0','f','u','l'), "Fulah" },
  { MAKECODE('\0','g','a','a'), "Ga" },
  { MAKECODE('\0','g','l','a'), "Gaelic" },
  { MAKECODE('\0','g','l','g'), "Gallegan" },
  { MAKECODE('\0','l','u','g'), "Ganda" },
  { MAKECODE('\0','g','a','y'), "Gayo" },
  { MAKECODE('\0','g','b','a'), "Gbaya" },
  { MAKECODE('\0','g','e','z'), "Geez" },
  { MAKECODE('\0','g','e','o'), "Georgian" },
  { MAKECODE('\0','k','a','t'), "Georgian" },
  { MAKECODE('\0','g','e','r'), "German" },
  { MAKECODE('\0','d','e','u'), "German" },
  { MAKECODE('\0','n','d','s'), "German, Low" },
  { MAKECODE('\0','g','m','h'), "German, Middle High (ca.1050-1500)" },
  { MAKECODE('\0','g','o','h'), "German, Old High (ca.750-1050)" },
  { MAKECODE('\0','g','e','m'), "Germanic (Other)" },
  { MAKECODE('\0','k','i','k'), "Gikuyu" },
  { MAKECODE('\0','g','i','l'), "Gilbertese" },
  { MAKECODE('\0','g','o','n'), "Gondi" },
  { MAKECODE('\0','g','o','r'), "Gorontalo" },
  { MAKECODE('\0','g','o','t'), "Gothic" },
  { MAKECODE('\0','g','r','b'), "Grebo" },
  { MAKECODE('\0','g','r','c'), "Greek, Ancient (to 1453)" },
  { MAKECODE('\0','g','r','e'), "Greek, Modern (1453-)" },
  { MAKECODE('\0','e','l','l'), "Greek, Modern (1453-)" },
  { MAKECODE('\0','k','a','l'), "Greenlandic" },
  { MAKECODE('\0','g','r','n'), "Guarani" },
  { MAKECODE('\0','g','u','j'), "Gujarati" },
  { MAKECODE('\0','g','w','i'), "Gwich\xC2\xB4in" },
  { MAKECODE('\0','h','a','i'), "Haida" },
  { MAKECODE('\0','h','a','t'), "Haitian" },
  { MAKECODE('\0','h','a','t'), "Haitian Creole" },
  { MAKECODE('\0','h','a','u'), "Hausa" },
  { MAKECODE('\0','h','a','w'), "Hawaiian" },
  { MAKECODE('\0','h','e','b'), "Hebrew" },
  { MAKECODE('\0','h','e','r'), "Herero" },
  { MAKECODE('\0','h','i','l'), "Hiligaynon" },
  { MAKECODE('\0','h','i','m'), "Himachali" },
  { MAKECODE('\0','h','i','n'), "Hindi" },
  { MAKECODE('\0','h','m','o'), "Hiri Motu" },
  { MAKECODE('\0','h','i','t'), "Hittite" },
  { MAKECODE('\0','h','m','n'), "Hmong" },
  { MAKECODE('\0','h','u','n'), "Hungarian" },
  { MAKECODE('\0','h','u','p'), "Hupa" },
  { MAKECODE('\0','i','b','a'), "Iban" },
  { MAKECODE('\0','i','c','e'), "Icelandic" },
  { MAKECODE('\0','i','s','l'), "Icelandic" },
  { MAKECODE('\0','i','d','o'), "Ido" },
  { MAKECODE('\0','i','b','o'), "Igbo" },
  { MAKECODE('\0','i','j','o'), "Ijo" },
  { MAKECODE('\0','i','l','o'), "Iloko" },
  { MAKECODE('\0','s','m','n'), "Inari Sami" },
  { MAKECODE('\0','i','n','c'), "Indic (Other)" },
  { MAKECODE('\0','i','n','e'), "Indo-European (Other)" },
  { MAKECODE('\0','i','n','d'), "Indonesian" },
  { MAKECODE('\0','i','n','h'), "Ingush" },
  { MAKECODE('\0','i','n','a'), "Auxiliary Language Association)" },
  { MAKECODE('\0','i','l','e'), "Interlingue" },
  { MAKECODE('\0','i','k','u'), "Inuktitut" },
  { MAKECODE('\0','i','p','k'), "Inupiaq" },
  { MAKECODE('\0','i','r','a'), "Iranian (Other)" },
  { MAKECODE('\0','g','l','e'), "Irish" },
  { MAKECODE('\0','m','g','a'), "Irish, Middle (900-1200)" },
  { MAKECODE('\0','s','g','a'), "Irish, Old (to 900)" },
  { MAKECODE('\0','i','r','o'), "Iroquoian languages" },
  { MAKECODE('\0','i','t','a'), "Italian" },
  { MAKECODE('\0','j','p','n'), "Japanese" },
  { MAKECODE('\0','j','a','v'), "Javanese" },
  { MAKECODE('\0','j','r','b'), "Judeo-Arabic" },
  { MAKECODE('\0','j','p','r'), "Judeo-Persian" },
  { MAKECODE('\0','k','b','d'), "Kabardian" },
  { MAKECODE('\0','k','a','b'), "Kabyle" },
  { MAKECODE('\0','k','a','c'), "Kachin" },
  { MAKECODE('\0','k','a','l'), "Kalaallisut" },
  { MAKECODE('\0','x','a','l'), "Kalmyk" },
  { MAKECODE('\0','k','a','m'), "Kamba" },
  { MAKECODE('\0','k','a','n'), "Kannada" },
  { MAKECODE('\0','k','a','u'), "Kanuri" },
  { MAKECODE('\0','k','r','c'), "Karachay-Balkar" },
  { MAKECODE('\0','k','a','a'), "Kara-Kalpak" },
  { MAKECODE('\0','k','a','r'), "Karen" },
  { MAKECODE('\0','k','a','s'), "Kashmiri" },
  { MAKECODE('\0','c','s','b'), "Kashubian" },
  { MAKECODE('\0','k','a','w'), "Kawi" },
  { MAKECODE('\0','k','a','z'), "Kazakh" },
  { MAKECODE('\0','k','h','a'), "Khasi" },
  { MAKECODE('\0','k','h','m'), "Khmer" },
  { MAKECODE('\0','k','h','i'), "Khoisan (Other)" },
  { MAKECODE('\0','k','h','o'), "Khotanese" },
  { MAKECODE('\0','k','i','k'), "Kikuyu" },
  { MAKECODE('\0','k','m','b'), "Kimbundu" },
  { MAKECODE('\0','k','i','n'), "Kinyarwanda" },
  { MAKECODE('\0','k','i','r'), "Kirghiz" },
  { MAKECODE('\0','t','l','h'), "Klingon" },
  { MAKECODE('\0','k','o','m'), "Komi" },
  { MAKECODE('\0','k','o','n'), "Kongo" },
  { MAKECODE('\0','k','o','k'), "Konkani" },
  { MAKECODE('\0','k','o','r'), "Korean" },
  { MAKECODE('\0','k','o','s'), "Kosraean" },
  { MAKECODE('\0','k','p','e'), "Kpelle" },
  { MAKECODE('\0','k','r','o'), "Kru" },
  { MAKECODE('\0','k','u','a'), "Kuanyama" },
  { MAKECODE('\0','k','u','m'), "Kumyk" },
  { MAKECODE('\0','k','u','r'), "Kurdish" },
  { MAKECODE('\0','k','r','u'), "Kurukh" },
  { MAKECODE('\0','k','u','t'), "Kutenai" },
  { MAKECODE('\0','k','u','a'), "Kwanyama, Kuanyama" },
  { MAKECODE('\0','l','a','d'), "Ladino" },
  { MAKECODE('\0','l','a','h'), "Lahnda" },
  { MAKECODE('\0','l','a','m'), "Lamba" },
  { MAKECODE('\0','l','a','o'), "Lao" },
  { MAKECODE('\0','l','a','t'), "Latin" },
  { MAKECODE('\0','l','a','v'), "Latvian" },
  { MAKECODE('\0','l','t','z'), "Letzeburgesch" },
  { MAKECODE('\0','l','e','z'), "Lezghian" },
  { MAKECODE('\0','l','i','m'), "Limburgan" },
  { MAKECODE('\0','l','i','m'), "Limburger" },
  { MAKECODE('\0','l','i','m'), "Limburgish" },
  { MAKECODE('\0','l','i','n'), "Lingala" },
  { MAKECODE('\0','l','i','t'), "Lithuanian" },
  { MAKECODE('\0','j','b','o'), "Lojban" },
  { MAKECODE('\0','n','d','s'), "Low German" },
  { MAKECODE('\0','n','d','s'), "Low Saxon" },
  { MAKECODE('\0','d','s','b'), "Lower Sorbian" },
  { MAKECODE('\0','l','o','z'), "Lozi" },
  { MAKECODE('\0','l','u','b'), "Luba-Katanga" },
  { MAKECODE('\0','l','u','a'), "Luba-Lulua" },
  { MAKECODE('\0','l','u','i'), "Luiseno" },
  { MAKECODE('\0','s','m','j'), "Lule Sami" },
  { MAKECODE('\0','l','u','n'), "Lunda" },
  { MAKECODE('\0','l','u','o'), "Luo (Kenya and Tanzania)" },
  { MAKECODE('\0','l','u','s'), "Lushai" },
  { MAKECODE('\0','l','t','z'), "Luxembourgish" },
  { MAKECODE('\0','m','a','c'), "Macedonian" },
  { MAKECODE('\0','m','k','d'), "Macedonian" },
  { MAKECODE('\0','m','a','d'), "Madurese" },
  { MAKECODE('\0','m','a','g'), "Magahi" },
  { MAKECODE('\0','m','a','i'), "Maithili" },
  { MAKECODE('\0','m','a','k'), "Makasar" },
  { MAKECODE('\0','m','l','g'), "Malagasy" },
  { MAKECODE('\0','m','a','y'), "Malay" },
  { MAKECODE('\0','m','s','a'), "Malay" },
  { MAKECODE('\0','m','a','l'), "Malayalam" },
  { MAKECODE('\0','m','l','t'), "Maltese" },
  { MAKECODE('\0','m','n','c'), "Manchu" },
  { MAKECODE('\0','m','d','r'), "Mandar" },
  { MAKECODE('\0','m','a','n'), "Mandingo" },
  { MAKECODE('\0','m','n','i'), "Manipuri" },
  { MAKECODE('\0','m','n','o'), "Manobo languages" },
  { MAKECODE('\0','g','l','v'), "Manx" },
  { MAKECODE('\0','m','a','o'), "Maori" },
  { MAKECODE('\0','m','r','i'), "Maori" },
  { MAKECODE('\0','m','a','r'), "Marathi" },
  { MAKECODE('\0','c','h','m'), "Mari" },
  { MAKECODE('\0','m','a','h'), "Marshallese" },
  { MAKECODE('\0','m','w','r'), "Marwari" },
  { MAKECODE('\0','m','a','s'), "Masai" },
  { MAKECODE('\0','m','y','n'), "Mayan languages" },
  { MAKECODE('\0','m','e','n'), "Mende" },
  { MAKECODE('\0','m','i','c'), "Micmac" },
  { MAKECODE('\0','m','i','c'), "Mi'kmaq" },
  { MAKECODE('\0','m','i','n'), "Minangkabau" },
  { MAKECODE('\0','m','w','l'), "Mirandese" },
  { MAKECODE('\0','m','i','s'), "Miscellaneous languages" },
  { MAKECODE('\0','m','o','h'), "Mohawk" },
  { MAKECODE('\0','m','d','f'), "Moksha" },
  { MAKECODE('\0','m','o','l'), "Moldavian" },
  { MAKECODE('\0','m','k','h'), "Mon-Khmer (Other)" },
  { MAKECODE('\0','l','o','l'), "Mongo" },
  { MAKECODE('\0','m','o','n'), "Mongolian" },
  { MAKECODE('\0','m','o','s'), "Mossi" },
  { MAKECODE('\0','m','u','l'), "Multiple languages" },
  { MAKECODE('\0','m','u','n'), "Munda languages" },
  { MAKECODE('\0','n','a','h'), "Nahuatl" },
  { MAKECODE('\0','n','a','u'), "Nauru" },
  { MAKECODE('\0','n','a','v'), "Navaho, Navajo" },
  { MAKECODE('\0','n','a','v'), "Navajo" },
  { MAKECODE('\0','n','d','e'), "Ndebele, North" },
  { MAKECODE('\0','n','b','l'), "Ndebele, South" },
  { MAKECODE('\0','n','d','o'), "Ndonga" },
  { MAKECODE('\0','n','a','p'), "Neapolitan" },
  { MAKECODE('\0','n','e','w'), "Nepal Bhasa" },
  { MAKECODE('\0','n','e','p'), "Nepali" },
  { MAKECODE('\0','n','e','w'), "Newari" },
  { MAKECODE('\0','n','i','a'), "Nias" },
  { MAKECODE('\0','n','i','c'), "Niger-Kordofanian (Other)" },
  { MAKECODE('\0','s','s','a'), "Nilo-Saharan (Other)" },
  { MAKECODE('\0','n','i','u'), "Niuean" },
  { MAKECODE('\0','z','x','x'), "No linguistic content" },
  { MAKECODE('\0','n','o','g'), "Nogai" },
  { MAKECODE('\0','n','o','n'), "Norse, Old" },
  { MAKECODE('\0','n','a','i'), "North American Indian (Other)" },
  { MAKECODE('\0','s','m','e'), "Northern Sami" },
  { MAKECODE('\0','n','s','o'), "Northern Sotho" },
  { MAKECODE('\0','n','d','e'), "North Ndebele" },
  { MAKECODE('\0','n','o','r'), "Norwegian" },
  { MAKECODE('\0','n','o','b'), "Norwegian Bokm\xC3\xA5l" },
  { MAKECODE('\0','n','n','o'), "Norwegian Nynorsk" },
  { MAKECODE('\0','n','u','b'), "Nubian languages" },
  { MAKECODE('\0','n','y','m'), "Nyamwezi" },
  { MAKECODE('\0','n','y','a'), "Nyanja" },
  { MAKECODE('\0','n','y','n'), "Nyankole" },
  { MAKECODE('\0','n','n','o'), "Nynorsk, Norwegian" },
  { MAKECODE('\0','n','y','o'), "Nyoro" },
  { MAKECODE('\0','n','z','i'), "Nzima" },
  { MAKECODE('\0','o','c','i'), "Occitan (post 1500)" },
  { MAKECODE('\0','o','j','i'), "Ojibwa" },
  { MAKECODE('\0','c','h','u'), "Old Bulgarian" },
  { MAKECODE('\0','c','h','u'), "Old Church Slavonic" },
  { MAKECODE('\0','n','w','c'), "Old Newari" },
  { MAKECODE('\0','c','h','u'), "Old Slavonic" },
  { MAKECODE('\0','o','r','i'), "Oriya" },
  { MAKECODE('\0','o','r','m'), "Oromo" },
  { MAKECODE('\0','o','s','a'), "Osage" },
  { MAKECODE('\0','o','s','s'), "Ossetian" },
  { MAKECODE('\0','o','s','s'), "Ossetic" },
  { MAKECODE('\0','o','t','o'), "Otomian languages" },
  { MAKECODE('\0','p','a','l'), "Pahlavi" },
  { MAKECODE('\0','p','a','u'), "Palauan" },
  { MAKECODE('\0','p','l','i'), "Pali" },
  { MAKECODE('\0','p','a','m'), "Pampanga" },
  { MAKECODE('\0','p','a','g'), "Pangasinan" },
  { MAKECODE('\0','p','a','n'), "Panjabi" },
  { MAKECODE('\0','p','a','p'), "Papiamento" },
  { MAKECODE('\0','p','a','a'), "Papuan (Other)" },
  { MAKECODE('\0','n','s','o'), "Pedi" },
  { MAKECODE('\0','p','e','r'), "Persian" },
  { MAKECODE('\0','f','a','s'), "Persian" },
  { MAKECODE('\0','p','e','o'), "Persian, Old (ca.600-400 B.C.)" },
  { MAKECODE('\0','p','h','i'), "Philippine (Other)" },
  { MAKECODE('\0','p','h','n'), "Phoenician" },
  { MAKECODE('\0','f','i','l'), "Pilipino" },
  { MAKECODE('\0','p','o','n'), "Pohnpeian" },
  { MAKECODE('\0','p','o','l'), "Polish" },
  { MAKECODE('\0','p','o','r'), "Portuguese" },
  { MAKECODE('\0','p','r','a'), "Prakrit languages" },
  { MAKECODE('\0','o','c','i'), "Proven\xC3\xA7""al" },
  { MAKECODE('\0','p','r','o'), "Proven\xC3\xA7""al, Old (to 1500)" },
  { MAKECODE('\0','p','a','n'), "Punjabi" },
  { MAKECODE('\0','p','u','s'), "Pushto" },
  { MAKECODE('\0','q','u','e'), "Quechua" },
  { MAKECODE('\0','r','o','h'), "Raeto-Romance" },
  { MAKECODE('\0','r','a','j'), "Rajasthani" },
  { MAKECODE('\0','r','a','p'), "Rapanui" },
  { MAKECODE('\0','r','a','r'), "Rarotongan" },
// { "qaa-qtz", "Reserved for local use" },
  { MAKECODE('\0','r','o','a'), "Romance (Other)" },
  { MAKECODE('\0','r','u','m'), "Romanian" },
  { MAKECODE('\0','r','o','n'), "Romanian" },
  { MAKECODE('\0','r','o','m'), "Romany" },
  { MAKECODE('\0','r','u','n'), "Rundi" },
  { MAKECODE('\0','r','u','s'), "Russian" },
  { MAKECODE('\0','s','a','l'), "Salishan languages" },
  { MAKECODE('\0','s','a','m'), "Samaritan Aramaic" },
  { MAKECODE('\0','s','m','i'), "Sami languages (Other)" },
  { MAKECODE('\0','s','m','o'), "Samoan" },
  { MAKECODE('\0','s','a','d'), "Sandawe" },
  { MAKECODE('\0','s','a','g'), "Sango" },
  { MAKECODE('\0','s','a','n'), "Sanskrit" },
  { MAKECODE('\0','s','a','t'), "Santali" },
  { MAKECODE('\0','s','r','d'), "Sardinian" },
  { MAKECODE('\0','s','a','s'), "Sasak" },
  { MAKECODE('\0','n','d','s'), "Saxon, Low" },
  { MAKECODE('\0','s','c','o'), "Scots" },
  { MAKECODE('\0','g','l','a'), "Scottish Gaelic" },
  { MAKECODE('\0','s','e','l'), "Selkup" },
  { MAKECODE('\0','s','e','m'), "Semitic (Other)" },
  { MAKECODE('\0','n','s','o'), "Sepedi" },
  { MAKECODE('\0','s','c','c'), "Serbian" },
  { MAKECODE('\0','s','r','p'), "Serbian" },
  { MAKECODE('\0','s','r','r'), "Serer" },
  { MAKECODE('\0','s','h','n'), "Shan" },
  { MAKECODE('\0','s','n','a'), "Shona" },
  { MAKECODE('\0','i','i','i'), "Sichuan Yi" },
  { MAKECODE('\0','s','c','n'), "Sicilian" },
  { MAKECODE('\0','s','i','d'), "Sidamo" },
  { MAKECODE('\0','s','g','n'), "Sign languages" },
  { MAKECODE('\0','b','l','a'), "Siksika" },
  { MAKECODE('\0','s','n','d'), "Sindhi" },
  { MAKECODE('\0','s','i','n'), "Sinhala" },
  { MAKECODE('\0','s','i','n'), "Sinhalese" },
  { MAKECODE('\0','s','i','t'), "Sino-Tibetan (Other)" },
  { MAKECODE('\0','s','i','o'), "Siouan languages" },
  { MAKECODE('\0','s','m','s'), "Skolt Sami" },
  { MAKECODE('\0','d','e','n'), "Slave (Athapascan)" },
  { MAKECODE('\0','s','l','a'), "Slavic (Other)" },
  { MAKECODE('\0','s','l','o'), "Slovak" },
  { MAKECODE('\0','s','l','k'), "Slovak" },
  { MAKECODE('\0','s','l','v'), "Slovenian" },
  { MAKECODE('\0','s','o','g'), "Sogdian" },
  { MAKECODE('\0','s','o','m'), "Somali" },
  { MAKECODE('\0','s','o','n'), "Songhai" },
  { MAKECODE('\0','s','n','k'), "Soninke" },
  { MAKECODE('\0','w','e','n'), "Sorbian languages" },
  { MAKECODE('\0','n','s','o'), "Sotho, Northern" },
  { MAKECODE('\0','s','o','t'), "Sotho, Southern" },
  { MAKECODE('\0','s','a','i'), "South American Indian (Other)" },
  { MAKECODE('\0','s','m','a'), "Southern Sami" },
  { MAKECODE('\0','n','b','l'), "South Ndebele" },
  { MAKECODE('\0','s','p','a'), "Castilian" },
  { MAKECODE('\0','s','u','k'), "Sukuma" },
  { MAKECODE('\0','s','u','x'), "Sumerian" },
  { MAKECODE('\0','s','u','n'), "Sundanese" },
  { MAKECODE('\0','s','u','s'), "Susu" },
  { MAKECODE('\0','s','w','a'), "Swahili" },
  { MAKECODE('\0','s','s','w'), "Swati" },
  { MAKECODE('\0','s','w','e'), "Swedish" },
  { MAKECODE('\0','s','y','r'), "Syriac" },
  { MAKECODE('\0','t','g','l'), "Tagalog" },
  { MAKECODE('\0','t','a','h'), "Tahitian" },
  { MAKECODE('\0','t','a','i'), "Tai (Other)" },
  { MAKECODE('\0','t','g','k'), "Tajik" },
  { MAKECODE('\0','t','m','h'), "Tamashek" },
  { MAKECODE('\0','t','a','m'), "Tamil" },
  { MAKECODE('\0','t','a','t'), "Tatar" },
  { MAKECODE('\0','t','e','l'), "Telugu" },
  { MAKECODE('\0','t','e','r'), "Tereno" },
  { MAKECODE('\0','t','e','t'), "Tetum" },
  { MAKECODE('\0','t','h','a'), "Thai" },
  { MAKECODE('\0','t','i','b'), "Tibetan" },
  { MAKECODE('\0','b','o','d'), "Tibetan" },
  { MAKECODE('\0','t','i','g'), "Tigre" },
  { MAKECODE('\0','t','i','r'), "Tigrinya" },
  { MAKECODE('\0','t','e','m'), "Timne" },
  { MAKECODE('\0','t','i','v'), "Tiv" },
  { MAKECODE('\0','t','l','h'), "tlhIngan-Hol" },
  { MAKECODE('\0','t','l','i'), "Tlingit" },
  { MAKECODE('\0','t','p','i'), "Tok Pisin" },
  { MAKECODE('\0','t','k','l'), "Tokelau" },
  { MAKECODE('\0','t','o','g'), "Tonga (Nyasa)" },
  { MAKECODE('\0','t','o','n'), "Tonga (Tonga Islands)" },
  { MAKECODE('\0','t','s','i'), "Tsimshian" },
  { MAKECODE('\0','t','s','o'), "Tsonga" },
  { MAKECODE('\0','t','s','n'), "Tswana" },
  { MAKECODE('\0','t','u','m'), "Tumbuka" },
  { MAKECODE('\0','t','u','p'), "Tupi languages" },
  { MAKECODE('\0','t','u','r'), "Turkish" },
  { MAKECODE('\0','o','t','a'), "Turkish, Ottoman (1500-1928)" },
  { MAKECODE('\0','t','u','k'), "Turkmen" },
  { MAKECODE('\0','t','v','l'), "Tuvalu" },
  { MAKECODE('\0','t','y','v'), "Tuvinian" },
  { MAKECODE('\0','t','w','i'), "Twi" },
  { MAKECODE('\0','u','d','m'), "Udmurt" },
  { MAKECODE('\0','u','g','a'), "Ugaritic" },
  { MAKECODE('\0','u','i','g'), "Uighur" },
  { MAKECODE('\0','u','k','r'), "Ukrainian" },
  { MAKECODE('\0','u','m','b'), "Umbundu" },
  { MAKECODE('\0','u','n','d'), "Undetermined" },
  { MAKECODE('\0','h','s','b'), "Upper Sorbian" },
  { MAKECODE('\0','u','r','d'), "Urdu" },
  { MAKECODE('\0','u','i','g'), "Uyghur" },
  { MAKECODE('\0','u','z','b'), "Uzbek" },
  { MAKECODE('\0','v','a','i'), "Vai" },
  { MAKECODE('\0','c','a','t'), "Valencian" },
  { MAKECODE('\0','v','e','n'), "Venda" },
  { MAKECODE('\0','v','i','e'), "Vietnamese" },
  { MAKECODE('\0','v','o','l'), "Volap\xC3\xBCk" },
  { MAKECODE('\0','v','o','t'), "Votic" },
  { MAKECODE('\0','w','a','k'), "Wakashan languages" },
  { MAKECODE('\0','w','a','l'), "Walamo" },
  { MAKECODE('\0','w','l','n'), "Walloon" },
  { MAKECODE('\0','w','a','r'), "Waray" },
  { MAKECODE('\0','w','a','s'), "Washo" },
  { MAKECODE('\0','w','e','l'), "Welsh" },
  { MAKECODE('\0','c','y','m'), "Welsh" },
  { MAKECODE('\0','w','o','l'), "Wolof" },
  { MAKECODE('\0','x','h','o'), "Xhosa" },
  { MAKECODE('\0','s','a','h'), "Yakut" },
  { MAKECODE('\0','y','a','o'), "Yao" },
  { MAKECODE('\0','y','a','p'), "Yapese" },
  { MAKECODE('\0','y','i','d'), "Yiddish" },
  { MAKECODE('\0','y','o','r'), "Yoruba" },
  { MAKECODE('\0','y','p','k'), "Yupik languages" },
  { MAKECODE('\0','z','n','d'), "Zande" },
  { MAKECODE('\0','z','a','p'), "Zapotec" },
  { MAKECODE('\0','z','e','n'), "Zenaga" },
  { MAKECODE('\0','z','h','a'), "Zhuang" },
  { MAKECODE('\0','z','u','l'), "Zulu" },
  { MAKECODE('\0','z','u','n'), "Zuni" },
};

const ISO639 LanguageCodes[189] =
{
  { "aa", "aar", NULL },
  { "ab", "abk", NULL },
  { "af", "afr", NULL },
  { "ak", "aka", NULL },
  { "am", "amh", NULL },
  { "ar", "ara", NULL },
  { "an", "arg", NULL },
  { "as", "asm", NULL },
  { "av", "ava", NULL },
  { "ae", "ave", NULL },
  { "ay", "aym", NULL },
  { "az", "aze", NULL },
  { "ba", "bak", NULL },
  { "bm", "bam", NULL },
  { "be", "bel", NULL },
  { "bn", "ben", NULL },
  { "bh", "bih", NULL },
  { "bi", "bis", NULL },
  { "bo", "tib", NULL },
  { "bs", "bos", NULL },
  { "br", "bre", NULL },
  { "bg", "bul", NULL },
  { "ca", "cat", NULL },
  { "cs", "cze", "ces" },
  { "ch", "cha", NULL },
  { "ce", "che", NULL },
  { "cu", "chu", NULL },
  { "cv", "chv", NULL },
  { "kw", "cor", NULL },
  { "co", "cos", NULL },
  { "cr", "cre", NULL },
  { "cy", "wel", NULL },
  { "da", "dan", NULL },
  { "de", "ger", "deu" },
  { "dv", "div", NULL },
  { "dz", "dzo", NULL },
  { "el", "gre", "ell" },
  { "en", "eng", NULL },
  { "eo", "epo", NULL },
  { "et", "est", NULL },
  { "eu", "baq", NULL },
  { "ee", "ewe", NULL },
  { "fo", "fao", NULL },
  { "fa", "per", NULL },
  { "fj", "fij", NULL },
  { "fi", "fin", NULL },
  { "fr", "fre", "fra" },
  { "fy", "fry", NULL },
  { "ff", "ful", NULL },
  { "gd", "gla", NULL },
  { "ga", "gle", NULL },
  { "gl", "glg", NULL },
  { "gv", "glv", NULL },
  { "gn", "grn", NULL },
  { "gu", "guj", NULL },
  { "ht", "hat", NULL },
  { "ha", "hau", NULL },
  { "he", "heb", NULL },
  { "hz", "her", NULL },
  { "hi", "hin", NULL },
  { "ho", "hmo", NULL },
  { "hr", "hrv", NULL },
  { "hu", "hun", NULL },
  { "hy", "arm", NULL },
  { "ig", "ibo", NULL },
  { "io", "ido", NULL },
  { "ii", "iii", NULL },
  { "iu", "iku", NULL },
  { "ie", "ile", NULL },
  { "ia", "ina", NULL },
  { "id", "ind", NULL },
  { "ik", "ipk", NULL },
  { "is", "ice", "isl" },
  { "it", "ita", NULL },
  { "jv", "jav", NULL },
  { "ja", "jpn", NULL },
  { "kl", "kal", NULL },
  { "kn", "kan", NULL },
  { "ks", "kas", NULL },
  { "ka", "geo", NULL },
  { "kr", "kau", NULL },
  { "kk", "kaz", NULL },
  { "km", "khm", NULL },
  { "ki", "kik", NULL },
  { "rw", "kin", NULL },
  { "ky", "kir", NULL },
  { "kv", "kom", NULL },
  { "kg", "kon", NULL },
  { "ko", "kor", NULL },
  { "kj", "kua", NULL },
  { "ku", "kur", NULL },
  { "lo", "lao", NULL },
  { "la", "lat", NULL },
  { "lv", "lav", NULL },
  { "li", "lim", NULL },
  { "ln", "lin", NULL },
  { "lt", "lit", NULL },
  { "lb", "ltz", NULL },
  { "lu", "lub", NULL },
  { "lg", "lug", NULL },
  { "mk", "mac", NULL },
  { "mh", "mah", NULL },
  { "ml", "mal", NULL },
  { "mi", "mao", NULL },
  { "mr", "mar", NULL },
  { "ms", "may", NULL },
  { "mg", "mlg", NULL },
  { "mt", "mlt", NULL },
  { "mn", "mon", NULL },
  { "my", "bur", NULL },
  { "na", "nau", NULL },
  { "nv", "nav", NULL },
  { "nr", "nbl", NULL },
  { "nd", "nde", NULL },
  { "ng", "ndo", NULL },
  { "ne", "nep", NULL },
  { "nl", "dut", "nld" },
  { "nn", "nno", NULL },
  { "nb", "nob", NULL },
  { "no", "nor", NULL },
  { "ny", "nya", NULL },
  { "oc", "oci", NULL },
  { "oj", "oji", NULL },
  { "or", "ori", NULL },
  { "om", "orm", NULL },
  { "os", "oss", NULL },
  { "pa", "pan", NULL },
  { "pi", "pli", NULL },
  { "pl", "pol", "plk" },
  { "pt", "por", "ptg" },
  { "ps", "pus", NULL },
  { "qu", "que", NULL },
  { "rm", "roh", NULL },
  { "ro", "rum", "ron" },
  { "rn", "run", NULL },
  { "ru", "rus", NULL },
  { "sh", "scr", NULL },
  { "sg", "sag", NULL },
  { "sa", "san", NULL },
  { "si", "sin", NULL },
  { "sk", "slo", "sky" },
  { "sl", "slv", NULL },
  { "se", "sme", NULL },
  { "sm", "smo", NULL },
  { "sn", "sna", NULL },
  { "sd", "snd", NULL },
  { "so", "som", NULL },
  { "st", "sot", NULL },
  { "es", "spa", "esp" },
  { "sq", "alb", NULL },
  { "sc", "srd", NULL },
  { "sr", "srp", NULL },
  { "ss", "ssw", NULL },
  { "su", "sun", NULL },
  { "sw", "swa", NULL },
  { "sv", "swe", "sve" },
  { "ty", "tah", NULL },
  { "ta", "tam", NULL },
  { "tt", "tat", NULL },
  { "te", "tel", NULL },
  { "tg", "tgk", NULL },
  { "tl", "tgl", NULL },
  { "th", "tha", NULL },
  { "ti", "tir", NULL },
  { "to", "ton", NULL },
  { "tn", "tsn", NULL },
  { "ts", "tso", NULL },
  { "tk", "tuk", NULL },
  { "tr", "tur", "trk" },
  { "tw", "twi", NULL },
  { "ug", "uig", NULL },
  { "uk", "ukr", NULL },
  { "ur", "urd", NULL },
  { "uz", "uzb", NULL },
  { "ve", "ven", NULL },
  { "vi", "vie", NULL },
  { "vo", "vol", NULL },
  { "wa", "wln", NULL },
  { "wo", "wol", NULL },
  { "xh", "xho", NULL },
  { "yi", "yid", NULL },
  { "yo", "yor", NULL },
  { "za", "zha", NULL },
  { "zh", "chi", "zho" },
  { "zu", "zul", NULL },
  { "zv", "und", NULL }, // Kodi intern mapping for missing "Undetermined" iso639-1 code
  { "zx", "zxx", NULL }, // Kodi intern mapping for missing "No linguistic content" iso639-1 code
  { "zy", "mis", NULL }, // Kodi intern mapping for missing "Miscellaneous languages" iso639-1 code
  { "zz", "mul", NULL }  // Kodi intern mapping for missing "Multiple languages" iso639-1 code
};

// Based on ISO 3166
const ISO3166_1 RegionCodes[246] =
{
  { "af", "afg" },
  { "ax", "ala" },
  { "al", "alb" },
  { "dz", "dza" },
  { "as", "asm" },
  { "ad", "and" },
  { "ao", "ago" },
  { "ai", "aia" },
  { "aq", "ata" },
  { "ag", "atg" },
  { "ar", "arg" },
  { "am", "arm" },
  { "aw", "abw" },
  { "au", "aus" },
  { "at", "aut" },
  { "az", "aze" },
  { "bs", "bhs" },
  { "bh", "bhr" },
  { "bd", "bgd" },
  { "bb", "brb" },
  { "by", "blr" },
  { "be", "bel" },
  { "bz", "blz" },
  { "bj", "ben" },
  { "bm", "bmu" },
  { "bt", "btn" },
  { "bo", "bol" },
  { "ba", "bih" },
  { "bw", "bwa" },
  { "bv", "bvt" },
  { "br", "bra" },
  { "io", "iot" },
  { "bn", "brn" },
  { "bg", "bgr" },
  { "bf", "bfa" },
  { "bi", "bdi" },
  { "kh", "khm" },
  { "cm", "cmr" },
  { "ca", "can" },
  { "cv", "cpv" },
  { "ky", "cym" },
  { "cf", "caf" },
  { "td", "tcd" },
  { "cl", "chl" },
  { "cn", "chn" },
  { "cx", "cxr" },
  { "cc", "cck" },
  { "co", "col" },
  { "km", "com" },
  { "cg", "cog" },
  { "cd", "cod" },
  { "ck", "cok" },
  { "cr", "cri" },
  { "ci", "civ" },
  { "hr", "hrv" },
  { "cu", "cub" },
  { "cy", "cyp" },
  { "cz", "cze" },
  { "dk", "dnk" },
  { "dj", "dji" },
  { "dm", "dma" },
  { "do", "dom" },
  { "ec", "ecu" },
  { "eg", "egy" },
  { "sv", "slv" },
  { "gq", "gnq" },
  { "er", "eri" },
  { "ee", "est" },
  { "et", "eth" },
  { "fk", "flk" },
  { "fo", "fro" },
  { "fj", "fji" },
  { "fi", "fin" },
  { "fr", "fra" },
  { "gf", "guf" },
  { "pf", "pyf" },
  { "tf", "atf" },
  { "ga", "gab" },
  { "gm", "gmb" },
  { "ge", "geo" },
  { "de", "deu" },
  { "gh", "gha" },
  { "gi", "gib" },
  { "gr", "grc" },
  { "gl", "grl" },
  { "gd", "grd" },
  { "gp", "glp" },
  { "gu", "gum" },
  { "gt", "gtm" },
  { "gg", "ggy" },
  { "gn", "gin" },
  { "gw", "gnb" },
  { "gy", "guy" },
  { "ht", "hti" },
  { "hm", "hmd" },
  { "va", "vat" },
  { "hn", "hnd" },
  { "hk", "hkg" },
  { "hu", "hun" },
  { "is", "isl" },
  { "in", "ind" },
  { "id", "idn" },
  { "ir", "irn" },
  { "iq", "irq" },
  { "ie", "irl" },
  { "im", "imn" },
  { "il", "isr" },
  { "it", "ita" },
  { "jm", "jam" },
  { "jp", "jpn" },
  { "je", "jey" },
  { "jo", "jor" },
  { "kz", "kaz" },
  { "ke", "ken" },
  { "ki", "kir" },
  { "kp", "prk" },
  { "kr", "kor" },
  { "kw", "kwt" },
  { "kg", "kgz" },
  { "la", "lao" },
  { "lv", "lva" },
  { "lb", "lbn" },
  { "ls", "lso" },
  { "lr", "lbr" },
  { "ly", "lby" },
  { "li", "lie" },
  { "lt", "ltu" },
  { "lu", "lux" },
  { "mo", "mac" },
  { "mk", "mkd" },
  { "mg", "mdg" },
  { "mw", "mwi" },
  { "my", "mys" },
  { "mv", "mdv" },
  { "ml", "mli" },
  { "mt", "mlt" },
  { "mh", "mhl" },
  { "mq", "mtq" },
  { "mr", "mrt" },
  { "mu", "mus" },
  { "yt", "myt" },
  { "mx", "mex" },
  { "fm", "fsm" },
  { "md", "mda" },
  { "mc", "mco" },
  { "mn", "mng" },
  { "me", "mne" },
  { "ms", "msr" },
  { "ma", "mar" },
  { "mz", "moz" },
  { "mm", "mmr" },
  { "na", "nam" },
  { "nr", "nru" },
  { "np", "npl" },
  { "nl", "nld" },
  { "an", "ant" },
  { "nc", "ncl" },
  { "nz", "nzl" },
  { "ni", "nic" },
  { "ne", "ner" },
  { "ng", "nga" },
  { "nu", "niu" },
  { "nf", "nfk" },
  { "mp", "mnp" },
  { "no", "nor" },
  { "om", "omn" },
  { "pk", "pak" },
  { "pw", "plw" },
  { "ps", "pse" },
  { "pa", "pan" },
  { "pg", "png" },
  { "py", "pry" },
  { "pe", "per" },
  { "ph", "phl" },
  { "pn", "pcn" },
  { "pl", "pol" },
  { "pt", "prt" },
  { "pr", "pri" },
  { "qa", "qat" },
  { "re", "reu" },
  { "ro", "rou" },
  { "ru", "rus" },
  { "rw", "rwa" },
  { "bl", "blm" },
  { "sh", "shn" },
  { "kn", "kna" },
  { "lc", "lca" },
  { "mf", "maf" },
  { "pm", "spm" },
  { "vc", "vct" },
  { "ws", "wsm" },
  { "sm", "smr" },
  { "st", "stp" },
  { "sa", "sau" },
  { "sn", "sen" },
  { "rs", "srb" },
  { "sc", "syc" },
  { "sl", "sle" },
  { "sg", "sgp" },
  { "sk", "svk" },
  { "si", "svn" },
  { "sb", "slb" },
  { "so", "som" },
  { "za", "zaf" },
  { "gs", "sgs" },
  { "es", "esp" },
  { "lk", "lka" },
  { "sd", "sdn" },
  { "sr", "sur" },
  { "sj", "sjm" },
  { "sz", "swz" },
  { "se", "swe" },
  { "ch", "che" },
  { "sy", "syr" },
  { "tw", "twn" },
  { "tj", "tjk" },
  { "tz", "tza" },
  { "th", "tha" },
  { "tl", "tls" },
  { "tg", "tgo" },
  { "tk", "tkl" },
  { "to", "ton" },
  { "tt", "tto" },
  { "tn", "tun" },
  { "tr", "tur" },
  { "tm", "tkm" },
  { "tc", "tca" },
  { "tv", "tuv" },
  { "ug", "uga" },
  { "ua", "ukr" },
  { "ae", "are" },
  { "gb", "gbr" },
  { "us", "usa" },
  { "um", "umi" },
  { "uy", "ury" },
  { "uz", "uzb" },
  { "vu", "vut" },
  { "ve", "ven" },
  { "vn", "vnm" },
  { "vg", "vgb" },
  { "vi", "vir" },
  { "wf", "wlf" },
  { "eh", "esh" },
  { "ye", "yem" },
  { "zm", "zmb" },
  { "zw", "zwe" }
};
