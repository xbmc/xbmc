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

using namespace std::literals;

struct ISO639
{
  std::string_view iso639_1;
  std::string_view iso639_2b;
  std::string_view iso639_2t;
  std::string_view win_id;
};

// Legacy ISO 639 table - unclear source
// Sorted by alpha-2

static constexpr int LANGUAGE_CODES_COUNT = 192;

// clang-format off
constexpr std::array<ISO639, LANGUAGE_CODES_COUNT> LanguageCodes = {{
    {"aa"sv, "aar"sv, ""sv, ""sv},
    {"ab"sv, "abk"sv, ""sv, ""sv},
    {"ae"sv, "ave"sv, ""sv, ""sv},
    {"af"sv, "afr"sv, ""sv, ""sv},
    {"ak"sv, "aka"sv, ""sv, ""sv},
    {"am"sv, "amh"sv, ""sv, ""sv},
    {"an"sv, "arg"sv, ""sv, ""sv},
    {"ar"sv, "ara"sv, ""sv, ""sv},
    {"as"sv, "asm"sv, ""sv, ""sv},
    {"av"sv, "ava"sv, ""sv, ""sv},
    {"ay"sv, "aym"sv, ""sv, ""sv},
    {"az"sv, "aze"sv, ""sv, ""sv},
    {"ba"sv, "bak"sv, ""sv, ""sv},
    {"be"sv, "bel"sv, ""sv, ""sv},
    {"bg"sv, "bul"sv, ""sv, ""sv},
    {"bh"sv, "bih"sv, ""sv, ""sv},
    {"bi"sv, "bis"sv, ""sv, ""sv},
    {"bm"sv, "bam"sv, ""sv, ""sv},
    {"bn"sv, "ben"sv, ""sv, ""sv},
    {"bo"sv, "tib"sv, ""sv, "bod"sv},
    {"br"sv, "bre"sv, ""sv, ""sv},
    {"bs"sv, "bos"sv, ""sv, ""sv},
    {"ca"sv, "cat"sv, ""sv, ""sv},
    {"ce"sv, "che"sv, ""sv, ""sv},
    {"ch"sv, "cha"sv, ""sv, ""sv},
    {"co"sv, "cos"sv, ""sv, ""sv},
    {"cr"sv, "cre"sv, ""sv, ""sv},
    {"cs"sv, "cze"sv, "ces"sv, "ces"sv},
    {"cu"sv, "chu"sv, ""sv, ""sv},
    {"cv"sv, "chv"sv, ""sv, ""sv},
    {"cy"sv, "wel"sv, ""sv, "cym"sv},
    {"da"sv, "dan"sv, ""sv, ""sv},
    {"de"sv, "ger"sv, "deu"sv, "deu"sv},
    {"dv"sv, "div"sv, ""sv, ""sv},
    {"dz"sv, "dzo"sv, ""sv, ""sv},
    {"ee"sv, "ewe"sv, ""sv, ""sv},
    {"el"sv, "gre"sv, "ell"sv, "ell"sv},
    {"en"sv, "eng"sv, ""sv, ""sv},
    {"eo"sv, "epo"sv, ""sv, ""sv},
    {"es"sv, "spa"sv, "esp"sv, ""sv},
    {"et"sv, "est"sv, ""sv, ""sv},
    {"eu"sv, "baq"sv, ""sv, "eus"sv},
    {"fa"sv, "per"sv, ""sv, "fas"sv},
    {"ff"sv, "ful"sv, ""sv, ""sv},
    {"fi"sv, "fin"sv, ""sv, ""sv},
    {"fj"sv, "fij"sv, ""sv, ""sv},
    {"fo"sv, "fao"sv, ""sv, ""sv},
    {"fr"sv, "fre"sv, "fra"sv, "fra"sv},
    {"fy"sv, "fry"sv, ""sv, ""sv},
    {"ga"sv, "gle"sv, ""sv, ""sv},
    {"gd"sv, "gla"sv, ""sv, ""sv},
    {"gl"sv, "glg"sv, ""sv, ""sv},
    {"gn"sv, "grn"sv, ""sv, ""sv},
    {"gu"sv, "guj"sv, ""sv, ""sv},
    {"gv"sv, "glv"sv, ""sv, ""sv},
    {"ha"sv, "hau"sv, ""sv, ""sv},
    {"he"sv, "heb"sv, ""sv, ""sv},
    {"hi"sv, "hin"sv, ""sv, ""sv},
    {"ho"sv, "hmo"sv, ""sv, ""sv},
    {"hr"sv, "hrv"sv, ""sv, ""sv},
    {"ht"sv, "hat"sv, ""sv, ""sv},
    {"hu"sv, "hun"sv, ""sv, ""sv},
    {"hy"sv, "arm"sv, ""sv, "hye"sv},
    {"hz"sv, "her"sv, ""sv, ""sv},
    {"ia"sv, "ina"sv, ""sv, ""sv},
    {"id"sv, "ind"sv, ""sv, ""sv},
    {"ie"sv, "ile"sv, ""sv, ""sv},
    {"ig"sv, "ibo"sv, ""sv, ""sv},
    {"ii"sv, "iii"sv, ""sv, ""sv},
    {"ik"sv, "ipk"sv, ""sv, ""sv},
    // in deprecated https://www.loc.gov/standards/iso639-2/php/code_changes_bycode.php?code_ID=207
    {"in"sv, "ind"sv, ""sv, ""sv},
    {"io"sv, "ido"sv, ""sv, ""sv},
    {"is"sv, "ice"sv, "isl"sv, "isl"sv},
    {"it"sv, "ita"sv, ""sv, ""sv},
    {"iu"sv, "iku"sv, ""sv, ""sv},
    // iw deprecated https://www.loc.gov/standards/iso639-2/php/code_changes_bycode.php?code_ID=184
    {"iw"sv, "heb"sv, ""sv, ""sv},
    {"ja"sv, "jpn"sv, ""sv, ""sv},
    {"jv"sv, "jav"sv, ""sv, ""sv},
    {"ka"sv, "geo"sv, ""sv, "kat"sv},
    {"kg"sv, "kon"sv, ""sv, ""sv},
    {"ki"sv, "kik"sv, ""sv, ""sv},
    {"kj"sv, "kua"sv, ""sv, ""sv},
    {"kk"sv, "kaz"sv, ""sv, ""sv},
    {"kl"sv, "kal"sv, ""sv, ""sv},
    {"km"sv, "khm"sv, ""sv, ""sv},
    {"kn"sv, "kan"sv, ""sv, ""sv},
    {"ko"sv, "kor"sv, ""sv, ""sv},
    {"kr"sv, "kau"sv, ""sv, ""sv},
    {"ks"sv, "kas"sv, ""sv, ""sv},
    {"ku"sv, "kur"sv, ""sv, ""sv},
    {"kv"sv, "kom"sv, ""sv, ""sv},
    {"kw"sv, "cor"sv, ""sv, ""sv},
    {"ky"sv, "kir"sv, ""sv, ""sv},
    {"la"sv, "lat"sv, ""sv, ""sv},
    {"lb"sv, "ltz"sv, ""sv, ""sv},
    {"lg"sv, "lug"sv, ""sv, ""sv},
    {"li"sv, "lim"sv, ""sv, ""sv},
    {"ln"sv, "lin"sv, ""sv, ""sv},
    {"lo"sv, "lao"sv, ""sv, ""sv},
    {"lt"sv, "lit"sv, ""sv, ""sv},
    {"lu"sv, "lub"sv, ""sv, ""sv},
    {"lv"sv, "lav"sv, ""sv, ""sv},
    {"mg"sv, "mlg"sv, ""sv, ""sv},
    {"mh"sv, "mah"sv, ""sv, ""sv},
    {"mi"sv, "mao"sv, ""sv, "mri"sv},
    {"mk"sv, "mac"sv, ""sv, "mdk"sv},
    {"ml"sv, "mal"sv, ""sv, ""sv},
    {"mn"sv, "mon"sv, ""sv, ""sv},
    {"mr"sv, "mar"sv, ""sv, ""sv},
    {"ms"sv, "may"sv, ""sv, "msa"sv},
    {"mt"sv, "mlt"sv, ""sv, ""sv},
    {"my"sv, "bur"sv, ""sv, "mya"sv},
    {"na"sv, "nau"sv, ""sv, ""sv},
    {"nb"sv, "nob"sv, ""sv, ""sv},
    {"nd"sv, "nde"sv, ""sv, ""sv},
    {"ne"sv, "nep"sv, ""sv, ""sv},
    {"ng"sv, "ndo"sv, ""sv, ""sv},
    {"nl"sv, "dut"sv, "nld"sv, "nld"sv},
    {"nn"sv, "nno"sv, ""sv, ""sv},
    {"no"sv, "nor"sv, ""sv, ""sv},
    {"nr"sv, "nbl"sv, ""sv, ""sv},
    {"nv"sv, "nav"sv, ""sv, ""sv},
    {"ny"sv, "nya"sv, ""sv, ""sv},
    {"oc"sv, "oci"sv, ""sv, ""sv},
    {"oj"sv, "oji"sv, ""sv, ""sv},
    {"om"sv, "orm"sv, ""sv, ""sv},
    {"or"sv, "ori"sv, ""sv, ""sv},
    {"os"sv, "oss"sv, ""sv, ""sv},
    {"pa"sv, "pan"sv, ""sv, ""sv},
    // pb / pob = unofficial language code for Brazilian Portuguese
    {"pb"sv, "pob"sv, ""sv, ""sv},
    {"pi"sv, "pli"sv, ""sv, ""sv},
    {"pl"sv, "pol"sv, "plk"sv, ""sv},
    {"ps"sv, "pus"sv, ""sv, ""sv},
    {"pt"sv, "por"sv, "ptg"sv, ""sv},
    {"qu"sv, "que"sv, ""sv, ""sv},
    {"rm"sv, "roh"sv, ""sv, ""sv},
    {"rn"sv, "run"sv, ""sv, ""sv},
    {"ro"sv, "rum"sv, "ron"sv, "ron"sv},
    {"ru"sv, "rus"sv, ""sv, ""sv},
    {"rw"sv, "kin"sv, ""sv, ""sv},
    {"sa"sv, "san"sv, ""sv, ""sv},
    {"sc"sv, "srd"sv, ""sv, ""sv},
    {"sd"sv, "snd"sv, ""sv, ""sv},
    {"se"sv, "sme"sv, ""sv, ""sv},
    {"sg"sv, "sag"sv, ""sv, ""sv},
    {"sh"sv, "scr"sv, ""sv, ""sv},
    {"si"sv, "sin"sv, ""sv, ""sv},
    {"sk"sv, "slo"sv, "sky"sv, "slk"sv},
    {"sl"sv, "slv"sv, ""sv, ""sv},
    {"sm"sv, "smo"sv, ""sv, ""sv},
    {"sn"sv, "sna"sv, ""sv, ""sv},
    {"so"sv, "som"sv, ""sv, ""sv},
    {"sq"sv, "alb"sv, ""sv, "sqi"sv},
    {"sr"sv, "srp"sv, ""sv, ""sv},
    {"ss"sv, "ssw"sv, ""sv, ""sv},
    {"st"sv, "sot"sv, ""sv, ""sv},
    {"su"sv, "sun"sv, ""sv, ""sv},
    {"sv"sv, "swe"sv, "sve"sv, ""sv},
    {"sw"sv, "swa"sv, ""sv, ""sv},
    {"ta"sv, "tam"sv, ""sv, ""sv},
    {"te"sv, "tel"sv, ""sv, ""sv},
    {"tg"sv, "tgk"sv, ""sv, ""sv},
    {"th"sv, "tha"sv, ""sv, ""sv},
    {"ti"sv, "tir"sv, ""sv, ""sv},
    {"tk"sv, "tuk"sv, ""sv, ""sv},
    {"tl"sv, "tgl"sv, ""sv, ""sv},
    {"tn"sv, "tsn"sv, ""sv, ""sv},
    {"to"sv, "ton"sv, ""sv, ""sv},
    {"tr"sv, "tur"sv, "trk"sv, ""sv},
    {"ts"sv, "tso"sv, ""sv, ""sv},
    {"tt"sv, "tat"sv, ""sv, ""sv},
    {"tw"sv, "twi"sv, ""sv, ""sv},
    {"ty"sv, "tah"sv, ""sv, ""sv},
    {"ug"sv, "uig"sv, ""sv, ""sv},
    {"uk"sv, "ukr"sv, ""sv, ""sv},
    {"ur"sv, "urd"sv, ""sv, ""sv},
    {"uz"sv, "uzb"sv, ""sv, ""sv},
    {"ve"sv, "ven"sv, ""sv, ""sv},
    {"vi"sv, "vie"sv, ""sv, ""sv},
    {"vo"sv, "vol"sv, ""sv, ""sv},
    {"wa"sv, "wln"sv, ""sv, ""sv},
    {"wo"sv, "wol"sv, ""sv, ""sv},
    {"xh"sv, "xho"sv, ""sv, ""sv},
    {"yi"sv, "yid"sv, ""sv, ""sv},
    {"yo"sv, "yor"sv, ""sv, ""sv},
    {"za"sv, "zha"sv, ""sv, ""sv},
    {"zh"sv, "chi"sv, "zho"sv, "zho"sv},
    {"zu"sv, "zul"sv, ""sv, ""sv},
    {"zv"sv, "und"sv, ""sv, ""sv}, // Kodi intern mapping for missing "Undetermined" iso639-1 code
    {"zx"sv, "zxx"sv, ""sv, ""sv}, // Kodi intern mapping for missing "No linguistic content" iso639-1 code
    {"zy"sv, "mis"sv, ""sv, ""sv}, // Kodi intern mapping for missing "Miscellaneous languages" iso639-1 code
    {"zz"sv, "mul"sv, ""sv, ""sv} // Kodi intern mapping for missing "Multiple languages" iso639-1 code
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

constexpr auto LanguageCodesByIso639_2b = CreateLanguageCodesSortedByIso639_2b();
constexpr auto LanguageCodesByWin_Id = CreateLanguageCodesSortedByWin_Id();
