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
  std::string_view iso639_2t;
  std::string_view win_id;
};

// Legacy ISO 639 table - unclear source
// Sorted by alpha-2

inline static constexpr int LANGUAGE_CODES_COUNT = 192;

// clang-format off
inline constexpr std::array<ISO639, LANGUAGE_CODES_COUNT> LanguageCodes = {{
    {"aa", "aar", "", ""},
    {"ab", "abk", "", ""},
    {"ae", "ave", "", ""},
    {"af", "afr", "", ""},
    {"ak", "aka", "", ""},
    {"am", "amh", "", ""},
    {"an", "arg", "", ""},
    {"ar", "ara", "", ""},
    {"as", "asm", "", ""},
    {"av", "ava", "", ""},
    {"ay", "aym", "", ""},
    {"az", "aze", "", ""},
    {"ba", "bak", "", ""},
    {"be", "bel", "", ""},
    {"bg", "bul", "", ""},
    {"bh", "bih", "", ""},
    {"bi", "bis", "", ""},
    {"bm", "bam", "", ""},
    {"bn", "ben", "", ""},
    {"bo", "tib", "", "bod"},
    {"br", "bre", "", ""},
    {"bs", "bos", "", ""},
    {"ca", "cat", "", ""},
    {"ce", "che", "", ""},
    {"ch", "cha", "", ""},
    {"co", "cos", "", ""},
    {"cr", "cre", "", ""},
    {"cs", "cze", "ces", "ces"},
    {"cu", "chu", "", ""},
    {"cv", "chv", "", ""},
    {"cy", "wel", "", "cym"},
    {"da", "dan", "", ""},
    {"de", "ger", "deu", "deu"},
    {"dv", "div", "", ""},
    {"dz", "dzo", "", ""},
    {"ee", "ewe", "", ""},
    {"el", "gre", "ell", "ell"},
    {"en", "eng", "", ""},
    {"eo", "epo", "", ""},
    {"es", "spa", "esp", ""},
    {"et", "est", "", ""},
    {"eu", "baq", "", "eus"},
    {"fa", "per", "", "fas"},
    {"ff", "ful", "", ""},
    {"fi", "fin", "", ""},
    {"fj", "fij", "", ""},
    {"fo", "fao", "", ""},
    {"fr", "fre", "fra", "fra"},
    {"fy", "fry", "", ""},
    {"ga", "gle", "", ""},
    {"gd", "gla", "", ""},
    {"gl", "glg", "", ""},
    {"gn", "grn", "", ""},
    {"gu", "guj", "", ""},
    {"gv", "glv", "", ""},
    {"ha", "hau", "", ""},
    {"he", "heb", "", ""},
    {"hi", "hin", "", ""},
    {"ho", "hmo", "", ""},
    {"hr", "hrv", "", ""},
    {"ht", "hat", "", ""},
    {"hu", "hun", "", ""},
    {"hy", "arm", "", "hye"},
    {"hz", "her", "", ""},
    {"ia", "ina", "", ""},
    {"id", "ind", "", ""},
    {"ie", "ile", "", ""},
    {"ig", "ibo", "", ""},
    {"ii", "iii", "", ""},
    {"ik", "ipk", "", ""},
    // in deprecated https://www.loc.gov/standards/iso639-2/php/code_changes_bycode.php?code_ID=207
    {"in", "ind", "", ""},
    {"io", "ido", "", ""},
    {"is", "ice", "isl", "isl"},
    {"it", "ita", "", ""},
    {"iu", "iku", "", ""},
    // iw deprecated https://www.loc.gov/standards/iso639-2/php/code_changes_bycode.php?code_ID=184
    {"iw", "heb", "", ""},
    {"ja", "jpn", "", ""},
    {"jv", "jav", "", ""},
    {"ka", "geo", "", "kat"},
    {"kg", "kon", "", ""},
    {"ki", "kik", "", ""},
    {"kj", "kua", "", ""},
    {"kk", "kaz", "", ""},
    {"kl", "kal", "", ""},
    {"km", "khm", "", ""},
    {"kn", "kan", "", ""},
    {"ko", "kor", "", ""},
    {"kr", "kau", "", ""},
    {"ks", "kas", "", ""},
    {"ku", "kur", "", ""},
    {"kv", "kom", "", ""},
    {"kw", "cor", "", ""},
    {"ky", "kir", "", ""},
    {"la", "lat", "", ""},
    {"lb", "ltz", "", ""},
    {"lg", "lug", "", ""},
    {"li", "lim", "", ""},
    {"ln", "lin", "", ""},
    {"lo", "lao", "", ""},
    {"lt", "lit", "", ""},
    {"lu", "lub", "", ""},
    {"lv", "lav", "", ""},
    {"mg", "mlg", "", ""},
    {"mh", "mah", "", ""},
    {"mi", "mao", "", "mri"},
    {"mk", "mac", "", "mdk"},
    {"ml", "mal", "", ""},
    {"mn", "mon", "", ""},
    {"mr", "mar", "", ""},
    {"ms", "may", "", "msa"},
    {"mt", "mlt", "", ""},
    {"my", "bur", "", "mya"},
    {"na", "nau", "", ""},
    {"nb", "nob", "", ""},
    {"nd", "nde", "", ""},
    {"ne", "nep", "", ""},
    {"ng", "ndo", "", ""},
    {"nl", "dut", "nld", "nld"},
    {"nn", "nno", "", ""},
    {"no", "nor", "", ""},
    {"nr", "nbl", "", ""},
    {"nv", "nav", "", ""},
    {"ny", "nya", "", ""},
    {"oc", "oci", "", ""},
    {"oj", "oji", "", ""},
    {"om", "orm", "", ""},
    {"or", "ori", "", ""},
    {"os", "oss", "", ""},
    {"pa", "pan", "", ""},
    // pb / pob = unofficial language code for Brazilian Portuguese
    {"pb", "pob", "", ""},
    {"pi", "pli", "", ""},
    {"pl", "pol", "plk", ""},
    {"ps", "pus", "", ""},
    {"pt", "por", "ptg", ""},
    {"qu", "que", "", ""},
    {"rm", "roh", "", ""},
    {"rn", "run", "", ""},
    {"ro", "rum", "ron", "ron"},
    {"ru", "rus", "", ""},
    {"rw", "kin", "", ""},
    {"sa", "san", "", ""},
    {"sc", "srd", "", ""},
    {"sd", "snd", "", ""},
    {"se", "sme", "", ""},
    {"sg", "sag", "", ""},
    {"sh", "scr", "", ""},
    {"si", "sin", "", ""},
    {"sk", "slo", "sky", "slk"},
    {"sl", "slv", "", ""},
    {"sm", "smo", "", ""},
    {"sn", "sna", "", ""},
    {"so", "som", "", ""},
    {"sq", "alb", "", "sqi"},
    {"sr", "srp", "", ""},
    {"ss", "ssw", "", ""},
    {"st", "sot", "", ""},
    {"su", "sun", "", ""},
    {"sv", "swe", "sve", ""},
    {"sw", "swa", "", ""},
    {"ta", "tam", "", ""},
    {"te", "tel", "", ""},
    {"tg", "tgk", "", ""},
    {"th", "tha", "", ""},
    {"ti", "tir", "", ""},
    {"tk", "tuk", "", ""},
    {"tl", "tgl", "", ""},
    {"tn", "tsn", "", ""},
    {"to", "ton", "", ""},
    {"tr", "tur", "trk", ""},
    {"ts", "tso", "", ""},
    {"tt", "tat", "", ""},
    {"tw", "twi", "", ""},
    {"ty", "tah", "", ""},
    {"ug", "uig", "", ""},
    {"uk", "ukr", "", ""},
    {"ur", "urd", "", ""},
    {"uz", "uzb", "", ""},
    {"ve", "ven", "", ""},
    {"vi", "vie", "", ""},
    {"vo", "vol", "", ""},
    {"wa", "wln", "", ""},
    {"wo", "wol", "", ""},
    {"xh", "xho", "", ""},
    {"yi", "yid", "", ""},
    {"yo", "yor", "", ""},
    {"za", "zha", "", ""},
    {"zh", "chi", "zho", "zho"},
    {"zu", "zul", "", ""},
    {"zv", "und", "", ""}, // Kodi intern mapping for missing "Undetermined" iso639-1 code
    {"zx", "zxx", "", ""}, // Kodi intern mapping for missing "No linguistic content" iso639-1 code
    {"zy", "mis", "", ""}, // Kodi intern mapping for missing "Miscellaneous languages" iso639-1 code
    {"zz", "mul", "", ""}, // Kodi intern mapping for missing "Multiple languages" iso639-1 code
}};
// clang-format on

static_assert(std::ranges::is_sorted(LanguageCodes, {}, &ISO639::iso639_1));

constexpr auto CreateLanguageCodesSortedByIso639_2b()
{
  auto codes{LanguageCodes};
  std::ranges::sort(codes, {}, &ISO639::iso639_2b);
  return codes;
}

//! @todo save some space by extracting only the rows with a win_id value
constexpr auto CreateLanguageCodesSortedByWin_Id()
{
  auto codes{LanguageCodes};
  std::ranges::sort(codes, {}, &ISO639::win_id);
  return codes;
}

inline constexpr auto LanguageCodesByIso639_2b = CreateLanguageCodesSortedByIso639_2b();
inline constexpr auto LanguageCodesByWin_Id = CreateLanguageCodesSortedByWin_Id();
