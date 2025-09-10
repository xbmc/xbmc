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
#include <cassert>
#include <cstdint>
#include <string_view>

namespace
{
/*!
 * \brief Convert a language code from 2-3 letters string to a 4 bytes integer
 * \param[in] The string representation of the code
 * \return integer representation of the code
 */
constexpr uint32_t StringToLongCode(std::string_view a)
{
  const size_t len = a.length();

  assert(len <= 4);

  return static_cast<uint32_t>(len >= 4 ? a[len - 4] : 0) << 24 |
         static_cast<uint32_t>(len >= 3 ? a[len - 3] : 0) << 16 |
         static_cast<uint32_t>(len >= 2 ? a[len - 2] : 0) << 8 |
         static_cast<uint32_t>(len >= 1 ? a[len - 1] : 0);
}
} // namespace

struct LCENTRY
{
  uint32_t code;
  std::string_view name;
};

// ISO 639-1 table
// Source: Library of Congress http://www.loc.gov/standards/iso639-2

// 183 active ISO 639-1 codes + some non-conflicting deprecated + Kodi additions
inline constexpr int ISO639_1_COUNT = 187;

// clang-format off
constexpr std::array<struct LCENTRY, ISO639_1_COUNT> TableISO639_1 = {{
    //
    // Active codes, sorted by alpha-2
    //
    {StringToLongCode("aa"), "Afar"},
    {StringToLongCode("ab"), "Abkhazian"},
    {StringToLongCode("ae"), "Avestan"},
    {StringToLongCode("af"), "Afrikaans"},
    {StringToLongCode("ak"), "Akan"},
    {StringToLongCode("am"), "Amharic"},
    {StringToLongCode("an"), "Aragonese"},
    {StringToLongCode("ar"), "Arabic"},
    {StringToLongCode("as"), "Assamese"},
    {StringToLongCode("av"), "Avaric"},
    {StringToLongCode("ay"), "Aymara"},
    {StringToLongCode("az"), "Azerbaijani"},
    {StringToLongCode("ba"), "Bashkir"},
    {StringToLongCode("be"), "Belarusian"},
    {StringToLongCode("bg"), "Bulgarian"},
    {StringToLongCode("bi"), "Bislama"},
    {StringToLongCode("bm"), "Bambara"},
    {StringToLongCode("bn"), "Bengali; Bangla"},
    {StringToLongCode("bo"), "Tibetan"},
    {StringToLongCode("br"), "Breton"},
    {StringToLongCode("bs"), "Bosnian"},
    {StringToLongCode("ca"), "Catalan"},
    {StringToLongCode("ce"), "Chechen"},
    {StringToLongCode("ch"), "Chamorro"},
    {StringToLongCode("co"), "Corsican"},
    {StringToLongCode("cr"), "Cree"},
    {StringToLongCode("cs"), "Czech"},
    {StringToLongCode("cu"), "Church Slavic"},
    {StringToLongCode("cv"), "Chuvash"},
    {StringToLongCode("cy"), "Welsh"},
    {StringToLongCode("da"), "Danish"},
    {StringToLongCode("de"), "German"},
    {StringToLongCode("dv"), "Dhivehi"},
    {StringToLongCode("dz"), "Dzongkha"},
    {StringToLongCode("ee"), "Ewe"},
    {StringToLongCode("el"), "Greek"},
    {StringToLongCode("en"), "English"},
    {StringToLongCode("eo"), "Esperanto"},
    {StringToLongCode("es"), "Spanish"},
    {StringToLongCode("et"), "Estonian"},
    {StringToLongCode("eu"), "Basque"},
    {StringToLongCode("fa"), "Persian"},
    {StringToLongCode("ff"), "Fulah"},
    {StringToLongCode("fi"), "Finnish"},
    {StringToLongCode("fj"), "Fijian"},
    {StringToLongCode("fo"), "Faroese"},
    {StringToLongCode("fr"), "French"},
    {StringToLongCode("fy"), "Western Frisian"},
    {StringToLongCode("ga"), "Irish"},
    {StringToLongCode("gd"), "Scottish Gaelic"},
    {StringToLongCode("gl"), "Galician"},
    {StringToLongCode("gn"), "Guarani"},
    {StringToLongCode("gu"), "Gujarati"},
    {StringToLongCode("gv"), "Manx"},
    {StringToLongCode("ha"), "Hausa"},
    {StringToLongCode("he"), "Hebrew"},
    {StringToLongCode("hi"), "Hindi"},
    {StringToLongCode("ho"), "Hiri Motu"},
    {StringToLongCode("hr"), "Croatian"},
    {StringToLongCode("ht"), "Haitian"},
    {StringToLongCode("hu"), "Hungarian"},
    {StringToLongCode("hy"), "Armenian"},
    {StringToLongCode("hz"), "Herero"},
    {StringToLongCode("ia"), "Interlingua"},
    {StringToLongCode("id"), "Indonesian"},
    {StringToLongCode("ie"), "Interlingue"},
    {StringToLongCode("ig"), "Igbo"},
    {StringToLongCode("ii"), "Sichuan Yi"},
    {StringToLongCode("ik"), "Inupiat"},
    {StringToLongCode("io"), "Ido"},
    {StringToLongCode("is"), "Icelandic"},
    {StringToLongCode("it"), "Italian"},
    {StringToLongCode("iu"), "Inuktitut"},
    {StringToLongCode("ja"), "Japanese"},
    {StringToLongCode("jv"), "Javanese"},
    {StringToLongCode("ka"), "Georgian"},
    {StringToLongCode("kg"), "Kongo"},
    {StringToLongCode("ki"), "Kikuyu"},
    {StringToLongCode("kj"), "Kuanyama"},
    {StringToLongCode("kk"), "Kazakh"},
    {StringToLongCode("kl"), "Kalaallisut"},
    {StringToLongCode("km"), "Khmer"},
    {StringToLongCode("kn"), "Kannada"},
    {StringToLongCode("ko"), "Korean"},
    {StringToLongCode("kr"), "Kanuri"},
    {StringToLongCode("ks"), "Kashmiri"},
    {StringToLongCode("ku"), "Kurdish"},
    {StringToLongCode("kv"), "Komi"},
    {StringToLongCode("kw"), "Cornish"},
    {StringToLongCode("ky"), "Kirghiz"},
    {StringToLongCode("la"), "Latin"},
    {StringToLongCode("lb"), "Luxembourgish"},
    {StringToLongCode("lg"), "Ganda"},
    {StringToLongCode("li"), "Limburgan"},
    {StringToLongCode("ln"), "Lingala"},
    {StringToLongCode("lo"), "Lao"},
    {StringToLongCode("lt"), "Lithuanian"},
    {StringToLongCode("lu"), "Luba-Katanga"},
    {StringToLongCode("lv"), "Latvian, Lettish"},
    {StringToLongCode("mg"), "Malagasy"},
    {StringToLongCode("mh"), "Marshallese"},
    {StringToLongCode("mi"), "Maori"},
    {StringToLongCode("mk"), "Macedonian"},
    {StringToLongCode("ml"), "Malayalam"},
    {StringToLongCode("mn"), "Mongolian"},
    {StringToLongCode("mr"), "Marathi"},
    {StringToLongCode("ms"), "Malay"},
    {StringToLongCode("mt"), "Maltese"},
    {StringToLongCode("my"), "Burmese"},
    {StringToLongCode("na"), "Nauru"},
    {StringToLongCode("nb"), "Norwegian Bokmål"},
    {StringToLongCode("nd"), "Ndebele, North"},
    {StringToLongCode("ne"), "Nepali"},
    {StringToLongCode("ng"), "Ndonga"},
    {StringToLongCode("nl"), "Dutch"},
    {StringToLongCode("nn"), "Norwegian Nynorsk"},
    {StringToLongCode("no"), "Norwegian"},
    {StringToLongCode("nr"), "Ndebele, South"},
    {StringToLongCode("nv"), "Navajo"},
    {StringToLongCode("ny"), "Chichewa"},
    {StringToLongCode("oc"), "Occitan"},
    {StringToLongCode("oj"), "Ojibwa"},
    {StringToLongCode("om"), "Oromo"},
    {StringToLongCode("or"), "Oriya"},
    {StringToLongCode("os"), "Ossetic"},
    {StringToLongCode("pa"), "Punjabi"},
    {StringToLongCode("pi"), "Pali"},
    {StringToLongCode("pl"), "Polish"},
    {StringToLongCode("ps"), "Pashto, Pushto"},
    {StringToLongCode("pt"), "Portuguese"},
    {StringToLongCode("qu"), "Quechua"},
    {StringToLongCode("rm"), "Romansh"},
    {StringToLongCode("rn"), "Kirundi"},
    {StringToLongCode("ro"), "Romanian"},
    {StringToLongCode("ru"), "Russian"},
    {StringToLongCode("rw"), "Kinyarwanda"},
    {StringToLongCode("sa"), "Sanskrit"},
    {StringToLongCode("sc"), "Sardinian"},
    {StringToLongCode("sd"), "Sindhi"},
    {StringToLongCode("se"), "Northern Sami"},
    {StringToLongCode("sg"), "Sangho"},
    {StringToLongCode("si"), "Sinhalese"},
    {StringToLongCode("sk"), "Slovak"},
    {StringToLongCode("sl"), "Slovenian"},
    {StringToLongCode("sm"), "Samoan"},
    {StringToLongCode("sn"), "Shona"},
    {StringToLongCode("so"), "Somali"},
    {StringToLongCode("sq"), "Albanian"},
    {StringToLongCode("sr"), "Serbian"},
    {StringToLongCode("ss"), "Swati"},
    {StringToLongCode("st"), "Sesotho"},
    {StringToLongCode("su"), "Sundanese"},
    {StringToLongCode("sv"), "Swedish"},
    {StringToLongCode("sw"), "Swahili"},
    {StringToLongCode("ta"), "Tamil"},
    {StringToLongCode("te"), "Telugu"},
    {StringToLongCode("tg"), "Tajik"},
    {StringToLongCode("th"), "Thai"},
    {StringToLongCode("ti"), "Tigrinya"},
    {StringToLongCode("tk"), "Turkmen"},
    {StringToLongCode("tl"), "Tagalog"},
    {StringToLongCode("tn"), "Tswana"},
    {StringToLongCode("to"), "Tonga"},
    {StringToLongCode("tr"), "Turkish"},
    {StringToLongCode("ts"), "Tsonga"},
    {StringToLongCode("tt"), "Tatar"},
    {StringToLongCode("tw"), "Twi"},
    {StringToLongCode("ty"), "Tahitian"},
    {StringToLongCode("ug"), "Uighur"},
    {StringToLongCode("uk"), "Ukrainian"},
    {StringToLongCode("ur"), "Urdu"},
    {StringToLongCode("uz"), "Uzbek"},
    {StringToLongCode("ve"), "Venda"},
    {StringToLongCode("vi"), "Vietnamese"},
    {StringToLongCode("vo"), "Volapuk"},
    {StringToLongCode("wa"), "Walloon"},
    {StringToLongCode("wo"), "Wolof"},
    {StringToLongCode("xh"), "Xhosa"},
    {StringToLongCode("yi"), "Yiddish"},
    {StringToLongCode("yo"), "Yoruba"},
    {StringToLongCode("za"), "Zhuang"},
    {StringToLongCode("zh"), "Chinese"},
    {StringToLongCode("zu"), "Zulu"},

    // ============================================================================================
    //
    // Deprecated codes
    // 
    //! @todo refactor to allow recognition of deprecated codes without conflicts in name lookup

    // ji Yiddish - replaced with yi Yiddish - conflicts in lookup by name
    //{StringToLongCode("ji"), "Yiddish"},

    // in Indonesian - replaced with id Indonesian - may conflict in lookup by name
    // https://www.loc.gov/standards/iso639-2/php/code_changes_bycode.php?code_ID=207
    //{StringToLongCode("in"), "Indonesian"},

    // iw Hebrew - replaced with he - may conflict in lookup by name
    // https://www.loc.gov/standards/iso639-2/php/code_changes_bycode.php?code_ID=184
    //{StringToLongCode("iw"), "Hebrew"},

    // sh Serbo-Croatian - no replacement in ISO 638-1 - split into sr hr bs mk
    {StringToLongCode("sh"), "Serbo-Croatian"},

    // jw Javanese - replaced with jv - conflicts in lookup by name
    //{StringToLongCode("jw"sv), "Javanese"},

    // mo Moldavian/Moldovan
    {StringToLongCode("mo"), "Moldavian"},

    // bh Bihari - no ISO 639-1 replacement
    {StringToLongCode("bh"), "Bihari"},

    // ============================================================================================
    //
    // Kodi Additions
    //

    // pb unofficial language code for Brazilian Portuguese
    {StringToLongCode("pb"), "Portuguese (Brazil)"},
}};
// clang-format on

constexpr auto CreateIso6391ByCode()
{
  auto codes{TableISO639_1};
  //! @todo create the array with lower-cased names to avoid case-insentive comparison by users later.
  std::ranges::sort(codes, {}, &LCENTRY::code);
  return codes;
}

inline constexpr auto TableISO639_1ByCode = CreateIso6391ByCode();

constexpr auto CreateIso6391ByName()
{
  auto codes{TableISO639_1};
  //! @todo create the array with lower-cased names to avoid case-insentive comparison by users later.
  std::ranges::sort(
      codes, [](std::string_view a, std::string_view b)
      { return StringUtils::CompareNoCase(a, b, 0) < 0; }, &LCENTRY::name);
  return codes;
}

inline constexpr auto TableISO639_1ByName = CreateIso6391ByName();
