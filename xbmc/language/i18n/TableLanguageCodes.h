/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/StringUtils.h"

#include <algorithm>
#include <array>
#include <optional>
#include <string_view>
#include <utility>

namespace KODI::LANGUAGE::I18N
{
struct ISO639_1
{
  std::string_view alpha2; // alpha-2 code
  std::string_view alpha3B; // ISO 639-2/B code
  std::string_view name; // English name
  bool withdrawn; // ISO 639-1 no longer assigns the code
};

// ISO 639-1 table
// Source: Library of Congress http://www.loc.gov/standards/iso639-2, sorted by alpha-2

// 4 special-scope ISO 639-2 codes + 183 current ISO 639-1 codes + 1 Kodi addition + 7 withdrawn
inline constexpr int ISO639_1_COUNT = 195;

// clang-format off
inline constexpr std::array<ISO639_1, ISO639_1_COUNT> TableISO639_1 = {{
    // The four special-scope ISO 639-2 codes have no ISO 639-1 code, and sort first for it
    {"", "und", "", false}, // Undetermined
    {"", "zxx", "", false}, // No linguistic content
    {"", "mis", "", false}, // Uncoded languages
    {"", "mul", "", false}, // Multiple languages
    {"aa", "aar", "Afar", false},
    {"ab", "abk", "Abkhazian", false},
    {"ae", "ave", "Avestan", false},
    {"af", "afr", "Afrikaans", false},
    {"ak", "aka", "Akan", false},
    {"am", "amh", "Amharic", false},
    {"an", "arg", "Aragonese", false},
    {"ar", "ara", "Arabic", false},
    {"as", "asm", "Assamese", false},
    {"av", "ava", "Avaric", false},
    {"ay", "aym", "Aymara", false},
    {"az", "aze", "Azerbaijani", false},
    {"ba", "bak", "Bashkir", false},
    {"be", "bel", "Belarusian", false},
    {"bg", "bul", "Bulgarian", false},
    {"bh", "bih", "Bihari", true}, // withdrawn 2021, with no alpha-2 replacement
    {"bi", "bis", "Bislama", false},
    {"bm", "bam", "Bambara", false},
    {"bn", "ben", "Bengali; Bangla", false},
    {"bo", "tib", "Tibetan", false},
    {"br", "bre", "Breton", false},
    {"bs", "bos", "Bosnian", false},
    {"ca", "cat", "Catalan", false},
    {"ce", "che", "Chechen", false},
    {"ch", "cha", "Chamorro", false},
    {"co", "cos", "Corsican", false},
    {"cr", "cre", "Cree", false},
    {"cs", "cze", "Czech", false},
    {"cu", "chu", "Church Slavic", false},
    {"cv", "chv", "Chuvash", false},
    {"cy", "wel", "Welsh", false},
    {"da", "dan", "Danish", false},
    {"de", "ger", "German", false},
    {"dv", "div", "Dhivehi", false},
    {"dz", "dzo", "Dzongkha", false},
    {"ee", "ewe", "Ewe", false},
    {"el", "gre", "Greek", false},
    {"en", "eng", "English", false},
    {"eo", "epo", "Esperanto", false},
    {"es", "spa", "Spanish", false},
    {"et", "est", "Estonian", false},
    {"eu", "baq", "Basque", false},
    {"fa", "per", "Persian", false},
    {"ff", "ful", "Fulah", false},
    {"fi", "fin", "Finnish", false},
    {"fj", "fij", "Fijian", false},
    {"fo", "fao", "Faroese", false},
    {"fr", "fre", "French", false},
    {"fy", "fry", "Western Frisian", false},
    {"ga", "gle", "Irish", false},
    {"gd", "gla", "Scottish Gaelic", false},
    {"gl", "glg", "Galician", false},
    {"gn", "grn", "Guarani", false},
    {"gu", "guj", "Gujarati", false},
    {"gv", "glv", "Manx", false},
    {"ha", "hau", "Hausa", false},
    {"he", "heb", "Hebrew", false},
    {"hi", "hin", "Hindi", false},
    {"ho", "hmo", "Hiri Motu", false},
    {"hr", "hrv", "Croatian", false},
    {"ht", "hat", "Haitian", false},
    {"hu", "hun", "Hungarian", false},
    {"hy", "arm", "Armenian", false},
    {"hz", "her", "Herero", false},
    {"ia", "ina", "Interlingua", false},
    {"id", "ind", "Indonesian", false},
    {"ie", "ile", "Interlingue", false},
    {"ig", "ibo", "Igbo", false},
    {"ii", "iii", "Sichuan Yi", false},
    {"ik", "ipk", "Inupiat", false},
    {"in", "ind", "Indonesian", true}, // withdrawn 1989, now id
    {"io", "ido", "Ido", false},
    {"is", "ice", "Icelandic", false},
    {"it", "ita", "Italian", false},
    {"iu", "iku", "Inuktitut", false},
    {"iw", "heb", "Hebrew", true}, // withdrawn 1989, now he
    {"ja", "jpn", "Japanese", false},
    {"ji", "yid", "Yiddish", true}, // withdrawn 1989, now yi
    {"jv", "jav", "Javanese", false},
    {"jw", "jav", "Javanese", true}, // withdrawn 2001, now jv
    {"ka", "geo", "Georgian", false},
    {"kg", "kon", "Kongo", false},
    {"ki", "kik", "Kikuyu", false},
    {"kj", "kua", "Kuanyama", false},
    {"kk", "kaz", "Kazakh", false},
    {"kl", "kal", "Kalaallisut", false},
    {"km", "khm", "Khmer", false},
    {"kn", "kan", "Kannada", false},
    {"ko", "kor", "Korean", false},
    {"kr", "kau", "Kanuri", false},
    {"ks", "kas", "Kashmiri", false},
    {"ku", "kur", "Kurdish", false},
    {"kv", "kom", "Komi", false},
    {"kw", "cor", "Cornish", false},
    {"ky", "kir", "Kirghiz", false},
    {"la", "lat", "Latin", false},
    {"lb", "ltz", "Luxembourgish", false},
    {"lg", "lug", "Ganda", false},
    {"li", "lim", "Limburgan", false},
    {"ln", "lin", "Lingala", false},
    {"lo", "lao", "Lao", false},
    {"lt", "lit", "Lithuanian", false},
    {"lu", "lub", "Luba-Katanga", false},
    {"lv", "lav", "Latvian, Lettish", false},
    {"mg", "mlg", "Malagasy", false},
    {"mh", "mah", "Marshallese", false},
    {"mi", "mao", "Maori", false},
    {"mk", "mac", "Macedonian", false},
    {"ml", "mal", "Malayalam", false},
    {"mn", "mon", "Mongolian", false},
    {"mo", "rum", "Moldavian", true}, // withdrawn 2008, merged into ro
    {"mr", "mar", "Marathi", false},
    {"ms", "may", "Malay", false},
    {"mt", "mlt", "Maltese", false},
    {"my", "bur", "Burmese", false},
    {"na", "nau", "Nauru", false},
    {"nb", "nob", "Norwegian Bokmål", false},
    {"nd", "nde", "Ndebele, North", false},
    {"ne", "nep", "Nepali", false},
    {"ng", "ndo", "Ndonga", false},
    {"nl", "dut", "Dutch", false},
    {"nn", "nno", "Norwegian Nynorsk", false},
    {"no", "nor", "Norwegian", false},
    {"nr", "nbl", "Ndebele, South", false},
    {"nv", "nav", "Navajo", false},
    {"ny", "nya", "Chichewa", false},
    {"oc", "oci", "Occitan", false},
    {"oj", "oji", "Ojibwa", false},
    {"om", "orm", "Oromo", false},
    {"or", "ori", "Oriya", false},
    {"os", "oss", "Ossetic", false},
    {"pa", "pan", "Punjabi", false},
    // unofficial code for Brazilian Portuguese
    {"pb", "pob", "Portuguese (Brazil)", false},
    {"pi", "pli", "Pali", false},
    {"pl", "pol", "Polish", false},
    {"ps", "pus", "Pashto, Pushto", false},
    {"pt", "por", "Portuguese", false},
    {"qu", "que", "Quechua", false},
    {"rm", "roh", "Romansh", false},
    {"rn", "run", "Kirundi", false},
    {"ro", "rum", "Romanian", false},
    {"ru", "rus", "Russian", false},
    {"rw", "kin", "Kinyarwanda", false},
    {"sa", "san", "Sanskrit", false},
    {"sc", "srd", "Sardinian", false},
    {"sd", "snd", "Sindhi", false},
    {"se", "sme", "Northern Sami", false},
    {"sg", "sag", "Sangho", false},
    {"sh", "hbs", "Serbo-Croatian", true}, // withdrawn 2000; hbs is ISO 639-3, ISO 639-2 having withdrawn scr
    {"si", "sin", "Sinhalese", false},
    {"sk", "slo", "Slovak", false},
    {"sl", "slv", "Slovenian", false},
    {"sm", "smo", "Samoan", false},
    {"sn", "sna", "Shona", false},
    {"so", "som", "Somali", false},
    {"sq", "alb", "Albanian", false},
    {"sr", "srp", "Serbian", false},
    {"ss", "ssw", "Swati", false},
    {"st", "sot", "Sesotho", false},
    {"su", "sun", "Sundanese", false},
    {"sv", "swe", "Swedish", false},
    {"sw", "swa", "Swahili", false},
    {"ta", "tam", "Tamil", false},
    {"te", "tel", "Telugu", false},
    {"tg", "tgk", "Tajik", false},
    {"th", "tha", "Thai", false},
    {"ti", "tir", "Tigrinya", false},
    {"tk", "tuk", "Turkmen", false},
    {"tl", "tgl", "Tagalog", false},
    {"tn", "tsn", "Tswana", false},
    {"to", "ton", "Tonga", false},
    {"tr", "tur", "Turkish", false},
    {"ts", "tso", "Tsonga", false},
    {"tt", "tat", "Tatar", false},
    {"tw", "twi", "Twi", false},
    {"ty", "tah", "Tahitian", false},
    {"ug", "uig", "Uighur", false},
    {"uk", "ukr", "Ukrainian", false},
    {"ur", "urd", "Urdu", false},
    {"uz", "uzb", "Uzbek", false},
    {"ve", "ven", "Venda", false},
    {"vi", "vie", "Vietnamese", false},
    {"vo", "vol", "Volapuk", false},
    {"wa", "wln", "Walloon", false},
    {"wo", "wol", "Wolof", false},
    {"xh", "xho", "Xhosa", false},
    {"yi", "yid", "Yiddish", false},
    {"yo", "yor", "Yoruba", false},
    {"za", "zha", "Zhuang", false},
    {"zh", "chi", "Chinese", false},
    {"zu", "zul", "Zulu", false},
}};
// clang-format on

static_assert(std::ranges::is_sorted(TableISO639_1, {}, &ISO639_1::alpha2));

static_assert(std::ranges::all_of(
    TableISO639_1,
    [](std::string_view name) { return StringUtils::IsAsciiTrimmed(name); },
    &ISO639_1::name));

constexpr auto CreateTableISO639_1ByAlpha3B()
{
  auto codes{TableISO639_1};
  // A language's current code sorts ahead of a code withdrawn from it
  std::ranges::sort(codes, {}, [](const ISO639_1& entry)
                    { return std::pair{entry.alpha3B, entry.withdrawn}; });
  return codes;
}

inline constexpr auto TableISO639_1ByAlpha3B = CreateTableISO639_1ByAlpha3B();

// The sort above is not stable, so a repeated code would resolve to whichever row the compiler
// happened to place first - a language exported differently by different builds.
static_assert(std::ranges::adjacent_find(TableISO639_1ByAlpha3B,
                                         [](const ISO639_1& a, const ISO639_1& b)
                                         {
                                           return a.alpha3B == b.alpha3B &&
                                                  a.withdrawn == b.withdrawn;
                                         }) == TableISO639_1ByAlpha3B.end());

/*!
 * \brief The ISO 639-1 code of a language given by its ISO 639-2/B code.
 * \return The current code, or the withdrawn one where ISO 639-1 has no other for the language,
 *         or nullopt when it has none at all.
 */
constexpr std::optional<std::string_view> Alpha2OfAlpha3B(std::string_view alpha3B)
{
  const auto it = std::ranges::lower_bound(TableISO639_1ByAlpha3B, alpha3B, {}, &ISO639_1::alpha3B);
  if (it == TableISO639_1ByAlpha3B.end() || it->alpha3B != alpha3B || it->alpha2.empty())
    return std::nullopt;

  return it->alpha2;
}
} // namespace KODI::LANGUAGE::I18N
