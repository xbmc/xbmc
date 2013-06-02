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
#include "utils/XBMCTinyXML.h"
#include "LangInfo.h"
#include "utils/log.h" 

#define MAKECODE(a, b, c, d)  ((((long)(a))<<24) | (((long)(b))<<16) | (((long)(c))<<8) | (long)(d))
#define MAKETWOCHARCODE(a, b) ((((long)(a))<<8) | (long)(b)) 

typedef struct LCENTRY
{
  long code;
  const char *name;
} LCENTRY;

extern const struct LCENTRY g_iso639_1[144];
extern const struct LCENTRY g_iso639_2[537];

struct CharCodeConvertionWithHack
{
  const char* old;
  const char* id;
  const char* win_id;
};

struct CharCodeConvertion
{
  const char* old;
  const char* id;
};

// declared as extern to allow forward declaration
extern const CharCodeConvertionWithHack CharCode2To3[184];
extern const CharCodeConvertion RegionCode2To3[246];

#ifdef TARGET_WINDOWS
typedef struct _WINLCIDENTRY
{
  const LCID LcId;
  const char* LanguageTag;
} WINLCIDENTRY;

extern const WINLCIDENTRY WinLCIDtoLangTag[351];
#endif

CLangCodeExpander::CLangCodeExpander(void)
{}

CLangCodeExpander::~CLangCodeExpander(void)
{
}

void CLangCodeExpander::Clear()
{
  m_mapUser.clear();
}

void CLangCodeExpander::LoadUserCodes(const TiXmlElement* pRootElement)
{
  if (pRootElement)
  {
    m_mapUser.clear();

    CStdString sShort, sLong;

    const TiXmlNode* pLangCode = pRootElement->FirstChild("code");
    while (pLangCode)
    {
      const TiXmlNode* pShort = pLangCode->FirstChildElement("short");
      const TiXmlNode* pLong = pLangCode->FirstChildElement("long");
      if (pShort && pLong)
      {
        sShort = pShort->FirstChild()->Value();
        sLong = pLong->FirstChild()->Value();
        sShort.ToLower();
        m_mapUser[sShort] = sLong;
      }
      pLangCode = pLangCode->NextSibling();
    }
  }
}

bool CLangCodeExpander::Lookup(CStdString& desc, const CStdString& code)
{
  int iSplit = code.find("-");
  if (iSplit > 0)
  {
    CStdString strLeft, strRight;
    const bool bLeft = Lookup(strLeft, code.Left(iSplit));
    const bool bRight = Lookup(strRight, code.Mid(iSplit + 1));
    if (bLeft || bRight)
    {
      desc = "";
      if (strLeft.length() > 0)
        desc = strLeft;
      else
        desc = code.Left(iSplit);

      if (strRight.length() > 0)
      {
        desc += " - ";
        desc += strRight;
      }
      else
      {
        desc += " - ";
        desc += code.Mid(iSplit + 1);
      }
      return true;
    }
    return false;
  }
  else
  {
    if( LookupInMap(desc, code) )
      return true;

    if( LookupInDb(desc, code) )
      return true;
  }
  return false;
}

bool CLangCodeExpander::Lookup(CStdString& desc, const int code)
{

  char lang[3];
  lang[2] = 0;
  lang[1] = (code & 255);
  lang[0] = (code >> 8) & 255;

  return Lookup(desc, lang);
}

#ifdef TARGET_WINDOWS
bool CLangCodeExpander::ConvertTwoToThreeCharCode(CStdString& strThreeCharCode, const CStdString& strTwoCharCode, bool localeHack /*= false*/)
#else
bool CLangCodeExpander::ConvertTwoToThreeCharCode(CStdString& strThreeCharCode, const CStdString& strTwoCharCode)
#endif
{       
  if ( strTwoCharCode.length() == 2 )
  {
    CStdString strTwoCharCodeLower( strTwoCharCode );
    strTwoCharCodeLower.MakeLower();
    strTwoCharCodeLower.TrimLeft();
    strTwoCharCodeLower.TrimRight();

    for (unsigned int index = 0; index < sizeof(CharCode2To3) / sizeof(CharCode2To3[0]); ++index)
    {
      if (strTwoCharCodeLower.Equals(CharCode2To3[index].old))
      {
#ifdef TARGET_WINDOWS
        if (localeHack && CharCode2To3[index].win_id)
        {
          strThreeCharCode = CharCode2To3[index].win_id;
          return true;
        }
#endif
        strThreeCharCode = CharCode2To3[index].id;
        return true;
      }
    }
  }

  // not a 2 char code
  return false;
}

#ifdef TARGET_WINDOWS
bool CLangCodeExpander::ConvertToThreeCharCode(CStdString& strThreeCharCode, const CStdString& strCharCode, bool localeHack /*= false*/)
#else
bool CLangCodeExpander::ConvertToThreeCharCode(CStdString& strThreeCharCode, const CStdString& strCharCode)
#endif
{
  if (strCharCode.size() == 2)
#ifdef TARGET_WINDOWS
    return g_LangCodeExpander.ConvertTwoToThreeCharCode(strThreeCharCode, strCharCode, localeHack);
#else
    return g_LangCodeExpander.ConvertTwoToThreeCharCode(strThreeCharCode, strCharCode);
#endif
  else if (strCharCode.size() == 3)
  {
    for (unsigned int index = 0; index < sizeof(CharCode2To3) / sizeof(CharCode2To3[0]); ++index)
    {
#ifdef TARGET_WINDOWS
      if (strCharCode.Equals(CharCode2To3[index].id) ||
         (localeHack && CharCode2To3[index].win_id != NULL && strCharCode.Equals(CharCode2To3[index].win_id)))
#else
      if (strCharCode.Equals(CharCode2To3[index].id))
#endif
      {
        strThreeCharCode = strCharCode;
        return true;
      }
    }
    for (unsigned int index = 0; index < sizeof(RegionCode2To3) / sizeof(RegionCode2To3[0]); ++index)
    {
      if (strCharCode.Equals(RegionCode2To3[index].id))
      {
        strThreeCharCode = strCharCode;
        return true;
      }
    }
  }
  else if (strCharCode.size() > 3)
  {
    CStdString strLangInfoPath;
    strLangInfoPath.Format("special://xbmc/language/%s/langinfo.xml", strCharCode.c_str());
    CLangInfo langInfo;
    if (!langInfo.Load(strLangInfoPath))
      return false;

    strThreeCharCode = langInfo.GetLanguageCode();
    return true;
  }

  return false;
}

#ifdef TARGET_WINDOWS
bool CLangCodeExpander::ConvertLinuxToWindowsRegionCodes(const CStdString& strTwoCharCode, CStdString& strThreeCharCode)
{
  if (strTwoCharCode.length() != 2)
    return false;

  CStdString strLower( strTwoCharCode );
  strLower.MakeLower();
  strLower.TrimLeft();
  strLower.TrimRight();
  for (unsigned int index = 0; index < sizeof(RegionCode2To3) / sizeof(RegionCode2To3[0]); ++index)
  {
    if (strLower.Equals(RegionCode2To3[index].old))
    {
      strThreeCharCode = RegionCode2To3[index].id;
      return true;
    }
  }

  return true;
}

bool CLangCodeExpander::ConvertWindowsToGeneralCharCode(const CStdString& strWindowsCharCode, CStdString& strThreeCharCode)
{
  if (strWindowsCharCode.length() != 3)
    return false;

  CStdString strLower(strWindowsCharCode);
  strLower.MakeLower();
  for (unsigned int index = 0; index < sizeof(CharCode2To3) / sizeof(CharCode2To3[0]); ++index)
  {
    if ((CharCode2To3[index].win_id && strLower.Equals(CharCode2To3[index].win_id)) ||
         strLower.Equals(CharCode2To3[index].id))
    {
      strThreeCharCode = CharCode2To3[index].id;
      return true;
    }
  }

  return true;
}

bool CLangCodeExpander::ConvertWindowsLCIDtoLanguageTag(LCID id, std::string& languageTag)
{
  if (id == LOCALE_USER_DEFAULT || id == LOCALE_SYSTEM_DEFAULT || id == LOCALE_INVARIANT)
    id = ConvertDefaultLocale(id);

  for (int i = 0; i < RTL_NUMBER_OF_V1(WinLCIDtoLangTag); ++i)
  {
    if (WinLCIDtoLangTag[i].LcId == id)
    {
      languageTag = WinLCIDtoLangTag[i].LanguageTag;
      return true;
    }
  }

  languageTag = "";
  return false;
}
#endif

bool CLangCodeExpander::ConvertToTwoCharCode(CStdString& code, const CStdString& lang)
{
  if (lang.length() == 2)
  {
    CStdString tmp;
    if (Lookup(tmp, lang))
    {
      code = lang;
      return true;
    }
  }
  else if (lang.length() == 3)
  {
    for (unsigned int index = 0; index < sizeof(CharCode2To3) / sizeof(CharCode2To3[0]); ++index)
    {
      if (lang.Equals(CharCode2To3[index].id) || (CharCode2To3[index].win_id && lang.Equals(CharCode2To3[index].win_id)))
      {
        code = CharCode2To3[index].old;
        return true;
      }
    }

    for (unsigned int index = 0; index < sizeof(RegionCode2To3) / sizeof(RegionCode2To3[0]); ++index)
    {
      if (lang.Equals(RegionCode2To3[index].id))
      {
        code = RegionCode2To3[index].old;
        return true;
      }
    }
  }

  // check if lang is full language name
  CStdString tmp;
  if (ReverseLookup(lang, tmp))
  {
    if (tmp.length() == 2)
    {
      code = tmp;
      return true;
    }
    else if (tmp.length() == 3)
      return ConvertToTwoCharCode(code, tmp);
  }

  // try xbmc specific language names
  CStdString strLangInfoPath;
  strLangInfoPath.Format("special://xbmc/language/%s/langinfo.xml", lang.c_str());
  CLangInfo langInfo;
  if (!langInfo.Load(strLangInfoPath))
    return false;

  return ConvertToTwoCharCode(code, langInfo.GetLanguageCode());
}

bool CLangCodeExpander::ReverseLookup(const CStdString& desc, CStdString& code)
{
  CStdString descTmp(desc);
  descTmp.Trim();
  STRINGLOOKUPTABLE::iterator it;
  for (it = m_mapUser.begin(); it != m_mapUser.end() ; it++)
  {
    if (descTmp.Equals(it->second))
    {
      code = it->first;
      return true;
    }
  }
  for(unsigned int i = 0; i < sizeof(g_iso639_1) / sizeof(LCENTRY); i++)
  {
    if (descTmp.Equals(g_iso639_1[i].name))
    {
      CodeToString(g_iso639_1[i].code, code);
      return true;
    }
  }
  for(unsigned int i = 0; i < sizeof(g_iso639_2) / sizeof(LCENTRY); i++)
  {
    if (descTmp.Equals(g_iso639_2[i].name))
    {
      CodeToString(g_iso639_2[i].code, code);
      return true;
    }
  }
  return false;
}

bool CLangCodeExpander::LookupInMap(CStdString& desc, const CStdString& code)
{
  STRINGLOOKUPTABLE::iterator it;
  //Make sure we convert to lowercase before trying to find it
  CStdString sCode(code);
  sCode.MakeLower();
  sCode.TrimLeft();
  sCode.TrimRight();
  it = m_mapUser.find(sCode);
  if (it != m_mapUser.end())
  {
    desc = it->second;
    return true;
  }
  return false;
}

bool CLangCodeExpander::LookupInDb(std::string& desc, const std::string& code)
{
  long longcode;
  CStdString sCode(code);
  sCode.MakeLower();
  sCode.TrimLeft();
  sCode.TrimRight();
  if(sCode.length() == 2)
  {
    longcode = MAKECODE('\0', '\0', sCode[0], sCode[1]);
    for(unsigned int i = 0; i < sizeof(g_iso639_1) / sizeof(LCENTRY); i++)
    {
      if(g_iso639_1[i].code == longcode)
      {
        desc = g_iso639_1[i].name;
        return true;
      }
    }
  }
  else if(code.length() == 3)
  {
    longcode = MAKECODE('\0', sCode[0], sCode[1], sCode[2]);
    for(unsigned int i = 0; i < sizeof(g_iso639_2) / sizeof(LCENTRY); i++)
    {
      if(g_iso639_2[i].code == longcode)
      {
        desc = g_iso639_2[i].name;
        return true;
      }
    }
  }
  return false;
}

void CLangCodeExpander::CodeToString(long code, CStdString& ret)
{
  ret.Empty();
  for (unsigned int j = 0 ; j < 4 ; j++)
  {
    char c = (char) code & 0xFF;
    if (c == '\0')
      return;
    ret.Insert(0, c);
    code >>= 8;
  }
}

std::vector<std::string> CLangCodeExpander::GetLanguageNames(LANGFORMATS format /* = CLangCodeExpander::ISO_639_1 */) const
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

  return languages;
}

extern const LCENTRY g_iso639_1[144] =
{
  { MAKECODE('\0','\0','c','c'), "Closed Caption" },
  { MAKECODE('\0','\0','a','a'), "Afar" },
  { MAKECODE('\0','\0','a','b'), "Abkhaz" },
  { MAKECODE('\0','\0','a','b'), "Abkhazian" },
  { MAKECODE('\0','\0','a','f'), "Afrikaans" },
  { MAKECODE('\0','\0','a','m'), "Amharic" },
  { MAKECODE('\0','\0','a','r'), "Arabic" },
  { MAKECODE('\0','\0','a','s'), "Assamese" },
  { MAKECODE('\0','\0','a','y'), "Aymara" },
  { MAKECODE('\0','\0','a','z'), "Azerbaijani" },
  { MAKECODE('\0','\0','b','a'), "Bashkir" },
  { MAKECODE('\0','\0','b','e'), "Byelorussian" },
  { MAKECODE('\0','\0','b','g'), "Bulgarian" },
  { MAKECODE('\0','\0','b','h'), "Bihari" },
  { MAKECODE('\0','\0','b','i'), "Bislama" },
  { MAKECODE('\0','\0','b','n'), "Bengali; Bangla" },
  { MAKECODE('\0','\0','b','o'), "Tibetan" },
  { MAKECODE('\0','\0','b','r'), "Breton" },
  { MAKECODE('\0','\0','c','a'), "Catalan" },
  { MAKECODE('\0','\0','c','o'), "Corsican" },
  { MAKECODE('\0','\0','c','s'), "Czech" },
  { MAKECODE('\0','\0','c','y'), "Welsh" },
  { MAKECODE('\0','\0','d','a'), "Dansk" },
  { MAKECODE('\0','\0','d','e'), "German" },
  { MAKECODE('\0','\0','d','z'), "Bhutani" },
  { MAKECODE('\0','\0','e','l'), "Greek" },
  { MAKECODE('\0','\0','e','n'), "English" },
  { MAKECODE('\0','\0','e','o'), "Esperanto" },
  { MAKECODE('\0','\0','e','s'), "Espanol" },
  { MAKECODE('\0','\0','e','t'), "Estonian" },
  { MAKECODE('\0','\0','e','u'), "Basque" },
  { MAKECODE('\0','\0','f','a'), "Persian" },
  { MAKECODE('\0','\0','f','i'), "Finnish" },
  { MAKECODE('\0','\0','f','j'), "Fiji" },
  { MAKECODE('\0','\0','f','o'), "Faroese" },
  { MAKECODE('\0','\0','f','r'), "Francais" },
  { MAKECODE('\0','\0','f','y'), "Frisian" },
  { MAKECODE('\0','\0','g','a'), "Irish" },
  { MAKECODE('\0','\0','g','d'), "Scots Gaelic" },
  { MAKECODE('\0','\0','g','l'), "Galician" },
  { MAKECODE('\0','\0','g','n'), "Guarani" },
  { MAKECODE('\0','\0','g','u'), "Gujarati" },
  { MAKECODE('\0','\0','h','a'), "Hausa" },
  { MAKECODE('\0','\0','h','e'), "Hebrew" },
  { MAKECODE('\0','\0','h','i'), "Hindi" },
  { MAKECODE('\0','\0','h','r'), "Hrvatski" },
  { MAKECODE('\0','\0','h','u'), "Hungarian" },
  { MAKECODE('\0','\0','h','y'), "Armenian" },
  { MAKECODE('\0','\0','i','a'), "Interlingua" },
  { MAKECODE('\0','\0','i','d'), "Indonesian" },
  { MAKECODE('\0','\0','i','e'), "Interlingue" },
  { MAKECODE('\0','\0','i','k'), "Inupiak" },
  { MAKECODE('\0','\0','i','n'), "Indonesian" },
  { MAKECODE('\0','\0','i','s'), "Islenska" },
  { MAKECODE('\0','\0','i','t'), "Italiano" },
  { MAKECODE('\0','\0','i','u'), "Inuktitut" },
  { MAKECODE('\0','\0','i','w'), "Hebrew" },
  { MAKECODE('\0','\0','j','a'), "Japanese" },
  { MAKECODE('\0','\0','j','i'), "Yiddish" },
  { MAKECODE('\0','\0','j','w'), "Javanese" },
  { MAKECODE('\0','\0','k','a'), "Georgian" },
  { MAKECODE('\0','\0','k','k'), "Kazakh" },
  { MAKECODE('\0','\0','k','l'), "Greenlandic" },
  { MAKECODE('\0','\0','k','m'), "Cambodian" },
  { MAKECODE('\0','\0','k','n'), "Kannada" },
  { MAKECODE('\0','\0','k','o'), "Korean" },
  { MAKECODE('\0','\0','k','s'), "Kashmiri" },
  { MAKECODE('\0','\0','k','u'), "Kurdish" },
  { MAKECODE('\0','\0','k','y'), "Kirghiz" },
  { MAKECODE('\0','\0','l','a'), "Latin" },
  { MAKECODE('\0','\0','l','n'), "Lingala" },
  { MAKECODE('\0','\0','l','o'), "Laothian" },
  { MAKECODE('\0','\0','l','t'), "Lithuanian" },
  { MAKECODE('\0','\0','l','v'), "Latvian, Lettish" },
  { MAKECODE('\0','\0','m','g'), "Malagasy" },
  { MAKECODE('\0','\0','m','i'), "Maori" },
  { MAKECODE('\0','\0','m','k'), "Macedonian" },
  { MAKECODE('\0','\0','m','l'), "Malayalam" },
  { MAKECODE('\0','\0','m','n'), "Mongolian" },
  { MAKECODE('\0','\0','m','o'), "Moldavian" },
  { MAKECODE('\0','\0','m','r'), "Marathi" },
  { MAKECODE('\0','\0','m','s'), "Malay" },
  { MAKECODE('\0','\0','m','t'), "Maltese" },
  { MAKECODE('\0','\0','m','y'), "Burmese" },
  { MAKECODE('\0','\0','n','a'), "Nauru" },
  { MAKECODE('\0','\0','n','e'), "Nepali" },
  { MAKECODE('\0','\0','n','l'), "Nederlands" },
  { MAKECODE('\0','\0','n','o'), "Norsk" },
  { MAKECODE('\0','\0','o','c'), "Occitan" },
  { MAKECODE('\0','\0','o','m'), "(Afan) Oromo" },
  { MAKECODE('\0','\0','o','r'), "Oriya" },
  { MAKECODE('\0','\0','p','a'), "Punjabi" },
  { MAKECODE('\0','\0','p','l'), "Polish" },
  { MAKECODE('\0','\0','p','s'), "Pashto, Pushto" },
  { MAKECODE('\0','\0','p','t'), "Portuguese" },
  { MAKECODE('\0','\0','q','u'), "Quechua" },
  { MAKECODE('\0','\0','r','m'), "Rhaeto-Romance" },
  { MAKECODE('\0','\0','r','n'), "Kirundi" },
  { MAKECODE('\0','\0','r','o'), "Romanian" },
  { MAKECODE('\0','\0','r','u'), "Russian" },
  { MAKECODE('\0','\0','r','w'), "Kinyarwanda" },
  { MAKECODE('\0','\0','s','a'), "Sanskrit" },
  { MAKECODE('\0','\0','s','d'), "Sindhi" },
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
  { MAKECODE('\0','\0','s','s'), "Siswati" },
  { MAKECODE('\0','\0','s','t'), "Sesotho" },
  { MAKECODE('\0','\0','s','u'), "Sundanese" },
  { MAKECODE('\0','\0','s','v'), "Svenska" },
  { MAKECODE('\0','\0','s','w'), "Swahili" },
  { MAKECODE('\0','\0','t','a'), "Tamil" },
  { MAKECODE('\0','\0','t','e'), "Telugu" },
  { MAKECODE('\0','\0','t','g'), "Tajik" },
  { MAKECODE('\0','\0','t','h'), "Thai" },
  { MAKECODE('\0','\0','t','i'), "Tigrinya" },
  { MAKECODE('\0','\0','t','k'), "Turkmen" },
  { MAKECODE('\0','\0','t','l'), "Tagalog" },
  { MAKECODE('\0','\0','t','n'), "Setswana" },
  { MAKECODE('\0','\0','t','o'), "Tonga" },
  { MAKECODE('\0','\0','t','r'), "Turkish" },
  { MAKECODE('\0','\0','t','s'), "Tsonga" },
  { MAKECODE('\0','\0','t','t'), "Tatar" },
  { MAKECODE('\0','\0','t','w'), "Twi" },
  { MAKECODE('\0','\0','u','g'), "Uighur" },
  { MAKECODE('\0','\0','u','k'), "Ukrainian" },
  { MAKECODE('\0','\0','u','r'), "Urdu" },
  { MAKECODE('\0','\0','u','z'), "Uzbek" },
  { MAKECODE('\0','\0','v','i'), "Vietnamese" },
  { MAKECODE('\0','\0','v','o'), "Volapuk" },
  { MAKECODE('\0','\0','w','o'), "Wolof" },
  { MAKECODE('\0','\0','x','h'), "Xhosa" },
  { MAKECODE('\0','\0','y','i'), "Yiddish" },
  { MAKECODE('\0','\0','y','o'), "Yoruba" },
  { MAKECODE('\0','\0','z','a'), "Zhuang" },
  { MAKECODE('\0','\0','z','h'), "Chinese" },
  { MAKECODE('\0','\0','z','u'), "Zulu" },
};

extern const LCENTRY g_iso639_2[537] =
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
  { MAKECODE('\0','n','o','b'), "Bokmål, Norwegian" },
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
  { MAKECODE('\0','s','p','a'), "Castilian" },
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
  { MAKECODE('\0','s','c','r'), "Croatian" },
  { MAKECODE('\0','h','r','v'), "Croatian" },
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
  { MAKECODE('\0','g','w','i'), "Gwich´in" },
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
  { MAKECODE('\0','n','o','g'), "Nogai" },
  { MAKECODE('\0','n','o','n'), "Norse, Old" },
  { MAKECODE('\0','n','a','i'), "North American Indian (Other)" },
  { MAKECODE('\0','s','m','e'), "Northern Sami" },
  { MAKECODE('\0','n','s','o'), "Northern Sotho" },
  { MAKECODE('\0','n','d','e'), "North Ndebele" },
  { MAKECODE('\0','n','o','r'), "Norwegian" },
  { MAKECODE('\0','n','o','b'), "Norwegian Bokmål" },
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
  { MAKECODE('\0','o','c','i'), "Provençal" },
  { MAKECODE('\0','p','r','o'), "Provençal, Old (to 1500)" },
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
  { MAKECODE('\0','s','p','a'), "Spanish" },
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
  { MAKECODE('\0','v','o','l'), "Volapük" },
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

const CharCodeConvertionWithHack CharCode2To3[184] =
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
  { "cs", "cze", "csy" },
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
  { "zh", "chi", NULL },
  { "zu", "zul", NULL }
};

// Based on ISO 3166
const CharCodeConvertion RegionCode2To3[246] =
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

#ifdef TARGET_WINDOWS
// See http://msdn.microsoft.com/goglobal/bb896001.aspx
static const WINLCIDENTRY WinLCIDtoLangTag[351] = 
{
  { 0x0036, "af" },
  { 0x0436, "af-ZA" },
  { 0x001C, "sq" },
  { 0x041C, "sq-AL" },
  { 0x0084, "gsw" },
  { 0x0484, "gsw-FR" },
  { 0x005E, "am" },
  { 0x045E, "am-ET" },
  { 0x0001, "ar" },
  { 0x1401, "ar-DZ" },
  { 0x3C01, "ar-BH" },
  { 0x0C01, "ar-EG" },
  { 0x0801, "ar-IQ" },
  { 0x2C01, "ar-JO" },
  { 0x3401, "ar-KW" },
  { 0x3001, "ar-LB" },
  { 0x1001, "ar-LY" },
  { 0x1801, "ar-MA" },
  { 0x2001, "ar-OM" },
  { 0x4001, "ar-QA" },
  { 0x0401, "ar-SA" },
  { 0x2801, "ar-SY" },
  { 0x1C01, "ar-TN" },
  { 0x3801, "ar-AE" },
  { 0x2401, "ar-YE" },
  { 0x002B, "hy" },
  { 0x042B, "hy-AM" },
  { 0x004D, "as" },
  { 0x044D, "as-IN" },
  { 0x002C, "az" },
  { 0x742C, "az-Cyrl" },
  { 0x082C, "az-Cyrl-AZ" },
  { 0x782C, "az-Latn" },
  { 0x042C, "az-Latn-AZ" },
  { 0x006D, "ba" },
  { 0x046D, "ba-RU" },
  { 0x002D, "eu" },
  { 0x042D, "eu-ES" },
  { 0x0023, "be" },
  { 0x0423, "be-BY" },
  { 0x0045, "bn" },
  { 0x0845, "bn-BD" },
  { 0x0445, "bn-IN" },
  { 0x781A, "bs" },
  { 0x641A, "bs-Cyrl" },
  { 0x201A, "bs-Cyrl-BA" },
  { 0x681A, "bs-Latn" },
  { 0x141A, "bs-Latn-BA" },
  { 0x007E, "br" },
  { 0x047E, "br-FR" },
  { 0x0002, "bg" },
  { 0x0402, "bg-BG" },
  { 0x0003, "ca" },
  { 0x0403, "ca-ES" },
  { 0x7804, "zh" },
  { 0x0004, "zh-Hans" },
  { 0x0804, "zh-CN" },
  { 0x1004, "zh-SG" },
  { 0x7C04, "zh-Hant" },
  { 0x0C04, "zh-HK" },
  { 0x1404, "zh-MO" },
  { 0x0404, "zh-TW" },
  { 0x0083, "co" },
  { 0x0483, "co-FR" },
  { 0x001A, "hr" },
  { 0x041A, "hr-HR" },
  { 0x101A, "hr-BA" },
  { 0x0005, "cs" },
  { 0x0405, "cs-CZ" },
  { 0x0006, "da" },
  { 0x0406, "da-DK" },
  { 0x008C, "prs" },
  { 0x048C, "prs-AF" },
  { 0x0065, "dv" },
  { 0x0465, "dv-MV" },
  { 0x0013, "nl" },
  { 0x0813, "nl-BE" },
  { 0x0413, "nl-NL" },
  { 0x0009, "en" },
  { 0x0C09, "en-AU" },
  { 0x2809, "en-BZ" },
  { 0x1009, "en-CA" },
  { 0x2409, "en-029" },
  { 0x4009, "en-IN" },
  { 0x1809, "en-IE" },
  { 0x2009, "en-JM" },
  { 0x4409, "en-MY" },
  { 0x1409, "en-NZ" },
  { 0x3409, "en-PH" },
  { 0x4809, "en-SG" },
  { 0x1C09, "en-ZA" },
  { 0x2C09, "en-TT" },
  { 0x0809, "en-GB" },
  { 0x0409, "en-US" },
  { 0x3009, "en-ZW" },
  { 0x0025, "et" },
  { 0x0425, "et-EE" },
  { 0x0038, "fo" },
  { 0x0438, "fo-FO" },
  { 0x0064, "fil" },
  { 0x0464, "fil-PH" },
  { 0x000B, "fi" },
  { 0x040B, "fi-FI" },
  { 0x000C, "fr" },
  { 0x080C, "fr-BE" },
  { 0x0C0C, "fr-CA" },
  { 0x040C, "fr-FR" },
  { 0x140C, "fr-LU" },
  { 0x180C, "fr-MC" },
  { 0x100C, "fr-CH" },
  { 0x0062, "fy" },
  { 0x0462, "fy-NL" },
  { 0x0056, "gl" },
  { 0x0456, "gl-ES" },
  { 0x0037, "ka" },
  { 0x0437, "ka-GE" },
  { 0x0007, "de" },
  { 0x0C07, "de-AT" },
  { 0x0407, "de-DE" },
  { 0x1407, "de-LI" },
  { 0x1007, "de-LU" },
  { 0x0807, "de-CH" },
  { 0x0008, "el" },
  { 0x0408, "el-GR" },
  { 0x006F, "kl" },
  { 0x046F, "kl-GL" },
  { 0x0047, "gu" },
  { 0x0447, "gu-IN" },
  { 0x0068, "ha" },
  { 0x7C68, "ha-Latn" },
  { 0x0468, "ha-Latn-NG" },
  { 0x000D, "he" },
  { 0x040D, "he-IL" },
  { 0x0039, "hi" },
  { 0x0439, "hi-IN" },
  { 0x000E, "hu" },
  { 0x040E, "hu-HU" },
  { 0x000F, "is" },
  { 0x040F, "is-IS" },
  { 0x0070, "ig" },
  { 0x0470, "ig-NG" },
  { 0x0021, "id" },
  { 0x0421, "id-ID" },
  { 0x005D, "iu" },
  { 0x7C5D, "iu-Latn" },
  { 0x085D, "iu-Latn-CA" },
  { 0x785D, "iu-Cans" },
  { 0x045D, "iu-Cans-CA" },
  { 0x003C, "ga" },
  { 0x083C, "ga-IE" },
  { 0x0034, "xh" },
  { 0x0434, "xh-ZA" },
  { 0x0035, "zu" },
  { 0x0435, "zu-ZA" },
  { 0x0010, "it" },
  { 0x0410, "it-IT" },
  { 0x0810, "it-CH" },
  { 0x0011, "ja" },
  { 0x0411, "ja-JP" },
  { 0x004B, "kn" },
  { 0x044B, "kn-IN" },
  { 0x003F, "kk" },
  { 0x043F, "kk-KZ" },
  { 0x0053, "km" },
  { 0x0453, "km-KH" },
  { 0x0086, "qut" },
  { 0x0486, "qut-GT" },
  { 0x0087, "rw" },
  { 0x0487, "rw-RW" },
  { 0x0041, "sw" },
  { 0x0441, "sw-KE" },
  { 0x0057, "kok" },
  { 0x0457, "kok-IN" },
  { 0x0012, "ko" },
  { 0x0412, "ko-KR" },
  { 0x0040, "ky" },
  { 0x0440, "ky-KG" },
  { 0x0054, "lo" },
  { 0x0454, "lo-LA" },
  { 0x0026, "lv" },
  { 0x0426, "lv-LV" },
  { 0x0027, "lt" },
  { 0x0427, "lt-LT" },
  { 0x7C2E, "dsb" },
  { 0x082E, "dsb-DE" },
  { 0x006E, "lb" },
  { 0x046E, "lb-LU" },
  { 0x042F, "mk-MK" },
  { 0x002F, "mk" },
  { 0x003E, "ms" },
  { 0x083E, "ms-BN" },
  { 0x043E, "ms-MY" },
  { 0x004C, "ml" },
  { 0x044C, "ml-IN" },
  { 0x003A, "mt" },
  { 0x043A, "mt-MT" },
  { 0x0081, "mi" },
  { 0x0481, "mi-NZ" },
  { 0x007A, "arn" },
  { 0x047A, "arn-CL" },
  { 0x004E, "mr" },
  { 0x044E, "mr-IN" },
  { 0x007C, "moh" },
  { 0x047C, "moh-CA" },
  { 0x0050, "mn" },
  { 0x7850, "mn-Cyrl" },
  { 0x0450, "mn-MN" },
  { 0x7C50, "mn-Mong" },
  { 0x0850, "mn-Mong-CN" },
  { 0x0061, "ne" },
  { 0x0461, "ne-NP" },
  { 0x0014, "no" },
  { 0x7C14, "nb" },
  { 0x7814, "nn" },
  { 0x0414, "nb-NO" },
  { 0x0814, "nn-NO" },
  { 0x0082, "oc" },
  { 0x0482, "oc-FR" },
  { 0x0048, "or" },
  { 0x0448, "or-IN" },
  { 0x0063, "ps" },
  { 0x0463, "ps-AF" },
  { 0x0029, "fa" },
  { 0x0429, "fa-IR" },
  { 0x0015, "pl" },
  { 0x0415, "pl-PL" },
  { 0x0016, "pt" },
  { 0x0416, "pt-BR" },
  { 0x0816, "pt-PT" },
  { 0x0046, "pa" },
  { 0x0446, "pa-IN" },
  { 0x006B, "quz" },
  { 0x046B, "quz-BO" },
  { 0x086B, "quz-EC" },
  { 0x0C6B, "quz-PE" },
  { 0x0018, "ro" },
  { 0x0418, "ro-RO" },
  { 0x0017, "rm" },
  { 0x0417, "rm-CH" },
  { 0x0019, "ru" },
  { 0x0419, "ru-RU" },
  { 0x703B, "smn" },
  { 0x7C3B, "smj" },
  { 0x003B, "se" },
  { 0x743B, "sms" },
  { 0x783B, "sma" },
  { 0x243B, "smn-FI" },
  { 0x103B, "smj-NO" },
  { 0x143B, "smj-SE" },
  { 0x0C3B, "se-FI" },
  { 0x043B, "se-NO" },
  { 0x083B, "se-SE" },
  { 0x203B, "sms-FI" },
  { 0x183B, "sma-NO" },
  { 0x1C3B, "sma-SE" },
  { 0x004F, "sa" },
  { 0x044F, "sa-IN" },
  { 0x0091, "gd" },
  { 0x0491, "gd-GB" },
  { 0x7C1A, "sr" },
  { 0x6C1A, "sr-Cyrl" },
  { 0x1C1A, "sr-Cyrl-BA" },
  { 0x301A, "sr-Cyrl-ME" },
  { 0x0C1A, "sr-Cyrl-CS" },
  { 0x281A, "sr-Cyrl-RS" },
  { 0x701A, "sr-Latn" },
  { 0x181A, "sr-Latn-BA" },
  { 0x2C1A, "sr-Latn-ME" },
  { 0x081A, "sr-Latn-CS" },
  { 0x241A, "sr-Latn-RS" },
  { 0x006C, "nso" },
  { 0x046C, "nso-ZA" },
  { 0x0032, "tn" },
  { 0x0432, "tn-ZA" },
  { 0x005B, "si" },
  { 0x045B, "si-LK" },
  { 0x001B, "sk" },
  { 0x041B, "sk-SK" },
  { 0x0024, "sl" },
  { 0x0424, "sl-SI" },
  { 0x000A, "es" },
  { 0x2C0A, "es-AR" },
  { 0x400A, "es-BO" },
  { 0x340A, "es-CL" },
  { 0x240A, "es-CO" },
  { 0x140A, "es-CR" },
  { 0x1C0A, "es-DO" },
  { 0x300A, "es-EC" },
  { 0x440A, "es-SV" },
  { 0x100A, "es-GT" },
  { 0x480A, "es-HN" },
  { 0x080A, "es-MX" },
  { 0x4C0A, "es-NI" },
  { 0x180A, "es-PA" },
  { 0x3C0A, "es-PY" },
  { 0x280A, "es-PE" },
  { 0x500A, "es-PR" },
  { 0x0C0A, "es-ES" },
  { 0x540A, "es-US" },
  { 0x380A, "es-UY" },
  { 0x200A, "es-VE" },
  { 0x001D, "sv" },
  { 0x081D, "sv-FI" },
  { 0x041D, "sv-SE" },
  { 0x005A, "syr" },
  { 0x045A, "syr-SY" },
  { 0x0028, "tg" },
  { 0x7C28, "tg-Cyrl" },
  { 0x0428, "tg-Cyrl-TJ" },
  { 0x005F, "tzm" },
  { 0x7C5F, "tzm-Latn" },
  { 0x085F, "tzm-Latn-DZ" },
  { 0x0049, "ta" },
  { 0x0449, "ta-IN" },
  { 0x0044, "tt" },
  { 0x0444, "tt-RU" },
  { 0x004A, "te" },
  { 0x044A, "te-IN" },
  { 0x001E, "th" },
  { 0x041E, "th-TH" },
  { 0x0051, "bo" },
  { 0x0451, "bo-CN" },
  { 0x001F, "tr" },
  { 0x041F, "tr-TR" },
  { 0x0042, "tk" },
  { 0x0442, "tk-TM" },
  { 0x0022, "uk" },
  { 0x0422, "uk-UA" },
  { 0x002E, "hsb" },
  { 0x042E, "hsb-DE" },
  { 0x0020, "ur" },
  { 0x0420, "ur-PK" },
  { 0x0080, "ug" },
  { 0x0480, "ug-CN" },
  { 0x7843, "uz-Cyrl" },
  { 0x0843, "uz-Cyrl-UZ" },
  { 0x0043, "uz" },
  { 0x7C43, "uz-Latn" },
  { 0x0443, "uz-Latn-UZ" },
  { 0x002A, "vi" },
  { 0x042A, "vi-VN" },
  { 0x0052, "cy" },
  { 0x0452, "cy-GB" },
  { 0x0088, "wo" },
  { 0x0488, "wo-SN" },
  { 0x0085, "sah" },
  { 0x0485, "sah-RU" },
  { 0x0078, "ii" },
  { 0x0478, "ii-CN" },
  { 0x006A, "yo" },
  { 0x046A, "yo-NG" }
};

#endif
