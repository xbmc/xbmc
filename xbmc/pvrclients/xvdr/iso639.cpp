/*
 *      Copyright (C) 2011 Alexander Pipelka
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

#include <string.h>
#include "iso639.h"

// language code structure
typedef struct _ISO639
{
	const char* lang;
	const char* code;
} ISO639;

static ISO639 ISO639_map[] =
{
  {"aar", "aa"},
  {"abk", "ab"},
  {"afr", "af"},
  {"aka", "ak"},
  {"alb", "sq"},
  {"amh", "am"},
  {"ara", "ar"},
  {"arg", "an"},
  {"asm", "as"},
  {"ava", "av"},
  {"ave", "ae"},
  {"aym", "ay"},
  {"aze", "az"},
  {"bak", "ba"},
  {"bam", "bm"},
  {"bel", "be"},
  {"ben", "bn"},
  {"bih", "bh"},
  {"bis", "bi"},
  {"bod", "bo"},
  {"bos", "bs"},
  {"bre", "br"},
  {"bul", "bg"},
  {"cat", "ca"},
  {"cha", "ch"},
  {"che", "ce"},
  {"chu", "cu"},
  {"cor", "kw"},
  {"cos", "co"},
  {"cre", "cr"},
  {"cym", "cy"},
  {"cze", "cs"},
  {"dan", "da"},
  {"deu", "de"},
  {"div", "dv"},
  {"dut", "nl"},
  {"dzo", "dz"},
  {"ell", "el"},
  {"eng", "en"},
  {"epo", "eo"},
  {"est", "et"},
  {"eus", "eu"},
  {"ewe", "ee"},
  {"fao", "fo"},
  {"fas", "fa"},
  {"fij", "fj"},
  {"fin", "fi"},
  {"fra", "fr"},
  {"fry", "fy"},
  {"ful", "ff"},
  {"gla", "gd"},
  {"gle", "ga"},
  {"glg", "gl"},
  {"glv", "gv"},
  {"gre", "el"},
  {"grn", "gn"},
  {"guj", "gu"},
  {"hat", "ht"},
  {"hau", "ha"},
  {"heb", "he"},
  {"her", "hz"},
  {"hin", "hi"},
  {"hmo", "ho"},
  {"hrv", "hr"},
  {"hun", "hu"},
  {"hye", "hy"},
  {"ibo", "ig"},
  {"ido", "io"},
  {"iii", "ii"},
  {"iku", "iu"},
  {"ile", "ie"},
  {"ina", "ia"},
  {"ind", "id"},
  {"ipk", "ik"},
  {"isl", "is"},
  {"ita", "it"},
  {"jav", "jv"},
  {"jpn", "ja"},
  {"kal", "kl"},
  {"kan", "kn"},
  {"kas", "ks"},
  {"kat", "ka"},
  {"kau", "kr"},
  {"kaz", "kk"},
  {"khm", "km"},
  {"kik", "ki"},
  {"kin", "rw"},
  {"kir", "ky"},
  {"kom", "kv"},
  {"kon", "kg"},
  {"kor", "ko"},
  {"kua", "kj"},
  {"kur", "ku"},
  {"lao", "lo"},
  {"lat", "la"},
  {"lav", "lv"},
  {"lim", "li"},
  {"lin", "ln"},
  {"lit", "lt"},
  {"ltz", "lb"},
  {"lub", "lu"},
  {"lug", "lg"},
  {"mah", "mh"},
  {"mal", "ml"},
  {"mar", "mr"},
  {"mkd", "mk"},
  {"mlg", "mg"},
  {"mlt", "mt"},
  {"mon", "mn"},
  {"mri", "mi"},
  {"msa", "ms"},
  {"mya", "my"},
  {"nau", "na"},
  {"nav", "nv"},
  {"nbl", "nr"},
  {"nde", "nd"},
  {"ndo", "ng"},
  {"nep", "ne"},
  {"nld", "nl"},
  {"nno", "nn"},
  {"nob", "nb"},
  {"nor", "no"},
  {"nya", "ny"},
  {"oci", "oc"},
  {"oji", "oj"},
  {"ori", "or"},
  {"orm", "om"},
  {"oss", "os"},
  {"pan", "pa"},
  {"pli", "pi"},
  {"pol", "pl"},
  {"por", "pt"},
  {"pus", "ps"},
  {"que", "qu"},
  {"roh", "rm"},
  {"ron", "ro"},
  {"run", "rn"},
  {"rus", "ru"},
  {"sag", "sg"},
  {"san", "sa"},
  {"sin", "si"},
  {"slk", "sk"},
  {"slv", "sl"},
  {"sme", "se"},
  {"smo", "sm"},
  {"sna", "sn"},
  {"snd", "sd"},
  {"som", "so"},
  {"sot", "st"},
  {"sqi", "sq"},
  {"srd", "sc"},
  {"srp", "sr"},
  {"ssw", "ss"},
  {"sun", "su"},
  {"swa", "sw"},
  {"swe", "sv"},
  {"tah", "ty"},
  {"tam", "ta"},
  {"tat", "tt"},
  {"tel", "te"},
  {"tgk", "tg"},
  {"tgl", "tl"},
  {"tha", "th"},
  {"tir", "ti"},
  {"ton", "to"},
  {"tsn", "tn"},
  {"tso", "ts"},
  {"tuk", "tk"},
  {"tur", "tr"},
  {"twi", "tw"},
  {"uig", "ug"},
  {"ukr", "uk"},
  {"urd", "ur"},
  {"uzb", "uz"},
  {"ven", "ve"},
  {"vie", "vi"},
  {"vol", "vo"},
  {"wln", "wa"},
  {"wol", "wo"},
  {"xho", "xh"},
  {"yid", "yi"},
  {"yor", "yo"},
  {"zha", "za"},
  {"zho", "zh"},
  {"zul", "zu"}
};


const char* ISO639_FindLanguage(const char* code)
{
  if(code == NULL)
    return NULL;

  for(int i = 0; i < sizeof(ISO639_map) / sizeof(ISO639); i++)
  {
	  if(strcmp(ISO639_map[i].code, code) == 0) {
		  return ISO639_map[i].lang;
	  }
  }
  return NULL;
}
