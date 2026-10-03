/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <algorithm>
#include <array>
#include <string_view>

struct ISO639
{
  std::string_view iso639_1;
  std::string_view iso639_2b;
};

// Legacy ISO 639 table - unclear source
// Sorted by alpha-2

inline static constexpr int LANGUAGE_CODES_COUNT = 190;

// clang-format off
inline constexpr std::array<ISO639, LANGUAGE_CODES_COUNT> LanguageCodes = {{
    // The four special-scope ISO 639-2 codes have no ISO 639-1 code, and sort first for it
    {"", "und"}, // Undetermined
    {"", "zxx"}, // No linguistic content
    {"", "mis"}, // Uncoded languages
    {"", "mul"}, // Multiple languages
    {"aa", "aar"},
    {"ab", "abk"},
    {"ae", "ave"},
    {"af", "afr"},
    {"ak", "aka"},
    {"am", "amh"},
    {"an", "arg"},
    {"ar", "ara"},
    {"as", "asm"},
    {"av", "ava"},
    {"ay", "aym"},
    {"az", "aze"},
    {"ba", "bak"},
    {"be", "bel"},
    {"bg", "bul"},
    {"bh", "bih"},
    {"bi", "bis"},
    {"bm", "bam"},
    {"bn", "ben"},
    {"bo", "tib"},
    {"br", "bre"},
    {"bs", "bos"},
    {"ca", "cat"},
    {"ce", "che"},
    {"ch", "cha"},
    {"co", "cos"},
    {"cr", "cre"},
    {"cs", "cze"},
    {"cu", "chu"},
    {"cv", "chv"},
    {"cy", "wel"},
    {"da", "dan"},
    {"de", "ger"},
    {"dv", "div"},
    {"dz", "dzo"},
    {"ee", "ewe"},
    {"el", "gre"},
    {"en", "eng"},
    {"eo", "epo"},
    {"es", "spa"},
    {"et", "est"},
    {"eu", "baq"},
    {"fa", "per"},
    {"ff", "ful"},
    {"fi", "fin"},
    {"fj", "fij"},
    {"fo", "fao"},
    {"fr", "fre"},
    {"fy", "fry"},
    {"ga", "gle"},
    {"gd", "gla"},
    {"gl", "glg"},
    {"gn", "grn"},
    {"gu", "guj"},
    {"gv", "glv"},
    {"ha", "hau"},
    {"he", "heb"},
    {"hi", "hin"},
    {"ho", "hmo"},
    {"hr", "hrv"},
    {"ht", "hat"},
    {"hu", "hun"},
    {"hy", "arm"},
    {"hz", "her"},
    {"ia", "ina"},
    {"id", "ind"},
    {"ie", "ile"},
    {"ig", "ibo"},
    {"ii", "iii"},
    {"ik", "ipk"},
    {"io", "ido"},
    {"is", "ice"},
    {"it", "ita"},
    {"iu", "iku"},
    {"ja", "jpn"},
    {"jv", "jav"},
    {"ka", "geo"},
    {"kg", "kon"},
    {"ki", "kik"},
    {"kj", "kua"},
    {"kk", "kaz"},
    {"kl", "kal"},
    {"km", "khm"},
    {"kn", "kan"},
    {"ko", "kor"},
    {"kr", "kau"},
    {"ks", "kas"},
    {"ku", "kur"},
    {"kv", "kom"},
    {"kw", "cor"},
    {"ky", "kir"},
    {"la", "lat"},
    {"lb", "ltz"},
    {"lg", "lug"},
    {"li", "lim"},
    {"ln", "lin"},
    {"lo", "lao"},
    {"lt", "lit"},
    {"lu", "lub"},
    {"lv", "lav"},
    {"mg", "mlg"},
    {"mh", "mah"},
    {"mi", "mao"},
    {"mk", "mac"},
    {"ml", "mal"},
    {"mn", "mon"},
    {"mr", "mar"},
    {"ms", "may"},
    {"mt", "mlt"},
    {"my", "bur"},
    {"na", "nau"},
    {"nb", "nob"},
    {"nd", "nde"},
    {"ne", "nep"},
    {"ng", "ndo"},
    {"nl", "dut"},
    {"nn", "nno"},
    {"no", "nor"},
    {"nr", "nbl"},
    {"nv", "nav"},
    {"ny", "nya"},
    {"oc", "oci"},
    {"oj", "oji"},
    {"om", "orm"},
    {"or", "ori"},
    {"os", "oss"},
    {"pa", "pan"},
    // pb / pob = unofficial language code for Brazilian Portuguese
    {"pb", "pob"},
    {"pi", "pli"},
    {"pl", "pol"},
    {"ps", "pus"},
    {"pt", "por"},
    {"qu", "que"},
    {"rm", "roh"},
    {"rn", "run"},
    {"ro", "rum"},
    {"ru", "rus"},
    {"rw", "kin"},
    {"sa", "san"},
    {"sc", "srd"},
    {"sd", "snd"},
    {"se", "sme"},
    {"sg", "sag"},
    {"sh", "scr"},
    {"si", "sin"},
    {"sk", "slo"},
    {"sl", "slv"},
    {"sm", "smo"},
    {"sn", "sna"},
    {"so", "som"},
    {"sq", "alb"},
    {"sr", "srp"},
    {"ss", "ssw"},
    {"st", "sot"},
    {"su", "sun"},
    {"sv", "swe"},
    {"sw", "swa"},
    {"ta", "tam"},
    {"te", "tel"},
    {"tg", "tgk"},
    {"th", "tha"},
    {"ti", "tir"},
    {"tk", "tuk"},
    {"tl", "tgl"},
    {"tn", "tsn"},
    {"to", "ton"},
    {"tr", "tur"},
    {"ts", "tso"},
    {"tt", "tat"},
    {"tw", "twi"},
    {"ty", "tah"},
    {"ug", "uig"},
    {"uk", "ukr"},
    {"ur", "urd"},
    {"uz", "uzb"},
    {"ve", "ven"},
    {"vi", "vie"},
    {"vo", "vol"},
    {"wa", "wln"},
    {"wo", "wol"},
    {"xh", "xho"},
    {"yi", "yid"},
    {"yo", "yor"},
    {"za", "zha"},
    {"zh", "chi"},
    {"zu", "zul"},
}};
// clang-format on

static_assert(std::ranges::is_sorted(LanguageCodes, {}, &ISO639::iso639_1));

// ISO 639-1 withdrew these codes. Media tagged with the old spelling still has to be understood,
// so they live here rather than in the table above, whose ISO 639-2/B codes have to stay unique
// for the reverse lookup to have one answer per language.
// Sorted by alpha-2
inline constexpr auto DeprecatedLanguageCodes = std::array<ISO639, 5>{{
    {"in", "ind"}, // Indonesian, now id
    {"iw", "heb"}, // Hebrew, now he
    {"ji", "yid"}, // Yiddish, now yi
    {"jw", "jav"}, // Javanese, now jv
    {"mo", "rum"}, // Moldavian, merged into Romanian (ro) in 2008
}};

static_assert(std::ranges::is_sorted(DeprecatedLanguageCodes, {}, &ISO639::iso639_1));

constexpr auto CreateLanguageCodesSortedByIso639_2b()
{
  auto codes{LanguageCodes};
  std::ranges::sort(codes, {}, &ISO639::iso639_2b);
  return codes;
}

inline constexpr auto LanguageCodesByIso639_2b = CreateLanguageCodesSortedByIso639_2b();

// The sort above is not stable, so a repeated ISO 639-2/B code would resolve to whichever row the
// compiler happened to place first - a language exported differently by different builds.
static_assert(std::ranges::adjacent_find(LanguageCodesByIso639_2b, {}, &ISO639::iso639_2b) ==
              LanguageCodesByIso639_2b.end());
