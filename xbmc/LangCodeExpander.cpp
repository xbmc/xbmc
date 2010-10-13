/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "LangCodeExpander.h"
#include "SectionLoader.h"
#include "tinyXML/tinyxml.h"
#include "utils/log.h" 

#define MAKECODE(a, b, c, d)  ((((long)(a))<<24) | (((long)(b))<<16) | (((long)(c))<<8) | (long)(d))
#define MAKETWOCHARCODE(a, b) ((((long)(a))<<8) | (long)(b)) 

typedef struct LCENTRY
{
  long code;
  const char *name;
} LCENTRY;

extern const struct LCENTRY g_iso639_1[143];
extern const struct LCENTRY g_iso639_2[536];

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

bool CLangCodeExpander::ConvertTwoToThreeCharCode(CStdString& strThreeCharCode, const CStdString& strTwoCharCode
#ifdef _WIN32
    , bool localeHack /*= false*/
#endif
    )
{       
  if ( strTwoCharCode.length()==2 )
  {
    CStdString strTwoCharCodeLower( strTwoCharCode );
    strTwoCharCodeLower.MakeLower();
    strTwoCharCodeLower.TrimLeft();
    strTwoCharCodeLower.TrimRight();
    
    long twoCharCode = (((long)(strTwoCharCode[0]))<<8) | (long)(strTwoCharCode[1]);

    switch (twoCharCode) {
      case MAKETWOCHARCODE('a','a'): strThreeCharCode =  "aar"; break;
      case MAKETWOCHARCODE('a','b'): strThreeCharCode =  "abk"; break;
      case MAKETWOCHARCODE('a','f'): strThreeCharCode =  "afr"; break;
      case MAKETWOCHARCODE('a','k'): strThreeCharCode =  "aka"; break;
      case MAKETWOCHARCODE('a','m'): strThreeCharCode =  "amh"; break;
      case MAKETWOCHARCODE('a','r'): strThreeCharCode =  "ara"; break;
      case MAKETWOCHARCODE('a','n'): strThreeCharCode =  "arg"; break;
      case MAKETWOCHARCODE('a','s'): strThreeCharCode =  "asm"; break;
      case MAKETWOCHARCODE('a','v'): strThreeCharCode =  "ava"; break;
      case MAKETWOCHARCODE('a','e'): strThreeCharCode =  "ave"; break;
      case MAKETWOCHARCODE('a','y'): strThreeCharCode =  "aym"; break;
      case MAKETWOCHARCODE('a','z'): strThreeCharCode =  "aze"; break;
      case MAKETWOCHARCODE('b','a'): strThreeCharCode =  "bak"; break;
      case MAKETWOCHARCODE('b','m'): strThreeCharCode =  "bam"; break;
      case MAKETWOCHARCODE('b','e'): strThreeCharCode =  "bel"; break;
      case MAKETWOCHARCODE('b','n'): strThreeCharCode =  "ben"; break;
      case MAKETWOCHARCODE('b','h'): strThreeCharCode =  "bih"; break;
      case MAKETWOCHARCODE('b','i'): strThreeCharCode =  "bis"; break;
      case MAKETWOCHARCODE('b','o'): strThreeCharCode =  "tib"; break;  // terminology lang code: bod
      case MAKETWOCHARCODE('b','s'): strThreeCharCode =  "bos"; break;
      case MAKETWOCHARCODE('b','r'): strThreeCharCode =  "bre"; break;
      case MAKETWOCHARCODE('b','g'): strThreeCharCode =  "bul"; break;
      case MAKETWOCHARCODE('c','a'): strThreeCharCode =  "cat"; break;
      case MAKETWOCHARCODE('c','s'):
#ifdef _WIN32
        if (localeHack)
          strThreeCharCode = "csy";
        else
#endif
          strThreeCharCode =  "cze";
        break;  // terminology lang code: ces
      case MAKETWOCHARCODE('c','h'): strThreeCharCode =  "cha"; break;
      case MAKETWOCHARCODE('c','e'): strThreeCharCode =  "che"; break;
      case MAKETWOCHARCODE('c','u'): strThreeCharCode =  "chu"; break;
      case MAKETWOCHARCODE('c','v'): strThreeCharCode =  "chv"; break;
      case MAKETWOCHARCODE('k','w'): strThreeCharCode =  "cor"; break;
      case MAKETWOCHARCODE('c','o'): strThreeCharCode =  "cos"; break;
      case MAKETWOCHARCODE('c','r'): strThreeCharCode =  "cre"; break;
      case MAKETWOCHARCODE('c','y'): strThreeCharCode =  "wel"; break;  // terminology lang code: cym
      case MAKETWOCHARCODE('d','a'): strThreeCharCode =  "dan"; break;
      case MAKETWOCHARCODE('d','e'):
#ifdef _WIN32
        if (localeHack)
          strThreeCharCode = "deu";
        else
#endif
          strThreeCharCode =  "ger";
        break;  // terminology lang code: deu
      case MAKETWOCHARCODE('d','v'): strThreeCharCode =  "div"; break;
      case MAKETWOCHARCODE('d','z'): strThreeCharCode =  "dzo"; break;
      case MAKETWOCHARCODE('e','l'):
#ifdef _WIN32
        if (localeHack)
          strThreeCharCode = "ell";
        else
#endif
          strThreeCharCode =  "gre";
        break;  // terminology lang code: ell
      case MAKETWOCHARCODE('e','n'): strThreeCharCode =  "eng"; break;
      case MAKETWOCHARCODE('e','o'): strThreeCharCode =  "epo"; break;
      case MAKETWOCHARCODE('e','t'): strThreeCharCode =  "est"; break;
      case MAKETWOCHARCODE('e','u'): strThreeCharCode =  "baq"; break;  // terminology lang code: eus
      case MAKETWOCHARCODE('e','e'): strThreeCharCode =  "ewe"; break;
      case MAKETWOCHARCODE('f','o'): strThreeCharCode =  "fao"; break;
      case MAKETWOCHARCODE('f','a'): strThreeCharCode =  "per"; break;  // terminology lang code: fas
      case MAKETWOCHARCODE('f','j'): strThreeCharCode =  "fij"; break;
      case MAKETWOCHARCODE('f','i'): strThreeCharCode =  "fin"; break;
      case MAKETWOCHARCODE('f','r'):
#ifdef _WIN32
        if (localeHack)
          strThreeCharCode = "fra";
        else
#endif
          strThreeCharCode =  "fre";
        break;  // terminology lang code: fra
      case MAKETWOCHARCODE('f','y'): strThreeCharCode =  "fry"; break;
      case MAKETWOCHARCODE('f','f'): strThreeCharCode =  "ful"; break;
      case MAKETWOCHARCODE('g','d'): strThreeCharCode =  "gla"; break;
      case MAKETWOCHARCODE('g','a'): strThreeCharCode =  "gle"; break;
      case MAKETWOCHARCODE('g','l'): strThreeCharCode =  "glg"; break;
      case MAKETWOCHARCODE('g','v'): strThreeCharCode =  "glv"; break;
      case MAKETWOCHARCODE('g','n'): strThreeCharCode =  "grn"; break;
      case MAKETWOCHARCODE('g','u'): strThreeCharCode =  "guj"; break;
      case MAKETWOCHARCODE('h','t'): strThreeCharCode =  "hat"; break;
      case MAKETWOCHARCODE('h','a'): strThreeCharCode =  "hau"; break;
      case MAKETWOCHARCODE('h','e'): strThreeCharCode =  "heb"; break;
      case MAKETWOCHARCODE('h','z'): strThreeCharCode =  "her"; break;
      case MAKETWOCHARCODE('h','i'): strThreeCharCode =  "hin"; break;
      case MAKETWOCHARCODE('h','o'): strThreeCharCode =  "hmo"; break;
      case MAKETWOCHARCODE('h','r'): strThreeCharCode =  "hrv"; break;
      case MAKETWOCHARCODE('h','u'): strThreeCharCode =  "hun"; break;
      case MAKETWOCHARCODE('h','y'): strThreeCharCode =  "arm"; break;  // terminology lang code: hye
      case MAKETWOCHARCODE('i','g'): strThreeCharCode =  "ibo"; break;
      case MAKETWOCHARCODE('i','o'): strThreeCharCode =  "ido"; break;
      case MAKETWOCHARCODE('i','i'): strThreeCharCode =  "iii"; break;
      case MAKETWOCHARCODE('i','u'): strThreeCharCode =  "iku"; break;
      case MAKETWOCHARCODE('i','e'): strThreeCharCode =  "ile"; break;
      case MAKETWOCHARCODE('i','a'): strThreeCharCode =  "ina"; break;
      case MAKETWOCHARCODE('i','d'): strThreeCharCode =  "ind"; break;
      case MAKETWOCHARCODE('i','k'): strThreeCharCode =  "ipk"; break;
      case MAKETWOCHARCODE('i','s'):
#ifdef _WIN32
        if (localeHack)
          strThreeCharCode = "isl";
        else
#endif
          strThreeCharCode =  "ice";
        break;  // terminology lang code: isl
      case MAKETWOCHARCODE('i','t'): strThreeCharCode =  "ita"; break;
      case MAKETWOCHARCODE('j','v'): strThreeCharCode =  "jav"; break;
      case MAKETWOCHARCODE('j','a'): strThreeCharCode =  "jpn"; break;
      case MAKETWOCHARCODE('k','l'): strThreeCharCode =  "kal"; break;
      case MAKETWOCHARCODE('k','n'): strThreeCharCode =  "kan"; break;
      case MAKETWOCHARCODE('k','s'): strThreeCharCode =  "kas"; break;
      case MAKETWOCHARCODE('k','a'): strThreeCharCode =  "geo"; break;  // terminology lang code: kat
      case MAKETWOCHARCODE('k','r'): strThreeCharCode =  "kau"; break;
      case MAKETWOCHARCODE('k','k'): strThreeCharCode =  "kaz"; break;
      case MAKETWOCHARCODE('k','m'): strThreeCharCode =  "khm"; break;
      case MAKETWOCHARCODE('k','i'): strThreeCharCode =  "kik"; break;
      case MAKETWOCHARCODE('r','w'): strThreeCharCode =  "kin"; break;
      case MAKETWOCHARCODE('k','y'): strThreeCharCode =  "kir"; break;
      case MAKETWOCHARCODE('k','v'): strThreeCharCode =  "kom"; break;
      case MAKETWOCHARCODE('k','g'): strThreeCharCode =  "kon"; break;
      case MAKETWOCHARCODE('k','o'): strThreeCharCode =  "kor"; break;
      case MAKETWOCHARCODE('k','j'): strThreeCharCode =  "kua"; break;
      case MAKETWOCHARCODE('k','u'): strThreeCharCode =  "kur"; break;
      case MAKETWOCHARCODE('l','o'): strThreeCharCode =  "lao"; break;
      case MAKETWOCHARCODE('l','a'): strThreeCharCode =  "lat"; break;
      case MAKETWOCHARCODE('l','v'): strThreeCharCode =  "lav"; break;
      case MAKETWOCHARCODE('l','i'): strThreeCharCode =  "lim"; break;
      case MAKETWOCHARCODE('l','n'): strThreeCharCode =  "lin"; break;
      case MAKETWOCHARCODE('l','t'): strThreeCharCode =  "lit"; break;
      case MAKETWOCHARCODE('l','b'): strThreeCharCode =  "ltz"; break;
      case MAKETWOCHARCODE('l','u'): strThreeCharCode =  "lub"; break;
      case MAKETWOCHARCODE('l','g'): strThreeCharCode =  "lug"; break;
      case MAKETWOCHARCODE('m','k'): strThreeCharCode =  "mac"; break;  // terminology lang code: mkd
      case MAKETWOCHARCODE('m','h'): strThreeCharCode =  "mah"; break;
      case MAKETWOCHARCODE('m','l'): strThreeCharCode =  "mal"; break;
      case MAKETWOCHARCODE('m','i'): strThreeCharCode =  "mao"; break;  // terminology lang code: mri
      case MAKETWOCHARCODE('m','r'): strThreeCharCode =  "mar"; break;
      case MAKETWOCHARCODE('m','s'): strThreeCharCode =  "may"; break;  // terminology lang code: msa
      case MAKETWOCHARCODE('m','g'): strThreeCharCode =  "mlg"; break;
      case MAKETWOCHARCODE('m','t'): strThreeCharCode =  "mlt"; break;
      case MAKETWOCHARCODE('m','n'): strThreeCharCode =  "mon"; break;
      case MAKETWOCHARCODE('m','y'): strThreeCharCode =  "bur"; break;  // terminology lang code: mya
      case MAKETWOCHARCODE('n','a'): strThreeCharCode =  "nau"; break;
      case MAKETWOCHARCODE('n','v'): strThreeCharCode =  "nav"; break;
      case MAKETWOCHARCODE('n','r'): strThreeCharCode =  "nbl"; break;
      case MAKETWOCHARCODE('n','d'): strThreeCharCode =  "nde"; break;
      case MAKETWOCHARCODE('n','g'): strThreeCharCode =  "ndo"; break;
      case MAKETWOCHARCODE('n','e'): strThreeCharCode =  "nep"; break;
      case MAKETWOCHARCODE('n','l'):
#ifdef _WIN32
        if (localeHack)
          strThreeCharCode = "nld";
        else
#endif
          strThreeCharCode =  "dut";
        break;  // terminology lang code: nld
      case MAKETWOCHARCODE('n','n'): strThreeCharCode =  "nno"; break;
      case MAKETWOCHARCODE('n','b'): strThreeCharCode =  "nob"; break;
      case MAKETWOCHARCODE('n','o'): strThreeCharCode =  "nor"; break;
      case MAKETWOCHARCODE('n','y'): strThreeCharCode =  "nya"; break;
      case MAKETWOCHARCODE('o','c'): strThreeCharCode =  "oci"; break;
      case MAKETWOCHARCODE('o','j'): strThreeCharCode =  "oji"; break;
      case MAKETWOCHARCODE('o','r'): strThreeCharCode =  "ori"; break;
      case MAKETWOCHARCODE('o','m'): strThreeCharCode =  "orm"; break;
      case MAKETWOCHARCODE('o','s'): strThreeCharCode =  "oss"; break;
      case MAKETWOCHARCODE('p','a'): strThreeCharCode =  "pan"; break;
      case MAKETWOCHARCODE('p','i'): strThreeCharCode =  "pli"; break;
      case MAKETWOCHARCODE('p','l'):
#ifdef _WIN32
        if (localeHack)
          strThreeCharCode = "plk";
        else
#endif
          strThreeCharCode =  "pol";
        break;
      case MAKETWOCHARCODE('p','t'):
#ifdef _WIN32
        if (localeHack)
          strThreeCharCode = "ptg";
        else
#endif
          strThreeCharCode =  "por";
        break;
      case MAKETWOCHARCODE('p','s'): strThreeCharCode =  "pus"; break;
      case MAKETWOCHARCODE('q','u'): strThreeCharCode =  "que"; break;
      case MAKETWOCHARCODE('r','m'): strThreeCharCode =  "roh"; break;
      case MAKETWOCHARCODE('r','o'):
#ifdef _WIN32
        if (localeHack)
          strThreeCharCode = "ron";
        else
#endif
          strThreeCharCode =  "rum";
        break;  // terminology lang code: ron
      case MAKETWOCHARCODE('r','n'): strThreeCharCode =  "run"; break;
      case MAKETWOCHARCODE('r','u'): strThreeCharCode =  "rus"; break;
      case MAKETWOCHARCODE('s','g'): strThreeCharCode =  "sag"; break;
      case MAKETWOCHARCODE('s','a'): strThreeCharCode =  "san"; break;
      case MAKETWOCHARCODE('s','i'): strThreeCharCode =  "sin"; break;
      case MAKETWOCHARCODE('s','k'):
#ifdef _WIN32
        if (localeHack)
          strThreeCharCode = "sky";
        else
#endif
          strThreeCharCode =  "slo"; break;  // terminology lang code: slk
      case MAKETWOCHARCODE('s','l'): strThreeCharCode =  "slv"; break;
      case MAKETWOCHARCODE('s','e'): strThreeCharCode =  "sme"; break;
      case MAKETWOCHARCODE('s','m'): strThreeCharCode =  "smo"; break;
      case MAKETWOCHARCODE('s','n'): strThreeCharCode =  "sna"; break;
      case MAKETWOCHARCODE('s','d'): strThreeCharCode =  "snd"; break;
      case MAKETWOCHARCODE('s','o'): strThreeCharCode =  "som"; break;
      case MAKETWOCHARCODE('s','t'): strThreeCharCode =  "sot"; break;
      case MAKETWOCHARCODE('e','s'):
#ifdef _WIN32
        if (localeHack)
          strThreeCharCode = "esp";
        else
#endif
          strThreeCharCode =  "spa";
        break;
      case MAKETWOCHARCODE('s','q'): strThreeCharCode =  "alb"; break;  // terminology lang code: sqi
      case MAKETWOCHARCODE('s','c'): strThreeCharCode =  "srd"; break;
      case MAKETWOCHARCODE('s','r'): strThreeCharCode =  "srp"; break;
      case MAKETWOCHARCODE('s','s'): strThreeCharCode =  "ssw"; break;
      case MAKETWOCHARCODE('s','u'): strThreeCharCode =  "sun"; break;
      case MAKETWOCHARCODE('s','w'): strThreeCharCode =  "swa"; break;
      case MAKETWOCHARCODE('s','v'):
#ifdef _WIN32
        if (localeHack)
          strThreeCharCode = "sve";
        else
#endif
        strThreeCharCode =  "swe"; break;
      case MAKETWOCHARCODE('t','y'): strThreeCharCode =  "tah"; break;
      case MAKETWOCHARCODE('t','a'): strThreeCharCode =  "tam"; break;
      case MAKETWOCHARCODE('t','t'): strThreeCharCode =  "tat"; break;
      case MAKETWOCHARCODE('t','e'): strThreeCharCode =  "tel"; break;
      case MAKETWOCHARCODE('t','g'): strThreeCharCode =  "tgk"; break;
      case MAKETWOCHARCODE('t','l'): strThreeCharCode =  "tgl"; break;
      case MAKETWOCHARCODE('t','h'): strThreeCharCode =  "tha"; break;
      case MAKETWOCHARCODE('t','i'): strThreeCharCode =  "tir"; break;
      case MAKETWOCHARCODE('t','o'): strThreeCharCode =  "ton"; break;
      case MAKETWOCHARCODE('t','n'): strThreeCharCode =  "tsn"; break;
      case MAKETWOCHARCODE('t','s'): strThreeCharCode =  "tso"; break;
      case MAKETWOCHARCODE('t','k'): strThreeCharCode =  "tuk"; break;
      case MAKETWOCHARCODE('t','r'):
#ifdef _WIN32
        if (localeHack)
          strThreeCharCode = "trk";
        else
#endif
          strThreeCharCode =  "tur";
        break;
      case MAKETWOCHARCODE('t','w'): strThreeCharCode =  "twi"; break;
      case MAKETWOCHARCODE('u','g'): strThreeCharCode =  "uig"; break;
      case MAKETWOCHARCODE('u','k'): strThreeCharCode =  "ukr"; break;
      case MAKETWOCHARCODE('u','r'): strThreeCharCode =  "urd"; break;
      case MAKETWOCHARCODE('u','z'): strThreeCharCode =  "uzb"; break;
      case MAKETWOCHARCODE('v','e'): strThreeCharCode =  "ven"; break;
      case MAKETWOCHARCODE('v','i'): strThreeCharCode =  "vie"; break;
      case MAKETWOCHARCODE('v','o'): strThreeCharCode =  "vol"; break;
      case MAKETWOCHARCODE('w','a'): strThreeCharCode =  "wln"; break;
      case MAKETWOCHARCODE('w','o'): strThreeCharCode =  "wol"; break;
      case MAKETWOCHARCODE('x','h'): strThreeCharCode =  "xho"; break;
      case MAKETWOCHARCODE('y','i'): strThreeCharCode =  "yid"; break;
      case MAKETWOCHARCODE('y','o'): strThreeCharCode =  "yor"; break;
      case MAKETWOCHARCODE('z','a'): strThreeCharCode =  "zha"; break;
      case MAKETWOCHARCODE('z','h'): strThreeCharCode =  "chi"; break;  // terminology lang code: zho
      case MAKETWOCHARCODE('z','u'): strThreeCharCode =  "zul"; break;
      default: return false; break;
    }
    // did not reach default statement
    return true;
  }
  // not a 2 char code
  return false;
}

// Based on ISO 3166
bool CLangCodeExpander::ConvertLinuxToWindowsRegionCodes(const CStdString& linux, CStdString& windows)
{
  if (linux.length() != 2)
    return false;

  CStdString strLower( linux );
  strLower.MakeLower();
  strLower.TrimLeft();
  strLower.TrimRight();
  long twoCharCode = (((long)(strLower[0])) << 8) | (long)(strLower[1]);

  switch (twoCharCode)
  {
    case MAKETWOCHARCODE('a','f'): windows =  "afg"; break;
    case MAKETWOCHARCODE('a','x'): windows =  "ala"; break;
    case MAKETWOCHARCODE('a','l'): windows =  "alb"; break;
    case MAKETWOCHARCODE('d','z'): windows =  "dza"; break;
    case MAKETWOCHARCODE('a','s'): windows =  "asm"; break;
    case MAKETWOCHARCODE('a','d'): windows =  "and"; break;
    case MAKETWOCHARCODE('a','o'): windows =  "ago"; break;
    case MAKETWOCHARCODE('a','i'): windows =  "aia"; break;
    case MAKETWOCHARCODE('a','q'): windows =  "ata"; break;
    case MAKETWOCHARCODE('a','g'): windows =  "atg"; break;
    case MAKETWOCHARCODE('a','r'): windows =  "arg"; break;
    case MAKETWOCHARCODE('a','m'): windows =  "arm"; break;
    case MAKETWOCHARCODE('a','w'): windows =  "abw"; break;
    case MAKETWOCHARCODE('a','u'): windows =  "aus"; break;
    case MAKETWOCHARCODE('a','t'): windows =  "aut"; break;
    case MAKETWOCHARCODE('a','z'): windows =  "aze"; break;
    case MAKETWOCHARCODE('b','s'): windows =  "bhs"; break;
    case MAKETWOCHARCODE('b','h'): windows =  "bhr"; break;
    case MAKETWOCHARCODE('b','d'): windows =  "bgd"; break;
    case MAKETWOCHARCODE('b','b'): windows =  "brb"; break;
    case MAKETWOCHARCODE('b','y'): windows =  "blr"; break;
    case MAKETWOCHARCODE('b','e'): windows =  "bel"; break;
    case MAKETWOCHARCODE('b','z'): windows =  "blz"; break;
    case MAKETWOCHARCODE('b','j'): windows =  "ben"; break;
    case MAKETWOCHARCODE('b','m'): windows =  "bmu"; break;
    case MAKETWOCHARCODE('b','t'): windows =  "btn"; break;
    case MAKETWOCHARCODE('b','o'): windows =  "bol"; break;
    case MAKETWOCHARCODE('b','a'): windows =  "bih"; break;
    case MAKETWOCHARCODE('b','w'): windows =  "bwa"; break;
    case MAKETWOCHARCODE('b','v'): windows =  "bvt"; break;
    case MAKETWOCHARCODE('b','r'): windows =  "bra"; break;
    case MAKETWOCHARCODE('i','o'): windows =  "iot"; break;
    case MAKETWOCHARCODE('b','n'): windows =  "brn"; break;
    case MAKETWOCHARCODE('b','g'): windows =  "bgr"; break;
    case MAKETWOCHARCODE('b','f'): windows =  "bfa"; break;
    case MAKETWOCHARCODE('b','i'): windows =  "bdi"; break;
    case MAKETWOCHARCODE('k','h'): windows =  "khm"; break;
    case MAKETWOCHARCODE('c','m'): windows =  "cmr"; break;
    case MAKETWOCHARCODE('c','a'): windows =  "can"; break;
    case MAKETWOCHARCODE('c','v'): windows =  "cpv"; break;
    case MAKETWOCHARCODE('k','y'): windows =  "cym"; break;
    case MAKETWOCHARCODE('c','f'): windows =  "caf"; break;
    case MAKETWOCHARCODE('t','d'): windows =  "tcd"; break;
    case MAKETWOCHARCODE('c','l'): windows =  "chl"; break;
    case MAKETWOCHARCODE('c','n'): windows =  "chn"; break;
    case MAKETWOCHARCODE('c','x'): windows =  "cxr"; break;
    case MAKETWOCHARCODE('c','c'): windows =  "cck"; break;
    case MAKETWOCHARCODE('c','o'): windows =  "col"; break;
    case MAKETWOCHARCODE('k','m'): windows =  "com"; break;
    case MAKETWOCHARCODE('c','g'): windows =  "cog"; break;
    case MAKETWOCHARCODE('c','d'): windows =  "cod"; break;
    case MAKETWOCHARCODE('c','k'): windows =  "cok"; break;
    case MAKETWOCHARCODE('c','r'): windows =  "cri"; break;
    case MAKETWOCHARCODE('c','i'): windows =  "civ"; break;
    case MAKETWOCHARCODE('h','r'): windows =  "hrv"; break;
    case MAKETWOCHARCODE('c','u'): windows =  "cub"; break;
    case MAKETWOCHARCODE('c','y'): windows =  "cyp"; break;
    case MAKETWOCHARCODE('c','z'): windows =  "cze"; break;
    case MAKETWOCHARCODE('d','k'): windows =  "dnk"; break;
    case MAKETWOCHARCODE('d','j'): windows =  "dji"; break;
    case MAKETWOCHARCODE('d','m'): windows =  "dma"; break;
    case MAKETWOCHARCODE('d','o'): windows =  "dom"; break;
    case MAKETWOCHARCODE('e','c'): windows =  "ecu"; break;
    case MAKETWOCHARCODE('e','g'): windows =  "egy"; break;
    case MAKETWOCHARCODE('s','v'): windows =  "slv"; break;
    case MAKETWOCHARCODE('g','q'): windows =  "gnq"; break;
    case MAKETWOCHARCODE('e','r'): windows =  "eri"; break;
    case MAKETWOCHARCODE('e','e'): windows =  "est"; break;
    case MAKETWOCHARCODE('e','t'): windows =  "eth"; break;
    case MAKETWOCHARCODE('f','k'): windows =  "flk"; break;
    case MAKETWOCHARCODE('f','o'): windows =  "fro"; break;
    case MAKETWOCHARCODE('f','j'): windows =  "fji"; break;
    case MAKETWOCHARCODE('f','i'): windows =  "fin"; break;
    case MAKETWOCHARCODE('f','r'): windows =  "fra"; break;
    case MAKETWOCHARCODE('g','f'): windows =  "guf"; break;
    case MAKETWOCHARCODE('p','f'): windows =  "pyf"; break;
    case MAKETWOCHARCODE('t','f'): windows =  "atf"; break;
    case MAKETWOCHARCODE('g','a'): windows =  "gab"; break;
    case MAKETWOCHARCODE('g','m'): windows =  "gmb"; break;
    case MAKETWOCHARCODE('g','e'): windows =  "geo"; break;
    case MAKETWOCHARCODE('d','e'): windows =  "deu"; break;
    case MAKETWOCHARCODE('g','h'): windows =  "gha"; break;
    case MAKETWOCHARCODE('g','i'): windows =  "gib"; break;
    case MAKETWOCHARCODE('g','r'): windows =  "grc"; break;
    case MAKETWOCHARCODE('g','l'): windows =  "grl"; break;
    case MAKETWOCHARCODE('g','d'): windows =  "grd"; break;
    case MAKETWOCHARCODE('g','p'): windows =  "glp"; break;
    case MAKETWOCHARCODE('g','u'): windows =  "gum"; break;
    case MAKETWOCHARCODE('g','t'): windows =  "gtm"; break;
    case MAKETWOCHARCODE('g','g'): windows =  "ggy"; break;
    case MAKETWOCHARCODE('g','n'): windows =  "gin"; break;
    case MAKETWOCHARCODE('g','w'): windows =  "gnb"; break;
    case MAKETWOCHARCODE('g','y'): windows =  "guy"; break;
    case MAKETWOCHARCODE('h','t'): windows =  "hti"; break;
    case MAKETWOCHARCODE('h','m'): windows =  "hmd"; break;
    case MAKETWOCHARCODE('v','a'): windows =  "vat"; break;
    case MAKETWOCHARCODE('h','n'): windows =  "hnd"; break;
    case MAKETWOCHARCODE('h','k'): windows =  "hkg"; break;
    case MAKETWOCHARCODE('h','u'): windows =  "hun"; break;
    case MAKETWOCHARCODE('i','s'): windows =  "isl"; break;
    case MAKETWOCHARCODE('i','n'): windows =  "ind"; break;
    case MAKETWOCHARCODE('i','d'): windows =  "idn"; break;
    case MAKETWOCHARCODE('i','r'): windows =  "irn"; break;
    case MAKETWOCHARCODE('i','q'): windows =  "irq"; break;
    case MAKETWOCHARCODE('i','e'): windows =  "irl"; break;
    case MAKETWOCHARCODE('i','m'): windows =  "imn"; break;
    case MAKETWOCHARCODE('i','l'): windows =  "isr"; break;
    case MAKETWOCHARCODE('i','t'): windows =  "ita"; break;
    case MAKETWOCHARCODE('j','m'): windows =  "jam"; break;
    case MAKETWOCHARCODE('j','p'): windows =  "jpn"; break;
    case MAKETWOCHARCODE('j','e'): windows =  "jey"; break;
    case MAKETWOCHARCODE('j','o'): windows =  "jor"; break;
    case MAKETWOCHARCODE('k','z'): windows =  "kaz"; break;
    case MAKETWOCHARCODE('k','e'): windows =  "ken"; break;
    case MAKETWOCHARCODE('k','i'): windows =  "kir"; break;
    case MAKETWOCHARCODE('k','p'): windows =  "prk"; break;
    case MAKETWOCHARCODE('k','r'): windows =  "kor"; break;
    case MAKETWOCHARCODE('k','w'): windows =  "kwt"; break;
    case MAKETWOCHARCODE('k','g'): windows =  "kgz"; break;
    case MAKETWOCHARCODE('l','a'): windows =  "lao"; break;
    case MAKETWOCHARCODE('l','v'): windows =  "lva"; break;
    case MAKETWOCHARCODE('l','b'): windows =  "lbn"; break;
    case MAKETWOCHARCODE('l','s'): windows =  "lso"; break;
    case MAKETWOCHARCODE('l','r'): windows =  "lbr"; break;
    case MAKETWOCHARCODE('l','y'): windows =  "lby"; break;
    case MAKETWOCHARCODE('l','i'): windows =  "lie"; break;
    case MAKETWOCHARCODE('l','t'): windows =  "ltu"; break;
    case MAKETWOCHARCODE('l','u'): windows =  "lux"; break;
    case MAKETWOCHARCODE('m','o'): windows =  "mac"; break;
    case MAKETWOCHARCODE('m','k'): windows =  "mkd"; break;
    case MAKETWOCHARCODE('m','g'): windows =  "mdg"; break;
    case MAKETWOCHARCODE('m','w'): windows =  "mwi"; break;
    case MAKETWOCHARCODE('m','y'): windows =  "mys"; break;
    case MAKETWOCHARCODE('m','v'): windows =  "mdv"; break;
    case MAKETWOCHARCODE('m','l'): windows =  "mli"; break;
    case MAKETWOCHARCODE('m','t'): windows =  "mlt"; break;
    case MAKETWOCHARCODE('m','h'): windows =  "mhl"; break;
    case MAKETWOCHARCODE('m','q'): windows =  "mtq"; break;
    case MAKETWOCHARCODE('m','r'): windows =  "mrt"; break;
    case MAKETWOCHARCODE('m','u'): windows =  "mus"; break;
    case MAKETWOCHARCODE('y','t'): windows =  "myt"; break;
    case MAKETWOCHARCODE('m','x'): windows =  "mex"; break;
    case MAKETWOCHARCODE('f','m'): windows =  "fsm"; break;
    case MAKETWOCHARCODE('m','d'): windows =  "mda"; break;
    case MAKETWOCHARCODE('m','c'): windows =  "mco"; break;
    case MAKETWOCHARCODE('m','n'): windows =  "mng"; break;
    case MAKETWOCHARCODE('m','e'): windows =  "mne"; break;
    case MAKETWOCHARCODE('m','s'): windows =  "msr"; break;
    case MAKETWOCHARCODE('m','a'): windows =  "mar"; break;
    case MAKETWOCHARCODE('m','z'): windows =  "moz"; break;
    case MAKETWOCHARCODE('m','m'): windows =  "mmr"; break;
    case MAKETWOCHARCODE('n','a'): windows =  "nam"; break;
    case MAKETWOCHARCODE('n','r'): windows =  "nru"; break;
    case MAKETWOCHARCODE('n','p'): windows =  "npl"; break;
    case MAKETWOCHARCODE('n','l'): windows =  "nld"; break;
    case MAKETWOCHARCODE('a','n'): windows =  "ant"; break;
    case MAKETWOCHARCODE('n','c'): windows =  "ncl"; break;
    case MAKETWOCHARCODE('n','z'): windows =  "nzl"; break;
    case MAKETWOCHARCODE('n','i'): windows =  "nic"; break;
    case MAKETWOCHARCODE('n','e'): windows =  "ner"; break;
    case MAKETWOCHARCODE('n','g'): windows =  "nga"; break;
    case MAKETWOCHARCODE('n','u'): windows =  "niu"; break;
    case MAKETWOCHARCODE('n','f'): windows =  "nfk"; break;
    case MAKETWOCHARCODE('m','p'): windows =  "mnp"; break;
    case MAKETWOCHARCODE('n','o'): windows =  "nor"; break;
    case MAKETWOCHARCODE('o','m'): windows =  "omn"; break;
    case MAKETWOCHARCODE('p','k'): windows =  "pak"; break;
    case MAKETWOCHARCODE('p','w'): windows =  "plw"; break;
    case MAKETWOCHARCODE('p','s'): windows =  "pse"; break;
    case MAKETWOCHARCODE('p','a'): windows =  "pan"; break;
    case MAKETWOCHARCODE('p','g'): windows =  "png"; break;
    case MAKETWOCHARCODE('p','y'): windows =  "pry"; break;
    case MAKETWOCHARCODE('p','e'): windows =  "per"; break;
    case MAKETWOCHARCODE('p','h'): windows =  "phl"; break;
    case MAKETWOCHARCODE('p','n'): windows =  "pcn"; break;
    case MAKETWOCHARCODE('p','l'): windows =  "pol"; break;
    case MAKETWOCHARCODE('p','t'): windows =  "prt"; break;
    case MAKETWOCHARCODE('p','r'): windows =  "pri"; break;
    case MAKETWOCHARCODE('q','a'): windows =  "qat"; break;
    case MAKETWOCHARCODE('r','e'): windows =  "reu"; break;
    case MAKETWOCHARCODE('r','o'): windows =  "rou"; break;
    case MAKETWOCHARCODE('r','u'): windows =  "rus"; break;
    case MAKETWOCHARCODE('r','w'): windows =  "rwa"; break;
    case MAKETWOCHARCODE('b','l'): windows =  "blm"; break;
    case MAKETWOCHARCODE('s','h'): windows =  "shn"; break;
    case MAKETWOCHARCODE('k','n'): windows =  "kna"; break;
    case MAKETWOCHARCODE('l','c'): windows =  "lca"; break;
    case MAKETWOCHARCODE('m','f'): windows =  "maf"; break;
    case MAKETWOCHARCODE('p','m'): windows =  "spm"; break;
    case MAKETWOCHARCODE('v','c'): windows =  "vct"; break;
    case MAKETWOCHARCODE('w','s'): windows =  "wsm"; break;
    case MAKETWOCHARCODE('s','m'): windows =  "smr"; break;
    case MAKETWOCHARCODE('s','t'): windows =  "stp"; break;
    case MAKETWOCHARCODE('s','a'): windows =  "sau"; break;
    case MAKETWOCHARCODE('s','n'): windows =  "sen"; break;
    case MAKETWOCHARCODE('r','s'): windows =  "srb"; break;
    case MAKETWOCHARCODE('s','c'): windows =  "syc"; break;
    case MAKETWOCHARCODE('s','l'): windows =  "sle"; break;
    case MAKETWOCHARCODE('s','g'): windows =  "sgp"; break;
    case MAKETWOCHARCODE('s','k'): windows =  "svk"; break;
    case MAKETWOCHARCODE('s','i'): windows =  "svn"; break;
    case MAKETWOCHARCODE('s','b'): windows =  "slb"; break;
    case MAKETWOCHARCODE('s','o'): windows =  "som"; break;
    case MAKETWOCHARCODE('z','a'): windows =  "zaf"; break;
    case MAKETWOCHARCODE('g','s'): windows =  "sgs"; break;
    case MAKETWOCHARCODE('e','s'): windows =  "esp"; break;
    case MAKETWOCHARCODE('l','k'): windows =  "lka"; break;
    case MAKETWOCHARCODE('s','d'): windows =  "sdn"; break;
    case MAKETWOCHARCODE('s','r'): windows =  "sur"; break;
    case MAKETWOCHARCODE('s','j'): windows =  "sjm"; break;
    case MAKETWOCHARCODE('s','z'): windows =  "swz"; break;
    case MAKETWOCHARCODE('s','e'): windows =  "swe"; break;
    case MAKETWOCHARCODE('c','h'): windows =  "che"; break;
    case MAKETWOCHARCODE('s','y'): windows =  "syr"; break;
    case MAKETWOCHARCODE('t','w'): windows =  "twn"; break;
    case MAKETWOCHARCODE('t','j'): windows =  "tjk"; break;
    case MAKETWOCHARCODE('t','z'): windows =  "tza"; break;
    case MAKETWOCHARCODE('t','h'): windows =  "tha"; break;
    case MAKETWOCHARCODE('t','l'): windows =  "tls"; break;
    case MAKETWOCHARCODE('t','g'): windows =  "tgo"; break;
    case MAKETWOCHARCODE('t','k'): windows =  "tkl"; break;
    case MAKETWOCHARCODE('t','o'): windows =  "ton"; break;
    case MAKETWOCHARCODE('t','t'): windows =  "tto"; break;
    case MAKETWOCHARCODE('t','n'): windows =  "tun"; break;
    case MAKETWOCHARCODE('t','r'): windows =  "tur"; break;
    case MAKETWOCHARCODE('t','m'): windows =  "tkm"; break;
    case MAKETWOCHARCODE('t','c'): windows =  "tca"; break;
    case MAKETWOCHARCODE('t','v'): windows =  "tuv"; break;
    case MAKETWOCHARCODE('u','g'): windows =  "uga"; break;
    case MAKETWOCHARCODE('u','a'): windows =  "ukr"; break;
    case MAKETWOCHARCODE('a','e'): windows =  "are"; break;
    case MAKETWOCHARCODE('g','b'): windows =  "gbr"; break;
    case MAKETWOCHARCODE('u','s'): windows =  "usa"; break;
    case MAKETWOCHARCODE('u','m'): windows =  "umi"; break;
    case MAKETWOCHARCODE('u','y'): windows =  "ury"; break;
    case MAKETWOCHARCODE('u','z'): windows =  "uzb"; break;
    case MAKETWOCHARCODE('v','u'): windows =  "vut"; break;
    case MAKETWOCHARCODE('v','e'): windows =  "ven"; break;
    case MAKETWOCHARCODE('v','n'): windows =  "vnm"; break;
    case MAKETWOCHARCODE('v','g'): windows =  "vgb"; break;
    case MAKETWOCHARCODE('v','i'): windows =  "vir"; break;
    case MAKETWOCHARCODE('w','f'): windows =  "wlf"; break;
    case MAKETWOCHARCODE('e','h'): windows =  "esh"; break;
    case MAKETWOCHARCODE('y','e'): windows =  "yem"; break;
    case MAKETWOCHARCODE('z','m'): windows =  "zmb"; break;
    case MAKETWOCHARCODE('z','w'): windows =  "zwe"; break;
    default:
      return false;
      break;
  }
  return true;
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

bool CLangCodeExpander::LookupInDb(CStdString& desc, const CStdString& code)
{
  long longcode;
  CStdString sCode(code);
  sCode.MakeLower();
  sCode.TrimLeft();
  sCode.TrimRight();
  if(sCode.length() == 2)
  {
    CSectionLoader::Load("LCODE");
    longcode = MAKECODE('\0', '\0', sCode[0], sCode[1]);
    for(unsigned int i = 0; i < sizeof(g_iso639_1) / sizeof(LCENTRY); i++)
    {
      if(g_iso639_1[i].code == longcode)
      {
        desc = g_iso639_1[i].name;
        CSectionLoader::Unload("LCODE");
        return true;
      }
    }
    CSectionLoader::Unload("LCODE");
  }
  else if(code.length() == 3)
  {
    CSectionLoader::Load("LCODE");
    longcode = MAKECODE('\0', sCode[0], sCode[1], sCode[2]);
    for(unsigned int i = 0; i < sizeof(g_iso639_2) / sizeof(LCENTRY); i++)
    {
      if(g_iso639_2[i].code == longcode)
      {
        desc = g_iso639_2[i].name;
        CSectionLoader::Unload("LCODE");
        return true;
      }
    }
    CSectionLoader::Unload("LCODE");
  }
  return false;
}

extern const LCENTRY g_iso639_1[143] =
{
  { MAKECODE('\0','\0','c','c'), "Closed Caption" },
  { MAKECODE('\0','\0','a','a'), "Afar" },
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
  { MAKECODE('\0','\0','d','e'), "Deutsch" },
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
  { MAKECODE('\0','\0','p','t'), "Portugues" },
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

extern const LCENTRY g_iso639_2[536] =
{
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
