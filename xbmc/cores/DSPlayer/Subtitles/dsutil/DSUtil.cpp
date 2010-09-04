/* 
 *  Copyright (C) 2003-2006 Gabest
 *  http://www.gabest.org
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
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include <Vfw.h>
#include "DSUtil.h"
#include <moreuuids.h>

#include <initguid.h>
#include <d3dx9.h>
#include <dxva.h>
#include <dxva2api.h>

int CountPins(IBaseFilter* pBF, int& nIn, int& nOut, int& nInC, int& nOutC)
{
  nIn = nOut = 0;
  nInC = nOutC = 0;

  BeginEnumPins(pBF, pEP, pPin)
  {
    PIN_DIRECTION dir;
    if(SUCCEEDED(pPin->QueryDirection(&dir)))
    {
      Com::SmartPtr<IPin> pPinConnectedTo;
      pPin->ConnectedTo(&pPinConnectedTo);

      if(dir == PINDIR_INPUT) {nIn++; if(pPinConnectedTo) nInC++;}
      else if(dir == PINDIR_OUTPUT) {nOut++; if(pPinConnectedTo) nOutC++;}
    }
  }
  EndEnumPins

  return(nIn+nOut);
}

bool IsSplitter(IBaseFilter* pBF, bool fCountConnectedOnly)
{
  int nIn, nOut, nInC, nOutC;
  CountPins(pBF, nIn, nOut, nInC, nOutC);
  return(fCountConnectedOnly ? nOutC > 1 : nOut > 1);
}

IPin* GetFirstPin(IBaseFilter* pBF, PIN_DIRECTION dir)
{
  if(!pBF) return(NULL);

  BeginEnumPins(pBF, pEP, pPin)
  {
    PIN_DIRECTION dir2;
    pPin->QueryDirection(&dir2);
    if(dir == dir2)
    {
      IPin* pRet = pPin.Detach();
      pRet->Release();
      return(pRet);
    }
  }
  EndEnumPins

  return(NULL);
}

CStdStringW GetPinName(IPin* pPin)
{
  CStdStringW name;
  CPinInfo pi;
  if(pPin && SUCCEEDED(pPin->QueryPinInfo(&pi))) 
    name = pi.achName;
  if(!name.Find(_T("Apple"))) name.Delete(0,1);

  return(name);
}

void memsetd(void* dst, unsigned int c, int nbytes)
{
#ifdef _WIN64
  for (int i=0; i<nbytes/sizeof(DWORD); i++)
    ((DWORD*)dst)[i] = c;
#else
  __asm
  {
    mov eax, c
    mov ecx, nbytes
    shr ecx, 2
    mov edi, dst
    cld
    rep stosd
  }
#endif
}


CStdStringW UTF8To16(LPCSTR utf8)
{
  CStdStringW str;
  int n = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, NULL, 0)-1;
  if(n < 0) return str;
  str.ReleaseBuffer(MultiByteToWideChar(CP_UTF8, 0, utf8, -1, str.GetBuffer(n), n+1)-1);
  return str;
}

static struct {LPCSTR name, iso6392, iso6391; LCID lcid;} s_isolangs[] =  // TODO : fill LCID !!!
{
  {"Abkhazian", "abk", "ab"},
  {"Achinese", "ace", ""},
  {"Acoli", "ach", ""},
  {"Adangme", "ada", ""},
  {"Afar", "aar", "aa"},
  {"Afrihili", "afh", ""},
  {"Afrikaans", "afr", "af",          MAKELCID( MAKELANGID(LANG_AFRIKAANS, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Afro-Asiatic (Other)", "afa", ""},
  {"Akan", "aka", "ak"},
  {"Akkadian", "akk", ""},
  {"Albanian", "alb", "sq"},
  {"Albanian", "sqi", "sq",          MAKELCID( MAKELANGID(LANG_ALBANIAN, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Aleut", "ale", ""},
  {"Algonquian languages", "alg", ""},
  {"Altaic (Other)", "tut", ""},
  {"Amharic", "amh", "am"},
  {"Apache languages", "apa", ""},
  {"Arabic", "ara", "ar",            MAKELCID( MAKELANGID(LANG_ARABIC, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Aragonese", "arg", "an"},
  {"Aramaic", "arc", ""},
  {"Arapaho", "arp", ""},
  {"Araucanian", "arn", ""},
  {"Arawak", "arw", ""},
  {"Armenian", "arm", "hy",          MAKELCID( MAKELANGID(LANG_ARMENIAN, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Armenian", "hye", "hy",          MAKELCID( MAKELANGID(LANG_ARMENIAN, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Artificial (Other)", "art", ""},
  {"Assamese", "asm", "as",          MAKELCID( MAKELANGID(LANG_ASSAMESE, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Asturian; Bable", "ast", ""},
  {"Athapascan languages", "ath", ""},
  {"Australian languages", "aus", ""},
  {"Austronesian (Other)", "map", ""},
  {"Avaric", "ava", "av"},
  {"Avestan", "ave", "ae"},
  {"Awadhi", "awa", ""},
  {"Aymara", "aym", "ay"},
  {"Azerbaijani", "aze", "az",        MAKELCID( MAKELANGID(LANG_AZERI, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Bable; Asturian", "ast", ""},
  {"Balinese", "ban", ""},
  {"Baltic (Other)", "bat", ""},
  {"Baluchi", "bal", ""},
  {"Bambara", "bam", "bm"},
  {"Bamileke languages", "bai", ""},
  {"Banda", "bad", ""},
  {"Bantu (Other)", "bnt", ""},
  {"Basa", "bas", ""},
  {"Bashkir", "bak", "ba",          MAKELCID( MAKELANGID(LANG_BASHKIR, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Basque", "baq", "eu",            MAKELCID( MAKELANGID(LANG_BASQUE, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Basque", "eus", "eu",            MAKELCID( MAKELANGID(LANG_BASQUE, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Batak (Indonesia)", "btk", ""},
  {"Beja", "bej", ""},
  {"Belarusian", "bel", "be",          MAKELCID( MAKELANGID(LANG_BELARUSIAN, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Bemba", "bem", ""},
  {"Bengali", "ben", "bn",          MAKELCID( MAKELANGID(LANG_BENGALI, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Berber (Other)", "ber", ""},
  {"Bhojpuri", "bho", ""},
  {"Bihari", "bih", "bh"},
  {"Bikol", "bik", ""},
  {"Bini", "bin", ""},
  {"Bislama", "bis", "bi"},
  {"Bokmål, Norwegian; Norwegian Bokmål", "nob", "nb"},
  {"Bosnian", "bos", "bs"},
  {"Braj", "bra", ""},
  {"Breton", "bre", "br",            MAKELCID( MAKELANGID(LANG_BRETON, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Buginese", "bug", ""},
  {"Bulgarian", "bul", "bg",          MAKELCID( MAKELANGID(LANG_BULGARIAN, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Buriat", "bua", ""},
  {"Burmese", "bur", "my"},
  {"Burmese", "mya", "my"},
  {"Caddo", "cad", ""},
  {"Carib", "car", ""},
  {"Spanish; Castilian", "spa", "es",      MAKELCID( MAKELANGID(LANG_SPANISH, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Catalan", "cat", "ca",          MAKELCID( MAKELANGID(LANG_CATALAN, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Caucasian (Other)", "cau", ""},
  {"Cebuano", "ceb", ""},
  {"Celtic (Other)", "cel", ""},
  {"Central American Indian (Other)", "cai", ""},
  {"Chagatai", "chg", ""},
  {"Chamic languages", "cmc", ""},
  {"Chamorro", "cha", "ch"},
  {"Chechen", "che", "ce"},
  {"Cherokee", "chr", ""},
  {"Chewa; Chichewa; Nyanja", "nya", "ny"},
  {"Cheyenne", "chy", ""},
  {"Chibcha", "chb", ""},
  {"Chichewa; Chewa; Nyanja", "nya", "ny"},
  {"Chinese", "chi", "zh",          MAKELCID( MAKELANGID(LANG_CHINESE, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Chinese", "zho", "zh"},
  {"Chinook jargon", "chn", ""},
  {"Chipewyan", "chp", ""},
  {"Choctaw", "cho", ""},
  {"Chuang; Zhuang", "zha", "za"},
  {"Church Slavic; Old Church Slavonic", "chu", "cu"},
  {"Old Church Slavonic; Old Slavonic; ", "chu", "cu"},
  {"Church Slavonic; Old Bulgarian; Church Slavic;", "chu", "cu"},
  {"Old Slavonic; Church Slavonic; Old Bulgarian;", "chu", "cu"},
  {"Church Slavic; Old Church Slavonic", "chu", "cu"},
  {"Chuukese", "chk", ""},
  {"Chuvash", "chv", "cv"},
  {"Coptic", "cop", ""},
  {"Cornish", "cor", "kw"},
  {"Corsican", "cos", "co",          MAKELCID( MAKELANGID(LANG_CORSICAN, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Cree", "cre", "cr"},
  {"Creek", "mus", ""},
  {"Creoles and pidgins (Other)", "crp", ""},
  {"Creoles and pidgins,", "cpe", ""},
  //   {"English-based (Other)", "", ""},
  {"Creoles and pidgins,", "cpf", ""},
  //   {"French-based (Other)", "", ""},
  {"Creoles and pidgins,", "cpp", ""},
  //   {"Portuguese-based (Other)", "", ""},
  {"Croatian", "scr", "hr",          MAKELCID( MAKELANGID(LANG_CROATIAN, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Croatian", "hrv", "hr",          MAKELCID( MAKELANGID(LANG_CROATIAN, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Cushitic (Other)", "cus", ""},
  {"Czech", "cze", "cs",            MAKELCID( MAKELANGID(LANG_CZECH, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Czech", "ces", "cs",            MAKELCID( MAKELANGID(LANG_CZECH, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Dakota", "dak", ""},
  {"Danish", "dan", "da",            MAKELCID( MAKELANGID(LANG_DANISH, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Dargwa", "dar", ""},
  {"Dayak", "day", ""},
  {"Delaware", "del", ""},
  {"Dinka", "din", ""},
  {"Divehi", "div", "dv",            MAKELCID( MAKELANGID(LANG_DIVEHI, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Dogri", "doi", ""},
  {"Dogrib", "dgr", ""},
  {"Dravidian (Other)", "dra", ""},
  {"Duala", "dua", ""},
  {"Dutch; Flemish", "dut", "nl",        MAKELCID( MAKELANGID(LANG_DUTCH, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Dutch; Flemish", "nld", "nl",        MAKELCID( MAKELANGID(LANG_DUTCH, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Dutch, Middle (ca. 1050-1350)", "dum", ""},
  {"Dyula", "dyu", ""},
  {"Dzongkha", "dzo", "dz"},
  {"Efik", "efi", ""},
  {"Egyptian (Ancient)", "egy", ""},
  {"Ekajuk", "eka", ""},
  {"Elamite", "elx", ""},
  {"English", "eng", "en",          MAKELCID( MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"English, Middle (1100-1500)", "enm", ""},
  {"English, Old (ca.450-1100)", "ang", ""},
  {"Esperanto", "epo", "eo"},
  {"Estonian", "est", "et",          MAKELCID( MAKELANGID(LANG_ESTONIAN, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Ewe", "ewe", "ee"},
  {"Ewondo", "ewo", ""},
  {"Fang", "fan", ""},
  {"Fanti", "fat", ""},
  {"Faroese", "fao", "fo",          MAKELCID( MAKELANGID(LANG_FAEROESE, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Fijian", "fij", "fj"},
  {"Finnish", "fin", "fi",          MAKELCID( MAKELANGID(LANG_FINNISH, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Finno-Ugrian (Other)", "fiu", ""},
  {"Flemish; Dutch", "dut", "nl"},
  {"Flemish; Dutch", "nld", "nl"},
  {"Fon", "fon", ""},
  {"French", "fre", "fr",            MAKELCID( MAKELANGID(LANG_FRENCH, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"French", "fra*", "fr",          MAKELCID( MAKELANGID(LANG_FRENCH, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"French", "fra", "fr",            MAKELCID( MAKELANGID(LANG_FRENCH, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"French, Middle (ca.1400-1600)", "frm", ""},
  {"French, Old (842-ca.1400)", "fro", ""},
  {"Frisian", "fry", "fy",          MAKELCID( MAKELANGID(LANG_FRISIAN, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Friulian", "fur", ""},
  {"Fulah", "ful", "ff"},
  {"Ga", "gaa", ""},
  {"Gaelic; Scottish Gaelic", "gla", "gd",  MAKELCID( MAKELANGID(LANG_GALICIAN, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Gallegan", "glg", "gl"},
  {"Ganda", "lug", "lg"},
  {"Gayo", "gay", ""},
  {"Gbaya", "gba", ""},
  {"Geez", "gez", ""},
  {"Georgian", "geo", "ka",          MAKELCID( MAKELANGID(LANG_GEORGIAN, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Georgian", "kat", "ka",          MAKELCID( MAKELANGID(LANG_GEORGIAN, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"German", "ger", "de",            MAKELCID( MAKELANGID(LANG_GERMAN, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"German", "deu", "de",            MAKELCID( MAKELANGID(LANG_GERMAN, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"German, Low; Saxon, Low; Low German; Low Saxon", "nds", ""},
  {"German, Middle High (ca.1050-1500)", "gmh", ""},
  {"German, Old High (ca.750-1050)", "goh", ""},
  {"Germanic (Other)", "gem", ""},
  {"Gikuyu; Kikuyu", "kik", "ki"},
  {"Gilbertese", "gil", ""},
  {"Gondi", "gon", ""},
  {"Gorontalo", "gor", ""},
  {"Gothic", "got", ""},
  {"Grebo", "grb", ""},
  {"Greek, Ancient (to 1453)", "grc", "",    MAKELCID( MAKELANGID(LANG_GREEK, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Greek, Modern (1453-)", "gre", "el",    MAKELCID( MAKELANGID(LANG_GREEK, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Greek, Modern (1453-)", "ell", "el",    MAKELCID( MAKELANGID(LANG_GREEK, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Greenlandic; Kalaallisut", "kal", "kl",  MAKELCID( MAKELANGID(LANG_GREENLANDIC, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Guarani", "grn", "gn"},
  {"Gujarati", "guj", "gu",          MAKELCID( MAKELANGID(LANG_GUJARATI, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Gwich´in", "gwi", ""},
  {"Haida", "hai", ""},
  {"Hausa", "hau", "ha",            MAKELCID( MAKELANGID(LANG_HAUSA, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Hawaiian", "haw", ""},
  {"Hebrew", "heb", "he",            MAKELCID( MAKELANGID(LANG_HEBREW, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Herero", "her", "hz"},
  {"Hiligaynon", "hil", ""},
  {"Himachali", "him", ""},
  {"Hindi", "hin", "hi",            MAKELCID( MAKELANGID(LANG_HINDI, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Hiri Motu", "hmo", "ho"},
  {"Hittite", "hit", ""},
  {"Hmong", "hmn", ""},
  {"Hungarian", "hun", "hu",          MAKELCID( MAKELANGID(LANG_HUNGARIAN, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Hupa", "hup", ""},
  {"Iban", "iba", ""},
  {"Icelandic", "ice", "is",          MAKELCID( MAKELANGID(LANG_ICELANDIC, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Icelandic", "isl", "is",          MAKELCID( MAKELANGID(LANG_ICELANDIC, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Ido", "ido", "io"},
  {"Igbo", "ibo", "ig",            MAKELCID( MAKELANGID(LANG_IGBO, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Ijo", "ijo", ""},
  {"Iloko", "ilo", ""},
  {"Inari Sami", "smn", ""},
  {"Indic (Other)", "inc", ""},
  {"Indo-European (Other)", "ine", ""},
  {"Indonesian", "ind", "id",          MAKELCID( MAKELANGID(LANG_INDONESIAN, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Ingush", "inh", ""},
  {"Interlingua (International", "ina", "ia"},
  //   {"Auxiliary Language Association)", "", ""},
  {"Interlingue", "ile", "ie"},
  {"Inuktitut", "iku", "iu",          MAKELCID( MAKELANGID(LANG_INUKTITUT, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Inupiaq", "ipk", "ik"},
  {"Iranian (Other)", "ira", ""},
  {"Irish", "gle", "ga",            MAKELCID( MAKELANGID(LANG_IRISH, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Irish, Middle (900-1200)", "mga", "",    MAKELCID( MAKELANGID(LANG_IRISH, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Irish, Old (to 900)", "sga", "",      MAKELCID( MAKELANGID(LANG_IRISH, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Iroquoian languages", "iro", ""},
  {"Italian", "ita", "it",          MAKELCID( MAKELANGID(LANG_ITALIAN, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Japanese", "jpn", "ja",          MAKELCID( MAKELANGID(LANG_JAPANESE, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Javanese", "jav", "jv"},
  {"Judeo-Arabic", "jrb", ""},
  {"Judeo-Persian", "jpr", ""},
  {"Kabardian", "kbd", ""},
  {"Kabyle", "kab", ""},
  {"Kachin", "kac", ""},
  {"Kalaallisut; Greenlandic", "kal", "kl"},
  {"Kamba", "kam", ""},
  {"Kannada", "kan", "kn",          MAKELCID( MAKELANGID(LANG_KANNADA, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Kanuri", "kau", "kr"},
  {"Kara-Kalpak", "kaa", ""},
  {"Karen", "kar", ""},
  {"Kashmiri", "kas", "ks",          MAKELCID( MAKELANGID(LANG_KASHMIRI, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Kawi", "kaw", ""},
  {"Kazakh", "kaz", "kk",            MAKELCID( MAKELANGID(LANG_KAZAK, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Khasi", "kha", ""},
  {"Khmer", "khm", "km",            MAKELCID( MAKELANGID(LANG_KHMER, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Khoisan (Other)", "khi", ""},
  {"Khotanese", "kho", ""},
  {"Kikuyu; Gikuyu", "kik", "ki"},
  {"Kimbundu", "kmb", ""},
  {"Kinyarwanda", "kin", "rw",        MAKELCID( MAKELANGID(LANG_KINYARWANDA, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Kirghiz", "kir", "ky"},
  {"Komi", "kom", "kv"},
  {"Kongo", "kon", "kg"},
  {"Konkani", "kok", "",            MAKELCID( MAKELANGID(LANG_KONKANI, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Korean", "kor", "ko",            MAKELCID( MAKELANGID(LANG_KOREAN, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Kosraean", "kos", ""},
  {"Kpelle", "kpe", ""},
  {"Kru", "kro", ""},
  {"Kuanyama; Kwanyama", "kua", "kj"},
  {"Kumyk", "kum", ""},
  {"Kurdish", "kur", "ku"},
  {"Kurukh", "kru", ""},
  {"Kutenai", "kut", ""},
  {"Kwanyama, Kuanyama", "kua", "kj"},
  {"Ladino", "lad", ""},
  {"Lahnda", "lah", ""},
  {"Lamba", "lam", ""},
  {"Lao", "lao", "lo",            MAKELCID( MAKELANGID(LANG_LAO, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Latin", "lat", "la"},
  {"Latvian", "lav", "lv",          MAKELCID( MAKELANGID(LANG_LATVIAN, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Letzeburgesch; Luxembourgish", "ltz", "lb"},
  {"Lezghian", "lez", ""},
  {"Limburgan; Limburger; Limburgish", "lim", "li"},
  {"Limburger; Limburgan; Limburgish;", "lim", "li"},
  {"Limburgish; Limburger; Limburgan", "lim", "li"},
  {"Lingala", "lin", "ln"},
  {"Lithuanian", "lit", "lt",          MAKELCID( MAKELANGID(LANG_LITHUANIAN, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Low German; Low Saxon; German, Low; Saxon, Low", "nds", ""},
  {"Low Saxon; Low German; Saxon, Low; German, Low", "nds", ""},
  {"Lozi", "loz", ""},
  {"Luba-Katanga", "lub", "lu"},
  {"Luba-Lulua", "lua", ""},
  {"Luiseno", "lui", ""},
  {"Lule Sami", "smj", ""},
  {"Lunda", "lun", ""},
  {"Luo (Kenya and Tanzania)", "luo", ""},
  {"Lushai", "lus", ""},
  {"Luxembourgish; Letzeburgesch", "ltz", "lb",  MAKELCID( MAKELANGID(LANG_LUXEMBOURGISH, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Macedonian", "mac", "mk",          MAKELCID( MAKELANGID(LANG_MACEDONIAN, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Macedonian", "mkd", "mk",          MAKELCID( MAKELANGID(LANG_MACEDONIAN, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Madurese", "mad", ""},
  {"Magahi", "mag", ""},
  {"Maithili", "mai", ""},
  {"Makasar", "mak", ""},
  {"Malagasy", "mlg", "mg"},
  {"Malay", "may", "ms",            MAKELCID( MAKELANGID(LANG_MALAY, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Malay", "msa", "ms",            MAKELCID( MAKELANGID(LANG_MALAY, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Malayalam", "mal", "ml",          MAKELCID( MAKELANGID(LANG_MALAYALAM, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Maltese", "mlt", "mt",          MAKELCID( MAKELANGID(LANG_MALTESE, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Manchu", "mnc", ""},
  {"Mandar", "mdr", ""},
  {"Mandingo", "man", ""},
  {"Manipuri", "mni", "",            MAKELCID( MAKELANGID(LANG_MANIPURI, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Manobo languages", "mno", ""},
  {"Manx", "glv", "gv"},
  {"Maori", "mao", "mi",            MAKELCID( MAKELANGID(LANG_MAORI, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Maori", "mri", "mi",            MAKELCID( MAKELANGID(LANG_MAORI, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Marathi", "mar", "mr",          MAKELCID( MAKELANGID(LANG_MARATHI, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Mari", "chm", ""},
  {"Marshallese", "mah", "mh"},
  {"Marwari", "mwr", ""},
  {"Masai", "mas", ""},
  {"Mayan languages", "myn", ""},
  {"Mende", "men", ""},
  {"Micmac", "mic", ""},
  {"Minangkabau", "min", ""},
  {"Miscellaneous languages", "mis", ""},
  {"Mohawk", "moh", "",            MAKELCID( MAKELANGID(LANG_MOHAWK, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Moldavian", "mol", "mo"},
  {"Mon-Khmer (Other)", "mkh", ""},
  {"Mongo", "lol", ""},
  {"Mongolian", "mon", "mn",          MAKELCID( MAKELANGID(LANG_MONGOLIAN, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Mossi", "mos", ""},
  {"Multiple languages", "mul", ""},
  {"Munda languages", "mun", ""},
  {"Nahuatl", "nah", ""},
  {"Nauru", "nau", "na"},
  {"Navaho, Navajo", "nav", "nv"},
  {"Navajo; Navaho", "nav", "nv"},
  {"Ndebele, North", "nde", "nd"},
  {"Ndebele, South", "nbl", "nr"},
  {"Ndonga", "ndo", "ng"},
  {"Neapolitan", "nap", ""},
  {"Nepali", "nep", "ne",            MAKELCID( MAKELANGID(LANG_NEPALI, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Newari", "new", ""},
  {"Nias", "nia", ""},
  {"Niger-Kordofanian (Other)", "nic", ""},
  {"Nilo-Saharan (Other)", "ssa", ""},
  {"Niuean", "niu", ""},
  {"Norse, Old", "non", ""},
  {"North American Indian (Other)", "nai", ""},
  {"Northern Sami", "sme", "se"},
  {"North Ndebele", "nde", "nd"},
  {"Norwegian", "nor", "no",          MAKELCID( MAKELANGID(LANG_NORWEGIAN, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Norwegian Bokmål; Bokmål, Norwegian", "nob", "nb",  MAKELCID( MAKELANGID(LANG_NORWEGIAN, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Norwegian Nynorsk; Nynorsk, Norwegian", "nno", "nn",  MAKELCID( MAKELANGID(LANG_NORWEGIAN, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Nubian languages", "nub", ""},
  {"Nyamwezi", "nym", ""},
  {"Nyanja; Chichewa; Chewa", "nya", "ny"},
  {"Nyankole", "nyn", ""},
  {"Nynorsk, Norwegian; Norwegian Nynorsk", "nno", "nn"},
  {"Nyoro", "nyo", ""},
  {"Nzima", "nzi", ""},
  {"Occitan (post 1500},; Provençal", "oci", "oc",    MAKELCID( MAKELANGID(LANG_OCCITAN, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Ojibwa", "oji", "oj"},
  {"Old Bulgarian; Old Slavonic; Church Slavonic;", "chu", "cu"},
  {"Oriya", "ori", "or"},
  {"Oromo", "orm", "om"},
  {"Osage", "osa", ""},
  {"Ossetian; Ossetic", "oss", "os"},
  {"Ossetic; Ossetian", "oss", "os"},
  {"Otomian languages", "oto", ""},
  {"Pahlavi", "pal", ""},
  {"Palauan", "pau", ""},
  {"Pali", "pli", "pi"},
  {"Pampanga", "pam", ""},
  {"Pangasinan", "pag", ""},
  {"Panjabi", "pan", "pa"},
  {"Papiamento", "pap", ""},
  {"Papuan (Other)", "paa", ""},
  {"Persian", "per", "fa",        MAKELCID( MAKELANGID(LANG_PERSIAN, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Persian", "fas", "fa",        MAKELCID( MAKELANGID(LANG_PERSIAN, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Persian, Old (ca.600-400 B.C.)", "peo", ""},
  {"Philippine (Other)", "phi", ""},
  {"Phoenician", "phn", ""},
  {"Pohnpeian", "pon", ""},
  {"Polish", "pol", "pl",          MAKELCID( MAKELANGID(LANG_POLISH, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Portuguese", "por", "pt",        MAKELCID( MAKELANGID(LANG_PORTUGUESE, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Prakrit languages", "pra", ""},
  {"Provençal; Occitan (post 1500)", "oci", "oc"},
  {"Provençal, Old (to 1500)", "pro", ""},
  {"Pushto", "pus", "ps"},
  {"Quechua", "que", "qu",        MAKELCID( MAKELANGID(LANG_QUECHUA, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Raeto-Romance", "roh", "rm"},
  {"Rajasthani", "raj", ""},
  {"Rapanui", "rap", ""},
  {"Rarotongan", "rar", ""},
  {"Reserved for local use", "qaa-qtz", ""},
  {"Romance (Other)", "roa", ""},
  {"Romanian", "rum", "ro",        MAKELCID( MAKELANGID(LANG_ROMANIAN, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Romanian", "ron", "ro",        MAKELCID( MAKELANGID(LANG_ROMANIAN, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Romany", "rom", ""},
  {"Rundi", "run", "rn"},
  {"Russian", "rus", "ru",        MAKELCID( MAKELANGID(LANG_RUSSIAN, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Salishan languages", "sal", ""},
  {"Samaritan Aramaic", "sam", ""},
  {"Sami languages (Other)", "smi", ""},
  {"Samoan", "smo", "sm"},
  {"Sandawe", "sad", ""},
  {"Sango", "sag", "sg"},
  {"Sanskrit", "san", "sa",        MAKELCID( MAKELANGID(LANG_SANSKRIT, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Santali", "sat", ""},
  {"Sardinian", "srd", "sc"},
  {"Sasak", "sas", ""},
  {"Saxon, Low; German, Low; Low Saxon; Low German", "nds", ""},
  {"Scots", "sco", ""},
  {"Scottish Gaelic; Gaelic", "gla", "gd"},
  {"Selkup", "sel", ""},
  {"Semitic (Other)", "sem", ""},
  {"Serbian", "scc", "sr",        MAKELCID( MAKELANGID(LANG_SERBIAN, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Serbian", "srp", "sr",        MAKELCID( MAKELANGID(LANG_SERBIAN, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Serer", "srr", ""},
  {"Shan", "shn", ""},
  {"Shona", "sna", "sn"},
  {"Sichuan Yi", "iii", "ii"},
  {"Sidamo", "sid", ""},
  {"Sign languages", "sgn", ""},
  {"Siksika", "bla", ""},
  {"Sindhi", "snd", "sd",          MAKELCID( MAKELANGID(LANG_SINDHI, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Sinhalese", "sin", "si",        MAKELCID( MAKELANGID(LANG_SINHALESE, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Sino-Tibetan (Other)", "sit", ""},
  {"Siouan languages", "sio", ""},
  {"Skolt Sami", "sms", ""},
  {"Slave (Athapascan)", "den", ""},
  {"Slavic (Other)", "sla", ""},
  {"Slovak", "slo", "sk",          MAKELCID( MAKELANGID(LANG_SLOVAK, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Slovak", "slk", "sk",          MAKELCID( MAKELANGID(LANG_SLOVAK, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Slovenian", "slv", "sl",        MAKELCID( MAKELANGID(LANG_SLOVENIAN, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Sogdian", "sog", ""},
  {"Somali", "som", "so"},
  {"Songhai", "son", ""},
  {"Soninke", "snk", ""},
  {"Sorbian languages", "wen", ""},
  {"Sotho, Northern", "nso", "",      MAKELCID( MAKELANGID(LANG_SOTHO, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Sotho, Southern", "sot", "st",    MAKELCID( MAKELANGID(LANG_SOTHO, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"South American Indian (Other)", "sai", ""},
  {"Southern Sami", "sma", ""},
  {"South Ndebele", "nbl", "nr"},
  {"Spanish; Castilian", "spa", "es",    MAKELCID( MAKELANGID(LANG_SPANISH, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Sukuma", "suk", ""},
  {"Sumerian", "sux", ""},
  {"Sundanese", "sun", "su"},
  {"Susu", "sus", ""},
  {"Swahili", "swa", "sw",        MAKELCID( MAKELANGID(LANG_SWAHILI, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Swati", "ssw", "ss"},
  {"Swedish", "swe", "sv",        MAKELCID( MAKELANGID(LANG_SWEDISH, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Syriac", "syr", "",          MAKELCID( MAKELANGID(LANG_SYRIAC, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Tagalog", "tgl", "tl"},
  {"Tahitian", "tah", "ty"},
  {"Tai (Other)", "tai", ""},
  {"Tajik", "tgk", "tg",          MAKELCID( MAKELANGID(LANG_TAJIK, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Tamashek", "tmh", ""},
  {"Tamil", "tam", "ta",          MAKELCID( MAKELANGID(LANG_TAMIL, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Tatar", "tat", "tt",          MAKELCID( MAKELANGID(LANG_TATAR, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Telugu", "tel", "te",          MAKELCID( MAKELANGID(LANG_TELUGU, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Tereno", "ter", ""},
  {"Tetum", "tet", ""},
  {"Thai", "tha", "th",          MAKELCID( MAKELANGID(LANG_THAI, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Tibetan", "tib", "bo",        MAKELCID( MAKELANGID(LANG_TIBETAN, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Tibetan", "bod", "bo",        MAKELCID( MAKELANGID(LANG_TIBETAN, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Tigre", "tig", ""},
  {"Tigrinya", "tir", "ti",        MAKELCID( MAKELANGID(LANG_TIGRIGNA, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Timne", "tem", ""},
  {"Tiv", "tiv", ""},
  {"Tlingit", "tli", ""},
  {"Tok Pisin", "tpi", ""},
  {"Tokelau", "tkl", ""},
  {"Tonga (Nyasa)", "tog", ""},
  {"Tonga (Tonga Islands)", "ton", "to"},
  {"Tsimshian", "tsi", ""},
  {"Tsonga", "tso", "ts"},
  {"Tswana", "tsn", "tn",          MAKELCID( MAKELANGID(LANG_TSWANA, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Tumbuka", "tum", ""},
  {"Tupi languages", "tup", ""},
  {"Turkish", "tur", "tr",        MAKELCID( MAKELANGID(LANG_TURKISH, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Turkish, Ottoman (1500-1928)", "ota", "",  MAKELCID( MAKELANGID(LANG_TURKISH, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Turkmen", "tuk", "tk",        MAKELCID( MAKELANGID(LANG_TURKMEN, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Tuvalu", "tvl", ""},
  {"Tuvinian", "tyv", ""},
  {"Twi", "twi", "tw"},
  {"Ugaritic", "uga", ""},
  {"Uighur", "uig", "ug",          MAKELCID( MAKELANGID(LANG_UIGHUR, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Ukrainian", "ukr", "uk",        MAKELCID( MAKELANGID(LANG_UKRAINIAN, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Umbundu", "umb", ""},
  {"Undetermined", "und", ""},
  {"Urdu", "urd", "ur",          MAKELCID( MAKELANGID(LANG_URDU, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Uzbek", "uzb", "uz",          MAKELCID( MAKELANGID(LANG_UZBEK, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Vai", "vai", ""},
  {"Venda", "ven", "ve"},
  {"Vietnamese", "vie", "vi",        MAKELCID( MAKELANGID(LANG_VIETNAMESE, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Volapük", "vol", "vo"},
  {"Votic", "vot", ""},
  {"Wakashan languages", "wak", ""},
  {"Walamo", "wal", ""},
  {"Walloon", "wln", "wa"},
  {"Waray", "war", ""},
  {"Washo", "was", ""},
  {"Welsh", "wel", "cy",          MAKELCID( MAKELANGID(LANG_WELSH, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Welsh", "cym", "cy",          MAKELCID( MAKELANGID(LANG_WELSH, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Wolof", "wol", "wo",          MAKELCID( MAKELANGID(LANG_WOLOF, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Xhosa", "xho", "xh",          MAKELCID( MAKELANGID(LANG_XHOSA, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Yakut", "sah", "",          MAKELCID( MAKELANGID(LANG_YAKUT, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Yao", "yao", ""},
  {"Yapese", "yap", ""},
  {"Yiddish", "yid", "yi"},
  {"Yoruba", "yor", "yo",          MAKELCID( MAKELANGID(LANG_YORUBA, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Yupik languages", "ypk", ""},
  {"Zande", "znd", ""},
  {"Zapotec", "zap", ""},
  {"Zenaga", "zen", ""},
  {"Zhuang; Chuang", "zha", "za"},
  {"Zulu", "zul", "zu",          MAKELCID( MAKELANGID(LANG_ZULU, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Zuni", "zun", ""},
  {"Classical Newari", "nwc", ""},
  {"Klingon", "tlh", ""},
  {"Blin", "byn", ""},
  {"Lojban", "jbo", ""},
  {"Lower Sorbian", "dsb", ""},
  {"Upper Sorbian", "hsb", ""},
  {"Kashubian", "csb", ""},
  {"Crimean Turkish", "crh", ""},
  {"Erzya", "myv", ""},
  {"Moksha", "mdf", ""},
  {"Karachay-Balkar", "krc", ""},
  {"Adyghe", "ady", ""},
  {"Udmurt", "udm", ""},
  {"Dargwa", "dar", ""},
  {"Ingush", "inh", ""},
  {"Nogai", "nog", ""},
  {"Haitian", "hat", "ht"},
  {"Kalmyk", "xal", ""},
  {"", "", ""},
  {"No subtitles", "---", "", LCID_NOSUBTITLES},
};

CStdString ISO6392ToLanguage(LPCSTR code)
{
  CHAR tmp[3+1];
  strncpy_s(tmp, code, 3);
  tmp[3] = 0;
  _strlwr_s(tmp);
  for(int i = 0, j = countof(s_isolangs); i < j; i++)
  {
    if(!strcmp(s_isolangs[i].iso6392, tmp))
    {
      CStdString ret = CStdString(CStdStringA(s_isolangs[i].name));
      int i = ret.Find(';');
      if(i > 0) ret = ret.Left(i);
      return ret;
    }
  }
  return _T("");
}

LCID ISO6391ToLcid(LPCSTR code)
{
  CHAR tmp[3+1];
  strncpy_s(tmp, code, 3);
  tmp[3] = 0;
  _strlwr_s(tmp);
  for(int i = 0, j = countof(s_isolangs); i < j; i++)
  {
    if(!strcmp(s_isolangs[i].iso6391, code))
    {
      return s_isolangs[i].lcid;
    }
  }
  return 0;
}

LCID ISO6392ToLcid(LPCSTR code)
{
  CHAR tmp[3+1];
  strncpy_s(tmp, code, 3);
  tmp[3] = 0;
  _strlwr_s(tmp);
  for(int i = 0, j = countof(s_isolangs); i < j; i++)
  {
    if(!strcmp(s_isolangs[i].iso6392, tmp))
    {
      return s_isolangs[i].lcid;
    }
  }
  return 0;
}

const double Rec601_Kr = 0.299;
const double Rec601_Kb = 0.114;
const double Rec601_Kg = 0.587;
COLORREF YCrCbToRGB_Rec601(BYTE Y, BYTE Cr, BYTE Cb)
{

  double rp = Y + 2*(Cr-128)*(1.0-Rec601_Kr);
  double gp = Y - 2*(Cb-128)*(1.0-Rec601_Kb)*Rec601_Kb/Rec601_Kg - 2*(Cr-128)*(1.0-Rec601_Kr)*Rec601_Kr/Rec601_Kg;
  double bp = Y + 2*(Cb-128)*(1.0-Rec601_Kb);

  return RGB (fabs(rp), fabs(gp), fabs(bp));
}

DWORD YCrCbToRGB_Rec601(BYTE A, BYTE Y, BYTE Cr, BYTE Cb)
{

  double rp = Y + 2*(Cr-128)*(1.0-Rec601_Kr);
  double gp = Y - 2*(Cb-128)*(1.0-Rec601_Kb)*Rec601_Kb/Rec601_Kg - 2*(Cr-128)*(1.0-Rec601_Kr)*Rec601_Kr/Rec601_Kg;
  double bp = Y + 2*(Cb-128)*(1.0-Rec601_Kb);

  return D3DCOLOR_ARGB(A, (BYTE)fabs(rp), (BYTE)fabs(gp), (BYTE)fabs(bp));
}


const double Rec709_Kr = 0.2125;
const double Rec709_Kb = 0.0721;
const double Rec709_Kg = 0.7154;
COLORREF YCrCbToRGB_Rec709(BYTE Y, BYTE Cr, BYTE Cb)
{

  double rp = Y + 2*(Cr-128)*(1.0-Rec709_Kr);
  double gp = Y - 2*(Cb-128)*(1.0-Rec709_Kb)*Rec709_Kb/Rec709_Kg - 2*(Cr-128)*(1.0-Rec709_Kr)*Rec709_Kr/Rec709_Kg;
  double bp = Y + 2*(Cb-128)*(1.0-Rec709_Kb);

  return RGB (fabs(rp), fabs(gp), fabs(bp));
}

DWORD YCrCbToRGB_Rec709(BYTE A, BYTE Y, BYTE Cr, BYTE Cb)
{

  double rp = Y + 2*(Cr-128)*(1.0-Rec709_Kr);
  double gp = Y - 2*(Cb-128)*(1.0-Rec709_Kb)*Rec709_Kb/Rec709_Kg - 2*(Cr-128)*(1.0-Rec709_Kr)*Rec709_Kr/Rec709_Kg;
  double bp = Y + 2*(Cb-128)*(1.0-Rec709_Kb);

  return D3DCOLOR_ARGB (A, (BYTE)fabs(rp), (BYTE)fabs(gp), (BYTE)fabs(bp));
}

void memsetw(void* dst, unsigned short c, size_t nbytes)
{
  memsetd(dst, c << 16 | c, nbytes);

  size_t n = nbytes / 2;
  size_t o = (n / 2) * 2;
  if ((n - o) == 1)
    ((WORD*)dst)[o] = c;
}
