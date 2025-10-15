/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/i18n/Iso639.h"

#include <algorithm>
#include <array>
#include <string_view>

namespace KODI::UTILS::I18N
{
// ISO 639-1 table
// Source: Library of Congress http://www.loc.gov/standards/iso639-2

// 183 active ISO 639-1 codes + 1 Kodi addition. See deprecated codes further down.
inline static constexpr int ISO639_1_COUNT = 184;

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
    // Kodi Additions
    //

    // pb unofficial language code for Brazilian Portuguese
    {StringToLongCode("pb"), "Portuguese (Brazil)"},
}};
// clang-format on

inline constexpr int ISO639_1_DEPRECATED_COUNT = 7;

// clang-format off
inline constexpr std::array<struct LCENTRY, ISO639_1_DEPRECATED_COUNT> TableISO639_1_Depr = {{
    // Deprecated codes in chronological order for ease of future maintenance

    // 1989-03-11 ji replaced with yi
    {StringToLongCode("ji"), "Yiddish"},

    // 1989-03-11 in replaced with id
    // https://www.loc.gov/standards/iso639-2/php/code_changes_bycode.php?code_ID=207
    {StringToLongCode("in"), "Indonesian"},

    // 1989-03-11 iw replaced with he
    // https://www.loc.gov/standards/iso639-2/php/code_changes_bycode.php?code_ID=184
    {StringToLongCode("iw"), "Hebrew"},

    // 2000-02-18 sh Serbo-Croatian - split into sr hr bs mk
    {StringToLongCode("sh"), "Serbo-Croatian"},

    // 2001-08-13 jw replaced with jv
    {StringToLongCode("jw"), "Javanese"},

    // 2008-11-03 mo Moldavian/Moldovan - no ISO 639-1 replacement.
    {StringToLongCode("mo"), "Moldavian"},

    // 2021-05-25 bh Bihari - no ISO 639-1 replacement. ISO 639-2 bih is recommended
    {StringToLongCode("bh"), "Bihari"},
}};
// clang-format on

// Prepare sorted arrays to enable binary search

inline constexpr auto TableISO639_1ByCode = CreateIso639ByCode(TableISO639_1);
inline constexpr auto TableISO639_1_DeprByCode = CreateIso639ByCode(TableISO639_1_Depr);

inline constexpr auto TableISO639_1ByName = CreateIso639ByName(TableISO639_1);
inline constexpr auto TableISO639_1_DeprByName = CreateIso639ByName(TableISO639_1_Depr);

//-------------------------------------------------------------------------------------------------
// Data integrity validations
//

static_assert(std::ranges::adjacent_find(TableISO639_1ByCode, {}, &LCENTRY::code) ==
              TableISO639_1ByCode.end());
static_assert(std::ranges::adjacent_find(TableISO639_1_DeprByCode, {}, &LCENTRY::code) ==
              TableISO639_1_DeprByCode.end());

static_assert(std::ranges::adjacent_find(TableISO639_1ByName, {}, &LCENTRY::name) ==
              TableISO639_1ByName.end());
static_assert(std::ranges::adjacent_find(TableISO639_1_DeprByName, {}, &LCENTRY::name) ==
              TableISO639_1_DeprByName.end());

} // namespace KODI::UTILS::I18N
