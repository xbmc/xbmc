/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/i18n/TableISO639.h"

#include <array>
#include <string_view>

using namespace std::literals;

// ISO 639-1 table
// Source: Library of Congress http://www.loc.gov/standards/iso639-2

// 183 active ISO 639-1 codes + 1 Kodi addition. See deprecated codes further down.
static constexpr int ISO639_1_COUNT = 184;

// clang-format off
constexpr std::array<struct LCENTRY, ISO639_1_COUNT> TableISO639_1 = {{
    //
    // Active codes, sorted by alpha-2
    //
    {StringToLongCode("aa"sv), "Afar"},
    {StringToLongCode("ab"sv), "Abkhazian"},
    {StringToLongCode("ae"sv), "Avestan"},
    {StringToLongCode("af"sv), "Afrikaans"},
    {StringToLongCode("ak"sv), "Akan"},
    {StringToLongCode("am"sv), "Amharic"},
    {StringToLongCode("an"sv), "Aragonese"},
    {StringToLongCode("ar"sv), "Arabic"},
    {StringToLongCode("as"sv), "Assamese"},
    {StringToLongCode("av"sv), "Avaric"},
    {StringToLongCode("ay"sv), "Aymara"},
    {StringToLongCode("az"sv), "Azerbaijani"},
    {StringToLongCode("ba"sv), "Bashkir"},
    {StringToLongCode("be"sv), "Belarusian"},
    {StringToLongCode("bg"sv), "Bulgarian"},
    {StringToLongCode("bi"sv), "Bislama"},
    {StringToLongCode("bm"sv), "Bambara"},
    {StringToLongCode("bn"sv), "Bengali; Bangla"},
    {StringToLongCode("bo"sv), "Tibetan"},
    {StringToLongCode("br"sv), "Breton"},
    {StringToLongCode("bs"sv), "Bosnian"},
    {StringToLongCode("ca"sv), "Catalan"},
    {StringToLongCode("ce"sv), "Chechen"},
    {StringToLongCode("ch"sv), "Chamorro"},
    {StringToLongCode("co"sv), "Corsican"},
    {StringToLongCode("cr"sv), "Cree"},
    {StringToLongCode("cs"sv), "Czech"},
    {StringToLongCode("cu"sv), "Church Slavic"},
    {StringToLongCode("cv"sv), "Chuvash"},
    {StringToLongCode("cy"sv), "Welsh"},
    {StringToLongCode("da"sv), "Danish"},
    {StringToLongCode("de"sv), "German"},
    {StringToLongCode("dv"sv), "Dhivehi"},
    {StringToLongCode("dz"sv), "Dzongkha"},
    {StringToLongCode("ee"sv), "Ewe"},
    {StringToLongCode("el"sv), "Greek"},
    {StringToLongCode("en"sv), "English"},
    {StringToLongCode("eo"sv), "Esperanto"},
    {StringToLongCode("es"sv), "Spanish"},
    {StringToLongCode("et"sv), "Estonian"},
    {StringToLongCode("eu"sv), "Basque"},
    {StringToLongCode("fa"sv), "Persian"},
    {StringToLongCode("ff"sv), "Fulah"},
    {StringToLongCode("fi"sv), "Finnish"},
    {StringToLongCode("fj"sv), "Fijian"},
    {StringToLongCode("fo"sv), "Faroese"},
    {StringToLongCode("fr"sv), "French"},
    {StringToLongCode("fy"sv), "Western Frisian"},
    {StringToLongCode("ga"sv), "Irish"},
    {StringToLongCode("gd"sv), "Scottish Gaelic"},
    {StringToLongCode("gl"sv), "Galician"},
    {StringToLongCode("gn"sv), "Guarani"},
    {StringToLongCode("gu"sv), "Gujarati"},
    {StringToLongCode("gv"sv), "Manx"},
    {StringToLongCode("ha"sv), "Hausa"},
    {StringToLongCode("he"sv), "Hebrew"},
    {StringToLongCode("hi"sv), "Hindi"},
    {StringToLongCode("ho"sv), "Hiri Motu"},
    {StringToLongCode("hr"sv), "Croatian"},
    {StringToLongCode("ht"sv), "Haitian"},
    {StringToLongCode("hu"sv), "Hungarian"},
    {StringToLongCode("hy"sv), "Armenian"},
    {StringToLongCode("hz"sv), "Herero"},
    {StringToLongCode("ia"sv), "Interlingua"},
    {StringToLongCode("id"sv), "Indonesian"},
    {StringToLongCode("ie"sv), "Interlingue"},
    {StringToLongCode("ig"sv), "Igbo"},
    {StringToLongCode("ii"sv), "Sichuan Yi"},
    {StringToLongCode("ik"sv), "Inupiat"},
    {StringToLongCode("io"sv), "Ido"},
    {StringToLongCode("is"sv), "Icelandic"},
    {StringToLongCode("it"sv), "Italian"},
    {StringToLongCode("iu"sv), "Inuktitut"},
    {StringToLongCode("ja"sv), "Japanese"},
    {StringToLongCode("jv"sv), "Javanese"},
    {StringToLongCode("ka"sv), "Georgian"},
    {StringToLongCode("kg"sv), "Kongo"},
    {StringToLongCode("ki"sv), "Kikuyu"},
    {StringToLongCode("kj"sv), "Kuanyama"},
    {StringToLongCode("kk"sv), "Kazakh"},
    {StringToLongCode("kl"sv), "Kalaallisut"},
    {StringToLongCode("km"sv), "Khmer"},
    {StringToLongCode("kn"sv), "Kannada"},
    {StringToLongCode("ko"sv), "Korean"},
    {StringToLongCode("kr"sv), "Kanuri"},
    {StringToLongCode("ks"sv), "Kashmiri"},
    {StringToLongCode("ku"sv), "Kurdish"},
    {StringToLongCode("kv"sv), "Komi"},
    {StringToLongCode("kw"sv), "Cornish"},
    {StringToLongCode("ky"sv), "Kirghiz"},
    {StringToLongCode("la"sv), "Latin"},
    {StringToLongCode("lb"sv), "Luxembourgish"},
    {StringToLongCode("lg"sv), "Ganda"},
    {StringToLongCode("li"sv), "Limburgan"},
    {StringToLongCode("ln"sv), "Lingala"},
    {StringToLongCode("lo"sv), "Lao"},
    {StringToLongCode("lt"sv), "Lithuanian"},
    {StringToLongCode("lu"sv), "Luba-Katanga"},
    {StringToLongCode("lv"sv), "Latvian, Lettish"},
    {StringToLongCode("mg"sv), "Malagasy"},
    {StringToLongCode("mh"sv), "Marshallese"},
    {StringToLongCode("mi"sv), "Maori"},
    {StringToLongCode("mk"sv), "Macedonian"},
    {StringToLongCode("ml"sv), "Malayalam"},
    {StringToLongCode("mn"sv), "Mongolian"},
    {StringToLongCode("mr"sv), "Marathi"},
    {StringToLongCode("ms"sv), "Malay"},
    {StringToLongCode("mt"sv), "Maltese"},
    {StringToLongCode("my"sv), "Burmese"},
    {StringToLongCode("na"sv), "Nauru"},
    {StringToLongCode("nb"sv), "Norwegian Bokmål"},
    {StringToLongCode("nd"sv), "Ndebele, North"},
    {StringToLongCode("ne"sv), "Nepali"},
    {StringToLongCode("ng"sv), "Ndonga"},
    {StringToLongCode("nl"sv), "Dutch"},
    {StringToLongCode("nn"sv), "Norwegian Nynorsk"},
    {StringToLongCode("no"sv), "Norwegian"},
    {StringToLongCode("nr"sv), "Ndebele, South"},
    {StringToLongCode("nv"sv), "Navajo"},
    {StringToLongCode("ny"sv), "Chichewa"},
    {StringToLongCode("oc"sv), "Occitan"},
    {StringToLongCode("oj"sv), "Ojibwa"},
    {StringToLongCode("om"sv), "Oromo"},
    {StringToLongCode("or"sv), "Oriya"},
    {StringToLongCode("os"sv), "Ossetic"},
    {StringToLongCode("pa"sv), "Punjabi"},
    {StringToLongCode("pi"sv), "Pali"},
    {StringToLongCode("pl"sv), "Polish"},
    {StringToLongCode("ps"sv), "Pashto, Pushto"},
    {StringToLongCode("pt"sv), "Portuguese"},
    {StringToLongCode("qu"sv), "Quechua"},
    {StringToLongCode("rm"sv), "Romansh"},
    {StringToLongCode("rn"sv), "Kirundi"},
    {StringToLongCode("ro"sv), "Romanian"},
    {StringToLongCode("ru"sv), "Russian"},
    {StringToLongCode("rw"sv), "Kinyarwanda"},
    {StringToLongCode("sa"sv), "Sanskrit"},
    {StringToLongCode("sc"sv), "Sardinian"},
    {StringToLongCode("sd"sv), "Sindhi"},
    {StringToLongCode("se"sv), "Northern Sami"},
    {StringToLongCode("sg"sv), "Sangho"},
    {StringToLongCode("si"sv), "Sinhalese"},
    {StringToLongCode("sk"sv), "Slovak"},
    {StringToLongCode("sl"sv), "Slovenian"},
    {StringToLongCode("sm"sv), "Samoan"},
    {StringToLongCode("sn"sv), "Shona"},
    {StringToLongCode("so"sv), "Somali"},
    {StringToLongCode("sq"sv), "Albanian"},
    {StringToLongCode("sr"sv), "Serbian"},
    {StringToLongCode("ss"sv), "Swati"},
    {StringToLongCode("st"sv), "Sesotho"},
    {StringToLongCode("su"sv), "Sundanese"},
    {StringToLongCode("sv"sv), "Swedish"},
    {StringToLongCode("sw"sv), "Swahili"},
    {StringToLongCode("ta"sv), "Tamil"},
    {StringToLongCode("te"sv), "Telugu"},
    {StringToLongCode("tg"sv), "Tajik"},
    {StringToLongCode("th"sv), "Thai"},
    {StringToLongCode("ti"sv), "Tigrinya"},
    {StringToLongCode("tk"sv), "Turkmen"},
    {StringToLongCode("tl"sv), "Tagalog"},
    {StringToLongCode("tn"sv), "Tswana"},
    {StringToLongCode("to"sv), "Tonga"},
    {StringToLongCode("tr"sv), "Turkish"},
    {StringToLongCode("ts"sv), "Tsonga"},
    {StringToLongCode("tt"sv), "Tatar"},
    {StringToLongCode("tw"sv), "Twi"},
    {StringToLongCode("ty"sv), "Tahitian"},
    {StringToLongCode("ug"sv), "Uighur"},
    {StringToLongCode("uk"sv), "Ukrainian"},
    {StringToLongCode("ur"sv), "Urdu"},
    {StringToLongCode("uz"sv), "Uzbek"},
    {StringToLongCode("ve"sv), "Venda"},
    {StringToLongCode("vi"sv), "Vietnamese"},
    {StringToLongCode("vo"sv), "Volapuk"},
    {StringToLongCode("wa"sv), "Walloon"},
    {StringToLongCode("wo"sv), "Wolof"},
    {StringToLongCode("xh"sv), "Xhosa"},
    {StringToLongCode("yi"sv), "Yiddish"},
    {StringToLongCode("yo"sv), "Yoruba"},
    {StringToLongCode("za"sv), "Zhuang"},
    {StringToLongCode("zh"sv), "Chinese"},
    {StringToLongCode("zu"sv), "Zulu"},

    // ============================================================================================
    //
    // Kodi Additions
    //

    // pb unofficial language code for Brazilian Portuguese
    {StringToLongCode("pb"sv), "Portuguese (Brazil)"},
}};
// clang-format on

static constexpr int ISO639_1_DEPRECATED_COUNT = 7;

// clang-format off
constexpr std::array<struct LCENTRY, ISO639_1_DEPRECATED_COUNT> TableISO639_1_Depr = {{
    // Deprecated codes in chronological order for ease of future maintenance

    // 1989-03-11 ji replaced with yi
    {StringToLongCode("ji"sv), "Yiddish"},

    // 1989-03-11 in replaced with id
    // https://www.loc.gov/standards/iso639-2/php/code_changes_bycode.php?code_ID=207
    {StringToLongCode("in"sv), "Indonesian"},

    // 1989-03-11 iw replaced with he
    // https://www.loc.gov/standards/iso639-2/php/code_changes_bycode.php?code_ID=184
    {StringToLongCode("iw"sv), "Hebrew"},

    // 2000-02-18 sh Serbo-Croatian - split into sr hr bs mk
    {StringToLongCode("sh"sv), "Serbo-Croatian"},

    // 2001-08-13 jw replaced with jv
    {StringToLongCode("jw"sv), "Javanese"},

    // 2008-11-03 mo Moldavian/Moldovan - no ISO 639-1 replacement.
    {StringToLongCode("mo"sv), "Moldavian"},

    // 2021-05-25 bh Bihari - no ISO 639-1 replacement. ISO 639-2 bih is recommended
    {StringToLongCode("bh"sv), "Bihari"},
}};
// clang-format on

// Prepare sorted arrays to enable binary search

static constexpr auto TableISO639_1ByCode = CreateIso639ByCode(TableISO639_1);
static constexpr auto TableISO639_1_DeprByCode = CreateIso639ByCode(TableISO639_1_Depr);

static constexpr auto TableISO639_1ByName = CreateIso639ByName(TableISO639_1);
static constexpr auto TableISO639_1_DeprByName = CreateIso639ByName(TableISO639_1_Depr);
