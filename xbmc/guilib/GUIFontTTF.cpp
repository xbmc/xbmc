/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIFontTTF.h"

#include "GUIFontManager.h"
#include "LangInfo.h"
#include "ServiceBroker.h"
#include "Texture.h"
#include "URL.h"
#include "addons/AddonInstaller.h"
#include "addons/AddonManager.h"
#include "addons/LanguageResource.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "guilib/LocalizeStrings.h"
#include "messaging/helpers/DialogHelper.h"
#include "messaging/helpers/DialogOKHelper.h"
#include "rendering/RenderSystem.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "threads/SystemClock.h"
#include "utils/MathUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"
#include "windowing/GraphicContext.h"
#include "windowing/WinSystem.h"

#include <math.h>
#include <memory>
#include <queue>
#include <unordered_map>
#include <utility>

// stuff for freetype
#include <ft2build.h>
#include <harfbuzz/hb-ft.h>
#if defined(HAS_GL) || defined(HAS_GLES)
#include "utils/GLUtils.h"

#include "system_gl.h"
#endif

#if defined(HAS_DX)
#include "guilib/D3DResource.h"
#endif

#ifdef TARGET_WINDOWS_STORE
#define generic GenericFromFreeTypeLibrary
#endif

#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_OUTLINE_H
#include FT_STROKER_H

namespace
{
constexpr int VERTEX_PER_GLYPH = 4; // number of vertex for each glyph
constexpr int CHARS_PER_TEXTURE_LINE = 20; // number characters to cache per texture line
constexpr int MAX_TRANSLATED_VERTEX = 32; // max number of structs CTranslatedVertices expect to use
constexpr int MAX_GLYPHS_PER_TEXT_LINE = 1024; // max number of glyphs per text line expect to use
constexpr unsigned int SPACING_BETWEEN_CHARACTERS_IN_TEXTURE = 1;
constexpr int CHAR_CHUNK = 64; // 64 chars allocated at a time (2048 bytes)
constexpr int GLYPH_STRENGTH_BOLD = 24;
constexpr int GLYPH_STRENGTH_LIGHT = -48;
constexpr int TAB_SPACE_LENGTH = 4;

constexpr std::chrono::hours JOB_RETRY_TIMEOUT{24};
constexpr std::chrono::hours JOB_CANCEL_TIMEOUT{std::numeric_limits<std::chrono::hours>::max()};

static const character_t DUMMY_POINT('.', FONT_STYLE_NORMAL, UTILS::COLOR::INDEX_DEFAULT);
static const character_t DUMMY_SPACE('X', FONT_STYLE_NORMAL, UTILS::COLOR::INDEX_DEFAULT);

static constexpr std::array<std::pair<CFontTable::ADDON, const char*>, 20> fontAddons = {
    {{CFontTable::ADDON::RESOURCE_LANGUAGE_AM_ET, "resource.language.am_et"},
     {CFontTable::ADDON::RESOURCE_LANGUAGE_AR_SA, "resource.language.ar_sa"},
     {CFontTable::ADDON::RESOURCE_LANGUAGE_HE_IL, "resource.language.he_il"},
     {CFontTable::ADDON::RESOURCE_LANGUAGE_KO_KR, "resource.language.ko_kr"},
     {CFontTable::ADDON::RESOURCE_LANGUAGE_KN_IN, "resource.language.kn_in"},
     {CFontTable::ADDON::RESOURCE_LANGUAGE_HI_IN, "resource.language.hi_in"},
     {CFontTable::ADDON::RESOURCE_LANGUAGE_HY_AM, "resource.language.hy_am"},
     {CFontTable::ADDON::RESOURCE_LANGUAGE_JA_JP, "resource.language.ja_jp"},
     {CFontTable::ADDON::RESOURCE_LANGUAGE_ML_IN, "resource.language.ml_in"},
     {CFontTable::ADDON::RESOURCE_LANGUAGE_MY_MM, "resource.language.my_mm"},
     {CFontTable::ADDON::RESOURCE_LANGUAGE_SI_LK, "resource.language.si_lk"},
     {CFontTable::ADDON::RESOURCE_LANGUAGE_TA_IN, "resource.language.ta_in"},
     {CFontTable::ADDON::RESOURCE_LANGUAGE_TE_IN, "resource.language.te_in"},
     {CFontTable::ADDON::RESOURCE_LANGUAGE_TH_TH, "resource.language.th_th"},
     {CFontTable::ADDON::RESOURCE_LANGUAGE_ZH_CN, "resource.language.zh_cn"},
     {CFontTable::ADDON::RESOURCE_LANGUAGE_ZH_TW, "resource.language.zh_tw"},
     {CFontTable::ADDON::RESOURCE_FONT_ACTIVE, "resource.font.active"},
     {CFontTable::ADDON::RESOURCE_FONT_LIMITED, "resource.font.limited"},
     {CFontTable::ADDON::RESOURCE_FONT_EXCLUDED, "resource.font.excluded"},
     {CFontTable::ADDON::RESOURCE_FONT_EMOJI, "resource.font.coloremoji"}}};

// List about available characters:
// - https://www.utf8-chartable.de/unicode-utf8-table.pl
// - https://en.wikipedia.org/wiki/Unicode_block
// List about language code:
// - https://help.phrase.com/help/language-codes
//
// clang-format off
static constexpr std::array<CFontTable, 281> fontFallbacks = {
  {{0x000000, 0x00007F, "NotoSans-Regular.ttf", CFontTable::ALIGN::LTOR, "Basic Latin", CFontTable::ADDON::IN_KODI},
   {0x000080, 0x0000FF, "NotoSans-Regular.ttf", CFontTable::ALIGN::LTOR, "Latin-1 Supplement", CFontTable::ADDON::IN_KODI},
   {0x000100, 0x00017F, "NotoSans-Regular.ttf", CFontTable::ALIGN::LTOR, "Latin-1 Extended-A", CFontTable::ADDON::IN_KODI},
   {0x000180, 0x00024F, "NotoSans-Regular.ttf", CFontTable::ALIGN::LTOR, "Latin-1 Extended-B", CFontTable::ADDON::IN_KODI},
   {0x000250, 0x0002AF, "NotoSans-Regular.ttf", CFontTable::ALIGN::LTOR, "IPA Extensions", CFontTable::ADDON::IN_KODI},
   {0x0002B0, 0x0002FF, "NotoSans-Regular.ttf", CFontTable::ALIGN::LTOR, "Spacing Modifier Letters", CFontTable::ADDON::IN_KODI},
   {0x000300, 0x00036F, "NotoSans-Regular.ttf", CFontTable::ALIGN::LTOR, "Combining Diacritical Marks", CFontTable::ADDON::IN_KODI},
   {0x000370, 0x0003FF, "NotoSans-Regular.ttf", CFontTable::ALIGN::LTOR, "Greek and Coptic", CFontTable::ADDON::IN_KODI},
   {0x000400, 0x0004FF, "NotoSans-Regular.ttf", CFontTable::ALIGN::LTOR, "Cyrillic", CFontTable::ADDON::IN_KODI},
   {0x000500, 0x00052F, "NotoSans-Regular.ttf", CFontTable::ALIGN::LTOR, "Cyrillic Supplement", CFontTable::ADDON::IN_KODI},
   {0x000530, 0x00058F, "NotoSansArmenian-Regular.ttf", CFontTable::ALIGN::LTOR, "Armenian", CFontTable::ADDON::RESOURCE_LANGUAGE_HY_AM},
   {0x000590, 0x0005FF, "NotoSansHebrew-Regular.ttf", CFontTable::ALIGN::RTOL, "Hebrew", CFontTable::ADDON::RESOURCE_LANGUAGE_HE_IL},
   {0x000600, 0x0006FF, "NotoSansArabic-Regular.ttf", CFontTable::ALIGN::RTOL, "Arabic", CFontTable::ADDON::RESOURCE_LANGUAGE_AR_SA},
   {0x000700, 0x00074F, "NotoSansSyriac-Regular.ttf", CFontTable::ALIGN::RTOL, "Syriac", CFontTable::ADDON::RESOURCE_FONT_LIMITED},
   {0x000750, 0x00077F, "NotoSansArabic-Regular.ttf", CFontTable::ALIGN::RTOL, "Arabic Supplement", CFontTable::ADDON::RESOURCE_LANGUAGE_AR_SA},
   {0x000780, 0x0007BF, "NotoSansThaana-Regular.ttf", CFontTable::ALIGN::RTOL, "Thaana", CFontTable::ADDON::RESOURCE_FONT_ACTIVE}, /* dv_mv */
   {0x0007C0, 0x0007FF, "NotoSansNKo-Regular.ttf", CFontTable::ALIGN::RTOL, "NKo", CFontTable::ADDON::RESOURCE_FONT_LIMITED},
   {0x000800, 0x00083F, "NotoSansSamaritan-Regular.ttf", CFontTable::ALIGN::RTOL, "Samaritan", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x000840, 0x00085F, "NotoSansMandaic-Regular.ttf", CFontTable::ALIGN::RTOL, "Mandaic", CFontTable::ADDON::RESOURCE_FONT_LIMITED},
   {0x000860, 0x00086F, "NotoSansSyriac-Regular.ttf", CFontTable::ALIGN::RTOL, "Syriac Supplement", CFontTable::ADDON::RESOURCE_FONT_LIMITED},
   {0x0008A0, 0x0008FF, "NotoSansArabic-Regular.ttf", CFontTable::ALIGN::RTOL, "Arabic Extended-A", CFontTable::ADDON::RESOURCE_LANGUAGE_AR_SA},
   {0x000900, 0x00097F, "NotoSansDevanagari-Regular.ttf", CFontTable::ALIGN::LTOR, "Devanagari", CFontTable::ADDON::RESOURCE_LANGUAGE_HI_IN},
   {0x000980, 0x0009FF, "NotoSansBengali-Regular.ttf", CFontTable::ALIGN::LTOR, "Bengali", CFontTable::ADDON::RESOURCE_FONT_ACTIVE}, /* bn_in */
   {0x000A00, 0x000A7F, "NotoSansGurmukhi-Regular.ttf", CFontTable::ALIGN::LTOR, "Gurmukhi", CFontTable::ADDON::RESOURCE_FONT_ACTIVE}, /* pa_in */
   {0x000A80, 0x000AFF, "NotoSansGujarati-Regular.ttf", CFontTable::ALIGN::LTOR, "Gujarati", CFontTable::ADDON::RESOURCE_FONT_ACTIVE}, /* gu_in */
   {0x000B00, 0x000B7F, "NotoSansOriya-Regular.ttf", CFontTable::ALIGN::LTOR, "Oriya", CFontTable::ADDON::RESOURCE_FONT_ACTIVE}, /* or_in */
   {0x000B80, 0x000BFF, "NotoSansTamil-Regular.ttf", CFontTable::ALIGN::LTOR, "Tamil", CFontTable::ADDON::RESOURCE_LANGUAGE_TA_IN},
   {0x000C00, 0x000C7F, "NotoSansTelugu-Regular.ttf", CFontTable::ALIGN::LTOR, "Telugu", CFontTable::ADDON::RESOURCE_LANGUAGE_TE_IN},
   {0x000C80, 0x000CFF, "NotoSansKannada-Regular.ttf", CFontTable::ALIGN::LTOR, "Kannada", CFontTable::ADDON::RESOURCE_LANGUAGE_KN_IN},
   {0x000D00, 0x000D7F, "NotoSansMalayalam-Regular.ttf", CFontTable::ALIGN::LTOR, "Malayalam", CFontTable::ADDON::RESOURCE_LANGUAGE_ML_IN},
   {0x000D80, 0x000DFF, "NotoSansSinhala-Regular.ttf", CFontTable::ALIGN::LTOR, "Sinhala", CFontTable::ADDON::RESOURCE_LANGUAGE_SI_LK},
   {0x000E00, 0x000E7F, "NotoSansThai-Regular.ttf", CFontTable::ALIGN::LTOR, "Thai", CFontTable::ADDON::RESOURCE_LANGUAGE_TH_TH},
   {0x000E80, 0x000EFF, "NotoSansLao-Regular.ttf", CFontTable::ALIGN::LTOR, "Lao", CFontTable::ADDON::RESOURCE_FONT_ACTIVE}, /* lo_la */
   {0x000F00, 0x000FFF, "NotoSansTibetan-Regular.ttf", CFontTable::ALIGN::LTOR, "Tibetan", CFontTable::ADDON::RESOURCE_FONT_ACTIVE}, /* bo_cn & bo_in */
   {0x001000, 0x00109F, "NotoSansMyanmar-Regular.ttf", CFontTable::ALIGN::LTOR, "Myanmar", CFontTable::ADDON::RESOURCE_LANGUAGE_MY_MM},
   {0x0010A0, 0x0010FF, "NotoSansGeorgian-Regular.ttf", CFontTable::ALIGN::LTOR, "Georgian", CFontTable::ADDON::RESOURCE_FONT_ACTIVE}, /* ka_ge */
   {0x001100, 0x0011FF, "NotoSansKR-Regular.ttf", CFontTable::ALIGN::LTOR, "Hangul Jamo", CFontTable::ADDON::RESOURCE_LANGUAGE_KO_KR},
   {0x001200, 0x00137F, "NotoSansEthiopic-Regular.ttf", CFontTable::ALIGN::LTOR, "Ethiopic", CFontTable::ADDON::RESOURCE_LANGUAGE_AM_ET},
   {0x001380, 0x00139F, "NotoSansEthiopic-Regular.ttf", CFontTable::ALIGN::LTOR, "Ethiopic Supplement", CFontTable::ADDON::RESOURCE_LANGUAGE_AM_ET},
   {0x0013A0, 0x0013FF, "NotoSansCherokee-Regular.ttf", CFontTable::ALIGN::LTOR, "Cherokee", CFontTable::ADDON::RESOURCE_FONT_LIMITED},
   {0x001400, 0x00167F, "NotoSansCanadianAboriginal-Regular.ttf", CFontTable::ALIGN::LTOR, "Unified Canadian Aboriginal Syllabics", CFontTable::ADDON::RESOURCE_FONT_LIMITED},
   {0x001680, 0x00169F, "NotoSansOgham-Regular.ttf", CFontTable::ALIGN::LTOR, "Ogham", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x0016A0, 0x0016FF, "NotoSansRunic-Regular.ttf", CFontTable::ALIGN::LTOR, "Runic", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x001700, 0x00171F, "NotoSansTagalog-Regular.ttf", CFontTable::ALIGN::LTOR, "Tagalog", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x001720, 0x00173F, "NotoSansHanunoo-Regular.ttf", CFontTable::ALIGN::LTOR, "Hanunoo", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x001740, 0x00175F, "NotoSansBuhid-Regular.ttf", CFontTable::ALIGN::LTOR, "Buhid", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x001760, 0x00177F, "NotoSansTagbanwa-Regular.ttf", CFontTable::ALIGN::LTOR, "Tagbanwa", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x001780, 0x0017FF, "NotoSansKhmer-Regular.ttf", CFontTable::ALIGN::LTOR, "Khmer", CFontTable::ADDON::RESOURCE_FONT_ACTIVE}, /* km_kh */
   {0x001800, 0x0018AF, "NotoSansMongolian-Regular.ttf", CFontTable::ALIGN::LTOR, "Mongolian", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x0018B0, 0x0018FF, "NotoSansCanadianAboriginal-Regular.ttf", CFontTable::ALIGN::LTOR, "Unified Canadian Aboriginal Syllabics Extended", CFontTable::ADDON::RESOURCE_FONT_LIMITED},
   {0x001900, 0x00194F, "NotoSansLimbu-Regular.ttf", CFontTable::ALIGN::LTOR, "Limbu", CFontTable::ADDON::RESOURCE_FONT_LIMITED},
   {0x001950, 0x00197F, "NotoSansTaiLe-Regular.ttf", CFontTable::ALIGN::LTOR, "Tai Le", CFontTable::ADDON::RESOURCE_FONT_LIMITED},
   {0x001980, 0x0019DF, "NotoSansNewTaiLue-Regular.ttf", CFontTable::ALIGN::LTOR, "New Tai Lue", CFontTable::ADDON::RESOURCE_FONT_LIMITED},
   {0x0019E0, 0x0019FF, "NotoSansKhmer-Regular.ttf", CFontTable::ALIGN::LTOR, "Khmer Symbols", CFontTable::ADDON::RESOURCE_FONT_ACTIVE}, /* km_kh */
   {0x001A00, 0x001A1F, "NotoSansBuginese-Regular.ttf", CFontTable::ALIGN::LTOR, "Buginese", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x001A20, 0x001AAF, "NotoSansTaiTham-Regular.ttf", CFontTable::ALIGN::LTOR, "Tai Tham", CFontTable::ADDON::RESOURCE_FONT_LIMITED},
   {0x001AB0, 0x001AFF, "NotoSans-Regular.ttf", CFontTable::ALIGN::LTOR, "Combining Diacritical Marks Extended", CFontTable::ADDON::IN_KODI},
   {0x001B00, 0x001B7F, "NotoSansBalinese-Regular.ttf", CFontTable::ALIGN::LTOR, "Balinese", CFontTable::ADDON::RESOURCE_FONT_LIMITED},
   {0x001B80, 0x001BBF, "NotoSansSundanese-Regular.ttf", CFontTable::ALIGN::LTOR, "Sundanese", CFontTable::ADDON::RESOURCE_FONT_LIMITED},
   {0x001BC0, 0x001BFF, "NotoSansBatak-Regular.ttf", CFontTable::ALIGN::LTOR, "Batak", CFontTable::ADDON::RESOURCE_FONT_LIMITED},
   {0x001C00, 0x001C4F, "NotoSansLepcha-Regular.ttf", CFontTable::ALIGN::LTOR, "Lepcha", CFontTable::ADDON::RESOURCE_FONT_LIMITED},
   {0x001C50, 0x001C7F, "NotoSansOlChiki-Regular.ttf", CFontTable::ALIGN::LTOR, "Ol Chiki", CFontTable::ADDON::RESOURCE_FONT_LIMITED},
   {0x001C80, 0x001C8F, "NotoSans-Regular.ttf", CFontTable::ALIGN::LTOR, "Cyrillic Extended-C", CFontTable::ADDON::IN_KODI},
   {0x001C90, 0x001CBF, "NotoSansGeorgian-Regular.ttf", CFontTable::ALIGN::LTOR, "Georgian Extended", CFontTable::ADDON::RESOURCE_FONT_ACTIVE}, /* ka_ge */
   {0x001CC0, 0x001CCF, "NotoSansSundanese-Regular.ttf", CFontTable::ALIGN::LTOR, "Sundanese Supplement", CFontTable::ADDON::RESOURCE_FONT_LIMITED},
   {0x001CD0, 0x001CFF, "NotoSansDevanagari-Regular.ttf", CFontTable::ALIGN::LTOR, "Vedik Extension", CFontTable::ADDON::RESOURCE_LANGUAGE_HI_IN},
   {0x001D00, 0x001D7F, "NotoSans-Regular.ttf", CFontTable::ALIGN::LTOR, "Phonetic Extensions", CFontTable::ADDON::IN_KODI},
   {0x001D80, 0x001DBF, "NotoSans-Regular.ttf", CFontTable::ALIGN::LTOR, "Phonetic Extensions Supplement", CFontTable::ADDON::IN_KODI},
   {0x001DC0, 0x001DFF, "NotoSans-Regular.ttf", CFontTable::ALIGN::LTOR, "Combining Diacritical Marks Supplement", CFontTable::ADDON::IN_KODI},
   {0x001E00, 0x001EFF, "NotoSans-Regular.ttf", CFontTable::ALIGN::LTOR, "Latin-1 Extended Additional", CFontTable::ADDON::IN_KODI},
   {0x001F00, 0x001FFF, "NotoSans-Regular.ttf", CFontTable::ALIGN::LTOR, "Greek Extended", CFontTable::ADDON::IN_KODI},
   {0x002000, 0x00206F, "NotoSans-Regular.ttf", CFontTable::ALIGN::LTOR, "General Punctuation", CFontTable::ADDON::IN_KODI},
   {0x002070, 0x00209F, "NotoSans-Regular.ttf", CFontTable::ALIGN::LTOR, "Superscripts and Subscripts", CFontTable::ADDON::IN_KODI},
   {0x0020A0, 0x0020CF, "NotoSansDisplay-Regular.ttf", CFontTable::ALIGN::LTOR, "Currency Symbols", CFontTable::ADDON::IN_KODI},
   {0x0020D0, 0x0020FF, "NotoSansSymbols-Regular.ttf", CFontTable::ALIGN::LTOR, "Combining Diacritical Marks for Symbols", CFontTable::ADDON::IN_KODI},
   {0x002100, 0x00214F, "NotoSansDisplay-Regular.ttf", CFontTable::ALIGN::LTOR, "Letterlike Symbols", CFontTable::ADDON::IN_KODI},
   {0x002150, 0x00218F, "NotoSansSymbols-Regular.ttf", CFontTable::ALIGN::LTOR,  "Number Forms", CFontTable::ADDON::IN_KODI},
   {0x002190, 0x0021FF, "NotoSansMath-Regular.ttf", CFontTable::ALIGN::LTOR, "Arrows", CFontTable::ADDON::IN_KODI},
   {0x002200, 0x0022FF, "NotoSansMath-Regular.ttf", CFontTable::ALIGN::LTOR, "Mathematical Operations", CFontTable::ADDON::IN_KODI},
   {0x002300, 0x0023FF, "NotoSansSymbols2-Regular.ttf", CFontTable::ALIGN::LTOR, "Miscellaneous Technical", CFontTable::ADDON::IN_KODI},
   {0x002400, 0x00243F, "NotoSansSymbols2-Regular.ttf", CFontTable::ALIGN::LTOR, "Control Pictures", CFontTable::ADDON::IN_KODI},
   {0x002440, 0x00245F, "NotoSansSymbols2-Regular.ttf", CFontTable::ALIGN::LTOR, "Optical Chracter Recognition", CFontTable::ADDON::IN_KODI},
   {0x002460, 0x0024FF, "NotoSansSymbols2-Regular.ttf", CFontTable::ALIGN::LTOR, "Enclosed Alphanumerics", CFontTable::ADDON::IN_KODI},
   {0x002500, 0x00257F, "NotoSansMono-Regular.ttf", CFontTable::ALIGN::LTOR, "Box Drawing", CFontTable::ADDON::IN_KODI},
   {0x002580, 0x00259F, "NotoSansMono-Regular.ttf", CFontTable::ALIGN::LTOR, "Block Elements", CFontTable::ADDON::IN_KODI},
   {0x0025A0, 0x0025FF, "NotoSansSymbols2-Regular.ttf", CFontTable::ALIGN::LTOR, "Geometric Shapes", CFontTable::ADDON::IN_KODI},
   {0x002600, 0x0026FF, "NotoSansSymbols2-Regular.ttf", CFontTable::ALIGN::LTOR, "Miscellaneous Symbols", CFontTable::ADDON::IN_KODI},
   {0x002700, 0x0027BF, "NotoSansSymbols2-Regular.ttf", CFontTable::ALIGN::LTOR, "Dingbats", CFontTable::ADDON::IN_KODI},
   {0x0027C0, 0x0027EF, "NotoSansMath-Regular.ttf", CFontTable::ALIGN::LTOR, "Miscellaneous Mathematical Symbols-A", CFontTable::ADDON::IN_KODI},
   {0x0027F0, 0x0027FF, "NotoSansMath-Regular.ttf", CFontTable::ALIGN::LTOR, "Supplemental Arrows-A", CFontTable::ADDON::IN_KODI},
   {0x002800, 0x0028FF, "NotoSansSymbols2-Regular.ttf", CFontTable::ALIGN::LTOR, "Braille Pattern", CFontTable::ADDON::IN_KODI},
   {0x002900, 0x00297F, "NotoSansMath-Regular.ttf", CFontTable::ALIGN::LTOR, "Supplemental Arrows-B", CFontTable::ADDON::IN_KODI},
   {0x002980, 0x0029FF, "NotoSansMath-Regular.ttf", CFontTable::ALIGN::LTOR, "Miscellaneous Mathematical Symbols-B", CFontTable::ADDON::IN_KODI},
   {0x002A00, 0x002AFF, "NotoSansMath-Regular.ttf", CFontTable::ALIGN::LTOR, "Supplemental Mathematic Operators", CFontTable::ADDON::IN_KODI},
   {0x002B00, 0x002BFF, "NotoSansSymbols2-Regular.ttf", CFontTable::ALIGN::LTOR, "Miscellaneous Symbols and Arrows", CFontTable::ADDON::IN_KODI},
   {0x002C00, 0x002C5F, "NotoSansGlagolitic-Regular.ttf", CFontTable::ALIGN::LTOR, "Glagolitic", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x002C60, 0x002C7F, "NotoSans-Regular.ttf", CFontTable::ALIGN::LTOR, "Latin Extended-C", CFontTable::ADDON::IN_KODI},
   {0x002C80, 0x002CFF, "NotoSansCoptic-Regular.ttf", CFontTable::ALIGN::LTOR, "Coptic", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x002D00, 0x002D2F, "NotoSansGeorgian-Regular.ttf", CFontTable::ALIGN::LTOR, "Georgian Supplement", CFontTable::ADDON::RESOURCE_FONT_ACTIVE}, /* ka_ge */
   {0x002D30, 0x002D7F, "NotoSansTifinagh-Regular.ttf", CFontTable::ALIGN::LTOR, "Tifinagh", CFontTable::ADDON::RESOURCE_FONT_LIMITED},
   {0x002D80, 0x002DDF, "NotoSansEthiopic-Regular.ttf", CFontTable::ALIGN::LTOR, "Ethiopic Extended", CFontTable::ADDON::RESOURCE_LANGUAGE_AM_ET},
   {0x002DE0, 0x002DFF, "NotoSans-Regular.ttf", CFontTable::ALIGN::LTOR, "Cyrillic Extended-A", CFontTable::ADDON::IN_KODI},
   {0x002E00, 0x002E7F, "NotoSans-Regular.ttf", CFontTable::ALIGN::LTOR, "Supplemental Punctuation", CFontTable::ADDON::IN_KODI},
   {0x002E80, 0x002EFF, "NotoSansSC-Regular.ttf", CFontTable::ALIGN::LTOR, "CJK Radicals Supplement", CFontTable::ADDON::RESOURCE_LANGUAGE_ZH_CN},
   {0x002F00, 0x002FDF, "NotoSansSC-Regular.ttf", CFontTable::ALIGN::LTOR, "Kangxi Radicals", CFontTable::ADDON::RESOURCE_LANGUAGE_ZH_CN},
   {0x002FF0, 0x002FFF, "NotoSansSC-Regular.ttf", CFontTable::ALIGN::LTOR, "Ideographic Description Character", CFontTable::ADDON::RESOURCE_LANGUAGE_ZH_CN},
   {0x003000, 0x00303F, "NotoSansSC-Regular.ttf", CFontTable::ALIGN::LTOR, "CJK Symbols and Punctuation", CFontTable::ADDON::RESOURCE_LANGUAGE_ZH_CN},
   {0x003040, 0x00309F, "NotoSansSC-Regular.ttf", CFontTable::ALIGN::LTOR, "Hiragana", CFontTable::ADDON::RESOURCE_LANGUAGE_ZH_CN},
   {0x0030A0, 0x0030FF, "NotoSansSC-Regular.ttf", CFontTable::ALIGN::LTOR, "Katakana", CFontTable::ADDON::RESOURCE_LANGUAGE_ZH_CN},
   {0x003100, 0x00312F, "NotoSansSC-Regular.ttf", CFontTable::ALIGN::LTOR, "Bopomofo", CFontTable::ADDON::RESOURCE_LANGUAGE_ZH_CN},
   {0x003130, 0x00318F, "NotoSansSC-Regular.ttf", CFontTable::ALIGN::LTOR, "Hangul Compatibility Jamo", CFontTable::ADDON::RESOURCE_LANGUAGE_ZH_CN},
   {0x003190, 0x00319F, "NotoSansSC-Regular.ttf", CFontTable::ALIGN::LTOR, "Kanbun", CFontTable::ADDON::RESOURCE_LANGUAGE_ZH_CN},
   {0x0031A0, 0x0031BF, "NotoSansSC-Regular.ttf", CFontTable::ALIGN::LTOR, "Bopomofo Extended", CFontTable::ADDON::RESOURCE_LANGUAGE_ZH_CN},
   {0x0031C0, 0x0031EF, "NotoSansSC-Regular.ttf", CFontTable::ALIGN::LTOR, "CJK Strokes", CFontTable::ADDON::RESOURCE_LANGUAGE_ZH_CN},
   {0x0031F0, 0x0031FF, "NotoSansSC-Regular.ttf", CFontTable::ALIGN::LTOR, "Katakana Phonetic Extensions", CFontTable::ADDON::RESOURCE_LANGUAGE_ZH_CN},
   {0x003200, 0x0032FF, "NotoSansSC-Regular.ttf", CFontTable::ALIGN::LTOR, "Enclosed CJK Letters and Months", CFontTable::ADDON::RESOURCE_LANGUAGE_ZH_CN},
   {0x003300, 0x0033FF, "NotoSansSC-Regular.ttf", CFontTable::ALIGN::LTOR, "CJK Compatibility", CFontTable::ADDON::RESOURCE_LANGUAGE_ZH_CN},
   {0x003400, 0x004DBF, "NotoSansSC-Regular.ttf", CFontTable::ALIGN::LTOR, "CJK Unified Ideographics Extensions A", CFontTable::ADDON::RESOURCE_LANGUAGE_ZH_CN},
   {0x004DC0, 0x004DFF, "NotoSansSymbols2-Regular.ttf", CFontTable::ALIGN::LTOR, "Yijing Hexagram Symbols", CFontTable::ADDON::IN_KODI},
   {0x004E00, 0x009FFF, "NotoSansSC-Regular.ttf", CFontTable::ALIGN::LTOR, "CJK Unified Ideographics", CFontTable::ADDON::RESOURCE_LANGUAGE_ZH_CN},
   {0x00A000, 0x00A48F, "NotoSansYi-Regular.ttf", CFontTable::ALIGN::LTOR, "Yi Syllables", CFontTable::ADDON::RESOURCE_FONT_LIMITED},
   {0x00A490, 0x00A4CF, "NotoSansYi-Regular.ttf", CFontTable::ALIGN::LTOR, "Yi Radicals", CFontTable::ADDON::RESOURCE_FONT_LIMITED},
   {0x00A4D0, 0x00A4FF, "NotoSansLisu-Regular.ttf", CFontTable::ALIGN::LTOR, "Lisu", CFontTable::ADDON::RESOURCE_FONT_LIMITED},
   {0x00A500, 0x00A63F, "NotoSansVai-Regular.ttf", CFontTable::ALIGN::LTOR, "Vai", CFontTable::ADDON::RESOURCE_FONT_LIMITED},
   {0x00A640, 0x00A69F, "NotoSans-Regular.ttf", CFontTable::ALIGN::LTOR, "Cyrillic Extended-B", CFontTable::ADDON::IN_KODI},
   {0x00A6A0, 0x00A6FF, "NotoSansBamum-Regular.ttf", CFontTable::ALIGN::LTOR, "Bamum", CFontTable::ADDON::RESOURCE_FONT_LIMITED},
   {0x00A700, 0x00A71F, "NotoSans-Regular.ttf", CFontTable::ALIGN::LTOR, "Modifier Tone Letters", CFontTable::ADDON::IN_KODI},
   {0x00A720, 0x00A7FF, "NotoSans-Regular.ttf", CFontTable::ALIGN::LTOR, "Latin Extended-D", CFontTable::ADDON::IN_KODI},
   {0x00A800, 0x00A82F, "NotoSansSylotiNagri-Regular.ttf", CFontTable::ALIGN::LTOR, "Syloti Nagri", CFontTable::ADDON::RESOURCE_FONT_LIMITED},
   {0x00A830, 0x00A83F, "NotoSansKannada-Regular.ttf", CFontTable::ALIGN::LTOR, "Common Indic Number Forms", CFontTable::ADDON::RESOURCE_LANGUAGE_KN_IN},
   {0x00A840, 0x00A87F, "NotoSansPhagsPa-Regular.ttf", CFontTable::ALIGN::LTOR, "Phags-pa", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x00A880, 0x00A8DF, "NotoSansSaurashtra-Regular.ttf", CFontTable::ALIGN::LTOR, "Saurashtra", CFontTable::ADDON::RESOURCE_FONT_LIMITED},
   {0x00A8E0, 0x00A8FF, "NotoSansDevanagari-Regular.ttf", CFontTable::ALIGN::LTOR, "Devanagari Extended", CFontTable::ADDON::RESOURCE_LANGUAGE_HI_IN},
   {0x00A900, 0x00A92F, "NotoSansKayahLi-Regular.ttf", CFontTable::ALIGN::LTOR, "Kayah Li", CFontTable::ADDON::RESOURCE_FONT_LIMITED},
   {0x00A930, 0x00A95F, "NotoSansRejang-Regular.ttf", CFontTable::ALIGN::LTOR, "Rejang", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x00A960, 0x00A97F, "NotoSansKR-Regular.ttf", CFontTable::ALIGN::LTOR, "Hangul Jamo Extended-A", CFontTable::ADDON::RESOURCE_LANGUAGE_KO_KR},
   {0x00A980, 0x00A9DF, "NotoSansJavanese-Regular.ttf", CFontTable::ALIGN::LTOR, "Javanese", CFontTable::ADDON::RESOURCE_FONT_LIMITED},
   {0x00A9E0, 0x00A9FF, "NotoSansMyanmar-Regular.ttf", CFontTable::ALIGN::LTOR, "Myanmar Extended-B", CFontTable::ADDON::IN_KODI},
   {0x00AA00, 0x00AA5F, "NotoSansCham-Regular.ttf", CFontTable::ALIGN::LTOR, "Cham", CFontTable::ADDON::RESOURCE_FONT_LIMITED},
   {0x00AA60, 0x00AA7F, "NotoSansMyanmar-Regular.ttf", CFontTable::ALIGN::LTOR, "Myanmar Extended-A", CFontTable::ADDON::IN_KODI},
   {0x00AA80, 0x00AADF, "NotoSansTaiViet-Regular.ttf", CFontTable::ALIGN::LTOR, "Tai Viet", CFontTable::ADDON::RESOURCE_FONT_LIMITED},
   {0x00AAE0, 0x00AAFF, "NotoSansMeeteiMayek-Regular.ttf", CFontTable::ALIGN::LTOR, "Meetei Mayek Extension", CFontTable::ADDON::RESOURCE_FONT_LIMITED},
   {0x00AB00, 0x00AB2F, "NotoSansEthiopic-Regular.ttf", CFontTable::ALIGN::LTOR, "Ethiopic Extension-A", CFontTable::ADDON::RESOURCE_LANGUAGE_AM_ET},
   {0x00AB30, 0x00AB6F, "NotoSans-Regular.ttf", CFontTable::ALIGN::LTOR, "Latin Extended-E", CFontTable::ADDON::IN_KODI},
   {0x00AB70, 0x00ABBF, "NotoSansCherokee-Regular.ttf", CFontTable::ALIGN::LTOR, "Cherokee Supplement", CFontTable::ADDON::RESOURCE_FONT_LIMITED},
   {0x00ABC0, 0x00ABFF, "NotoSansMeeteiMayek-Regular.ttf", CFontTable::ALIGN::LTOR, "Meetei Mayek", CFontTable::ADDON::RESOURCE_FONT_LIMITED},
   {0x00AC00, 0x00D7AF, "NotoSansKR-Regular.ttf", CFontTable::ALIGN::LTOR, "Hangul Syllable", CFontTable::ADDON::RESOURCE_LANGUAGE_KO_KR},
   {0x00D7B0, 0x00D7FF, "NotoSansKR-Regular.ttf", CFontTable::ALIGN::LTOR, "Hangul Jamo Extended-B", CFontTable::ADDON::RESOURCE_LANGUAGE_KO_KR},
   //{0x00D800, 0x00DB7F, "", CFontTable::ALIGN::LTOR, "High Surrogates", CFontTable::ADDON::NOT_USED},
   //{0x00DB80, 0x00DBFF, "", CFontTable::ALIGN::LTOR, "Private Use High Surrogates", CFontTable::ADDON::NOT_USED},
   //{0x00DC00, 0x00DFFF, "", CFontTable::ALIGN::LTOR, "Low Surrogates", CFontTable::ADDON::NOT_USED},
   //{0x00E000, 0x00F8FF, "", CFontTable::ALIGN::LTOR, "Private Use Area", CFontTable::ADDON::NOT_USED},
   {0x00F900, 0x00FAFF, "NotoSansJP-Regular.ttf", CFontTable::ALIGN::LTOR, "CJK Compatibility Ideographics", CFontTable::ADDON::RESOURCE_LANGUAGE_JA_JP},
   {0x00FB00, 0x00FB4F, "NotoSans-Regular.ttf", CFontTable::ALIGN::LTOR, "Alphabetic Presentation Forms", CFontTable::ADDON::IN_KODI},
   {0x00FB50, 0x00FDFF, "NotoSansArabic-Regular.ttf", CFontTable::ALIGN::RTOL, "Arabic Presentation Forms", CFontTable::ADDON::RESOURCE_LANGUAGE_AR_SA},
   //{0x00FE00, 0x00FE0F, "", CFontTable::ALIGN::LTOR, "Variation Selectors", CFontTable::ADDON::NOT_USED},
   {0x00FE10, 0x00FE1F, "NotoSansSC-Regular.ttf", CFontTable::ALIGN::LTOR, "Vertical Forms", CFontTable::ADDON::RESOURCE_LANGUAGE_ZH_CN},
   {0x00FE20, 0x00FE2F, "NotoSans-Regular.ttf", CFontTable::ALIGN::LTOR, "Combining Half Masks", CFontTable::ADDON::IN_KODI},
   {0x00FE30, 0x00FE4F, "NotoSansSC-Regular.ttf", CFontTable::ALIGN::LTOR, "CJK Compatibility Forms", CFontTable::ADDON::RESOURCE_LANGUAGE_ZH_CN},
   {0x00FE50, 0x00FE6F, "NotoSansSC-Regular.ttf", CFontTable::ALIGN::LTOR, "Small Form Variants", CFontTable::ADDON::RESOURCE_LANGUAGE_ZH_CN},
   {0x00FE70, 0x00FEFF, "NotoSansArabic-Regular.ttf", CFontTable::ALIGN::RTOL, "Arabic Presentation Forms-B", CFontTable::ADDON::RESOURCE_LANGUAGE_AR_SA},
   {0x00FF00, 0x00FFEF, "NotoSansSC-Regular.ttf", CFontTable::ALIGN::LTOR, "Halfwidth and Fullwidth Forms", CFontTable::ADDON::RESOURCE_LANGUAGE_ZH_CN},
   {0x00FFF0, 0x00FFFF, "NotoSans-Regular.ttf", CFontTable::ALIGN::LTOR, "Specials", CFontTable::ADDON::IN_KODI},
   {0x010000, 0x01007F, "NotoSansLinearB-Regular.ttf", CFontTable::ALIGN::LTOR, "Linear B Syllable", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x010080, 0x0100FF, "NotoSansLinearB-Regular.ttf", CFontTable::ALIGN::LTOR, "Linear B Ideograms", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x010100, 0x01013F, "NotoSansLinearB-Regular.ttf", CFontTable::ALIGN::LTOR, "Aegean Numbers", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x010140, 0x01018F, "NotoSansSymbols2-Regular.ttf", CFontTable::ALIGN::LTOR, "Ancient Greek Numbers", CFontTable::ADDON::IN_KODI},
   {0x010190, 0x0101CF, "NotoSansSymbols2-Regular.ttf", CFontTable::ALIGN::LTOR, "Ancient Symbols", CFontTable::ADDON::IN_KODI},
   {0x0101D0, 0x0101FF, "NotoSansSymbols2-Regular.ttf", CFontTable::ALIGN::LTOR, "Phaistos Disc", CFontTable::ADDON::IN_KODI},
   {0x010280, 0x01029F, "NotoSansLycian-Regular.ttf", CFontTable::ALIGN::LTOR, "Lycian", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x0102A0, 0x0102DF, "NotoSansCarian-Regular.ttf", CFontTable::ALIGN::LTOR, "Carian", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x0102E0, 0x0102FF, "NotoSansCoptic-Regular.ttf", CFontTable::ALIGN::LTOR, "Coptic Epact Numbers", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x010300, 0x01032F, "NotoSansOldItalic-Regular.ttf", CFontTable::ALIGN::LTOR, "Old Italic", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x010330, 0x01034F, "NotoSansGothic-Regular.ttf", CFontTable::ALIGN::LTOR, "Gothic", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x010350, 0x01037F, "NotoSansOldPermic-Regular.ttf", CFontTable::ALIGN::LTOR, "Old Permic", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x010380, 0x01039F, "NotoSansUgaritic-Regular.ttf", CFontTable::ALIGN::LTOR, "Ugaritic", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x0103A0, 0x0103DF, "NotoSansOldPersian-Regular.ttf", CFontTable::ALIGN::LTOR, "Old Persian", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x010400, 0x01044F, "NotoSansDeseret-Regular.ttf", CFontTable::ALIGN::LTOR, "Deseret", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x010450, 0x01047F, "NotoSansShavian-Regular.ttf", CFontTable::ALIGN::LTOR, "Shavian", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x010480, 0x0104AF, "NotoSansOsmanya-Regular.ttf", CFontTable::ALIGN::LTOR, "Osmanya", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x0104B0, 0x0104FF, "NotoSansOsage-Regular.ttf", CFontTable::ALIGN::LTOR, "Osage", CFontTable::ADDON::RESOURCE_FONT_LIMITED},
   {0x010500, 0x01052F, "NotoSansElbasan-Regular.ttf", CFontTable::ALIGN::LTOR, "Elbasan", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x010530, 0x01056F, "NotoSansCaucasianAlbanian-Regular.ttf", CFontTable::ALIGN::LTOR, "Caucasian Albanian", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   //{0x010570, 0x0105BF, "", CFontTable::ALIGN::LTOR, "Vithkuqi", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x010600, 0x01077F, "NotoSansLinearA-Regular.ttf", CFontTable::ALIGN::LTOR, "Linear A", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x010800, 0x01083F, "NotoSansCypriot-Regular.ttf", CFontTable::ALIGN::RTOL, "Cypriot Syllable", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x010840, 0x01085F, "NotoSansImperialAramaic-Regular.ttf", CFontTable::ALIGN::RTOL, "Imperial Aramaic", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x010860, 0x01087F, "NotoSansPalmyrene-Regular.ttf", CFontTable::ALIGN::RTOL, "Palmyrene", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x010880, 0x0108AF, "NotoSansNabataean-Regular.ttf", CFontTable::ALIGN::RTOL, "Nabataean", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x0108E0, 0x0108FF, "NotoSansHatran-Regular.ttf", CFontTable::ALIGN::RTOL, "Hatran", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x010900, 0x01091F, "NotoSansPhoenician-Regular.ttf", CFontTable::ALIGN::RTOL, "Phoenician", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x010920, 0x01093F, "NotoSansLydian-Regular.ttf", CFontTable::ALIGN::RTOL, "Lydian", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x010980, 0x01099F, "NotoSansMeroitic-Regular.ttf", CFontTable::ALIGN::RTOL, "Meroitic Hieroglyphic", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x0109A0, 0x0109FF, "NotoSansMeroitic-Regular.ttf", CFontTable::ALIGN::RTOL, "Meroitic Cursive", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x010A00, 0x010A5F, "NotoSansKharoshthi-Regular.ttf", CFontTable::ALIGN::RTOL, "Kharoshthi", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x010A60, 0x010A7F, "NotoSansOldSouthArabian-Regular.ttf", CFontTable::ALIGN::RTOL, "Old South Arabian", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x010A80, 0x010A9F, "NotoSansOldNorthArabian-Regular.ttf", CFontTable::ALIGN::RTOL, "Old North Arabian", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x010AC0, 0x010AFF, "NotoSansManichaean-Regular.ttf", CFontTable::ALIGN::RTOL, "Manichaean", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x010B00, 0x010B3F, "NotoSansAvestan-Regular.ttf", CFontTable::ALIGN::RTOL, "Avestan", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x010B40, 0x010B5F, "NotoSansInscriptionalParthian-Regular.ttf", CFontTable::ALIGN::RTOL, "Inscriptional Parthian", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x010B60, 0x010B7F, "NotoSansInscriptionalPahlavi-Regular.ttf", CFontTable::ALIGN::RTOL, "Inscriptional Pahlavi", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x010B80, 0x010BAF, "NotoSansPsalterPahlavi-Regular.ttf", CFontTable::ALIGN::RTOL, "Psalter Pahlavi", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x010C00, 0x010C4F, "NotoSansOldTurkic-Regular.ttf", CFontTable::ALIGN::RTOL, "Old Turkic", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x010C80, 0x010CFF, "NotoSansOldHungarian-Regular.ttf", CFontTable::ALIGN::RTOL, "Old Hungarian", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x010D00, 0x010D3F, "NotoSansHanifiRohingya-Regular.ttf", CFontTable::ALIGN::RTOL, "Hanifi Rohingya", CFontTable::ADDON::RESOURCE_FONT_LIMITED},
   {0x010E60, 0x010E7F, "NotoSansSymbols2-Regular.ttf", CFontTable::ALIGN::LTOR, "Rumi Numeral Symbols", CFontTable::ADDON::IN_KODI},
   {0x010E80, 0x010EBF, "NotoSerifYezidi-Regular.ttf", CFontTable::ALIGN::LTOR, "Yezidi", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x010F00, 0x010F2F, "NotoSansOldSogdian-Regular.ttf", CFontTable::ALIGN::RTOL, "Old Sogdian", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x010F30, 0x010F6F, "NotoSansSogdian-Regular.ttf", CFontTable::ALIGN::RTOL, "Sogdian", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   //{0x010FB0, 0x010FDF, "", CFontTable::ALIGN::LTOR, "Chorasmian", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x010FE0, 0x010FFF, "NotoSansElymaic-Regular.ttf", CFontTable::ALIGN::RTOL, "Elymaic", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x011000, 0x01107F, "NotoSansBrahmi-Regular.ttf", CFontTable::ALIGN::LTOR, "Brahmi", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x011080, 0x0110CF, "NotoSansKaithi-Regular.ttf", CFontTable::ALIGN::LTOR, "Kaithi", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x0110D0, 0x0110FF, "NotoSansSoraSompeng-Regular.ttf", CFontTable::ALIGN::LTOR, "Sora Sompeng", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x011100, 0x01114F, "NotoSansChakma-Regular.ttf", CFontTable::ALIGN::LTOR, "Chakma", CFontTable::ADDON::RESOURCE_FONT_LIMITED},
   {0x011150, 0x01117F, "NotoSansMahajani-Regular.ttf", CFontTable::ALIGN::LTOR, "Mahajani", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x011180, 0x0111DF, "NotoSansSharada-Regular.ttf", CFontTable::ALIGN::LTOR, "Sharada", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x0111E0, 0x0111FF, "NotoSansSinhala-Regular.ttf", CFontTable::ALIGN::LTOR, "Sinhala Archaic Numbers", CFontTable::ADDON::IN_KODI},
   {0x011200, 0x01124F, "NotoSansKhojki-Regular.ttf", CFontTable::ALIGN::LTOR, "Khojki", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x011280, 0x0112AF, "NotoSansMultani-Regular.ttf", CFontTable::ALIGN::LTOR, "Multani", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x0112B0, 0x0112FF, "NotoSansKhudawadi-Regular.ttf", CFontTable::ALIGN::LTOR, "Khudawadi", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x011300, 0x01137F, "NotoSansGrantha-Regular.ttf", CFontTable::ALIGN::LTOR, "Grantha", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x011400, 0x01147F, "NotoSansNewa-Regular.ttf", CFontTable::ALIGN::LTOR, "Newa", CFontTable::ADDON::RESOURCE_FONT_LIMITED},
   {0x011480, 0x0114DF, "NotoSansTirhuta-Regular.ttf", CFontTable::ALIGN::LTOR, "Tirhuta", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x011580, 0x0115FF, "NotoSansSiddham-Regular.ttf", CFontTable::ALIGN::LTOR, "Siddham", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x011600, 0x01165F, "NotoSansModi-Regular.ttf", CFontTable::ALIGN::LTOR, "Modi", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x011660, 0x01167F, "NotoSansMongolian-Regular.ttf", CFontTable::ALIGN::LTOR, "Mongolian Supplement", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x011680, 0x0116CF, "NotoSansTakri-Regular.ttf", CFontTable::ALIGN::LTOR, "Takri", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x0117D0, 0x01173F, "NotoSerifAhom-Regular.ttf", CFontTable::ALIGN::LTOR, "Ahom", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x011800, 0x01184F, "NotoSerifDogra-Regular.ttf", CFontTable::ALIGN::LTOR, "Dogra", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x0118A0, 0x0118FF, "NotoSansWarangCiti-Regular.ttf", CFontTable::ALIGN::LTOR, "Warang Citi", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   //{0x011900, 0x01195F, "", CFontTable::ALIGN::LTOR, "Dives Akuru", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   //{0x0119A0, 0x0119FF, "", CFontTable::ALIGN::LTOR, "Nandinagari", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x011A00, 0x011A4F, "NotoSansZanabazarSquare-Regular.ttf", CFontTable::ALIGN::LTOR, "Zanabazar Square", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x011A50, 0x011AAF, "NotoSansSoyombo-Regular.ttf", CFontTable::ALIGN::LTOR, "Soyombo", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x011AC0, 0x011AFF, "NotoSansPauCinHau-Regular.ttf", CFontTable::ALIGN::LTOR, "Pau Cin Hau", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x011C00, 0x011C6F, "NotoSansBhaiksuki-Regular.ttf", CFontTable::ALIGN::LTOR, "Bhaiksuki", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x011C70, 0x011CBF, "NotoSansMarchen-Regular.ttf", CFontTable::ALIGN::LTOR, "Marchen", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x011D00, 0x011D5F, "NotoSansMasaramGondi-Regular.ttf", CFontTable::ALIGN::LTOR, "Masaram Gondi", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x011D60, 0x011DAF, "NotoSansGunjalaGondi-Regular.ttf", CFontTable::ALIGN::LTOR, "Gunjala Gondi", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   //{0x011EE0, 0x011EFF, "", CFontTable::ALIGN::LTOR, "Makasar", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x011FB0, 0x011FBF, "NotoSansLisu-Regular.ttf", CFontTable::ALIGN::LTOR, "Lisu Supplement", CFontTable::ADDON::RESOURCE_FONT_LIMITED},
   {0x011FC0, 0x011FFF, "NotoSansTamilSupplement-Regular.ttf", CFontTable::ALIGN::LTOR, "Tamil Supplement", CFontTable::ADDON::RESOURCE_LANGUAGE_TA_IN},
   {0x012000, 0x0123FF, "NotoSansCuneiform-Regular.ttf", CFontTable::ALIGN::LTOR, "Cuneiform", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x012400, 0x01247F, "NotoSansCuneiform-Regular.ttf", CFontTable::ALIGN::LTOR, "Cuneiform Numbers and Punctuation", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x012480, 0x01254F, "NotoSansCuneiform-Regular.ttf", CFontTable::ALIGN::LTOR, "Early Dynastic Cuneiform", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x013000, 0x01342F, "NotoSansEgyptianHieroglyphs-Regular.ttf", CFontTable::ALIGN::LTOR, "Egyptian Hieroglyphs", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   //{0x013430, 0x01343F, "", CFontTable::ALIGN::LTOR, "Egyptian Hieroglyphs Format Controls", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x014400, 0x01467F, "NotoSansAnatolianHieroglyphs-Regular.ttf", CFontTable::ALIGN::LTOR, "Anatolian Hieroglyphs", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x016800, 0x016A3F, "NotoSansBamum-Regular.ttf", CFontTable::ALIGN::LTOR, "Bamum Supplement", CFontTable::ADDON::RESOURCE_FONT_LIMITED},
   {0x016A40, 0x016A6F, "NotoSansMro-Regular.ttf", CFontTable::ALIGN::LTOR, "Mro", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   //{0x016A70, 0x016AC9, "", CFontTable::ALIGN::LTOR, "Tangsa", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x016AD0, 0x016AFF, "NotoSansBassaVah-Regular.ttf", CFontTable::ALIGN::LTOR, "Bassa Vah", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x016B00, 0x016B8F, "NotoSansPahawhHmong-Regular.ttf", CFontTable::ALIGN::LTOR, "Pahawh Hmong", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x016E40, 0x016E9F, "NotoSansMedefaidrin-Regular.ttf", CFontTable::ALIGN::LTOR, "Medefaidrin", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x016F00, 0x016F9F, "NotoSansMiao-Regular.ttf", CFontTable::ALIGN::LTOR, "Miao", CFontTable::ADDON::RESOURCE_FONT_LIMITED},
   {0x016FE0, 0x016FE0, "NotoSerifTangut-Regular.ttf", CFontTable::ALIGN::LTOR, "Ideographic Symbols and Punctuation", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x016FE0, 0x016FFF, "NotoSansNushu-Regular.ttf", CFontTable::ALIGN::LTOR, "Ideographic Symbols and Punctuation", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x017000, 0x0187FF, "NotoSerifTangut-Regular.ttf", CFontTable::ALIGN::LTOR, "Tangut", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x018800, 0x018AFF, "NotoSerifTangut-Regular.ttf", CFontTable::ALIGN::LTOR, "Tangut Components", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   //{0x018B00, 0x018CFF, "", CFontTable::ALIGN::LTOR, "Khitan Small Script", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x018D00, 0x018D8F, "NotoSerifTangut-Regular.ttf", CFontTable::ALIGN::LTOR, "Tangut Supplement", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   //{0x01B000, 0x01B0FF, "", CFontTable::ALIGN::LTOR, "Kana Supplement", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   //{0x01B100, 0x01B12F, "", CFontTable::ALIGN::LTOR, "Kana Extended-A", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   //{0x01B130, 0x01B16F, "", CFontTable::ALIGN::LTOR, "Small Kana Extension", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x01B170, 0x01B2FF, "NotoSansNushu-Regular.ttf", CFontTable::ALIGN::LTOR, "Nushu", CFontTable::ADDON::IN_KODI},
   {0x01BC00, 0x01BC9F, "NotoSansDuployan-Regular.ttf", CFontTable::ALIGN::LTOR, "Duployan", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x01BCA0, 0x01BCAF, "NotoSansDuployan-Regular.ttf", CFontTable::ALIGN::LTOR, "Shorthand Format Controls", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x01BC00, 0x01BC9F, "NotoMusic-Regular.ttf", CFontTable::ALIGN::LTOR, "Byzantine Musical Symbols", CFontTable::ADDON::IN_KODI},
   {0x01D100, 0x01D1FF, "NotoMusic-Regular.ttf", CFontTable::ALIGN::LTOR, "Musical Symbols", CFontTable::ADDON::IN_KODI},
   {0x01D200, 0x01D24F, "NotoMusic-Regular.ttf", CFontTable::ALIGN::LTOR, "Ancient Greek Musical Notation", CFontTable::ADDON::IN_KODI},
   {0x01D2E0, 0x01D2FF, "NotoSansMayanNumerals-Regular.ttf", CFontTable::ALIGN::LTOR, "Mayan Numerals", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x01D300, 0x01D35F, "NotoSansSymbols2-Regular.ttf", CFontTable::ALIGN::LTOR, "Tai Xuan Jing Symbols", CFontTable::ADDON::IN_KODI},
   {0x01D400, 0x01D7FF, "NotoSansMath-Regular.ttf", CFontTable::ALIGN::LTOR, "Mathematical Alphanumeric Symbols", CFontTable::ADDON::IN_KODI},
   {0x01D800, 0x01DAFF, "NotoSansSignWriting-Regular.ttf", CFontTable::ALIGN::LTOR, "Sutton SignWriting", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x01E000, 0x01E02F, "NotoSansGlagolitic-Regular.ttf", CFontTable::ALIGN::LTOR, "Glagolitic Supplement", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x01E100, 0x01E14F, "NotoSerifNyiakengPuachueHmong-Regular.ttf", CFontTable::ALIGN::LTOR, "Nyiakeng Puachue Hmong", CFontTable::ADDON::RESOURCE_FONT_LIMITED},
   //{0x01E290, 0x01E2BF, "", CFontTable::ALIGN::LTOR, "Toto", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x01E2C0, 0x01E2FF, "NotoSansWancho-Regular.ttf", CFontTable::ALIGN::LTOR, "Wancho", CFontTable::ADDON::RESOURCE_FONT_LIMITED},
   {0x01E800, 0x01E8DF, "NotoSansMendeKikakui-Regular.ttf", CFontTable::ALIGN::RTOL, "Mende Kikakui", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x01E900, 0x01E95F, "NotoSansAdlam-Regular.ttf", CFontTable::ALIGN::RTOL, "Adlam", CFontTable::ADDON::RESOURCE_FONT_LIMITED},
   {0x01EC70, 0x01ECBF, "NotoSansIndicSiyaqNumbers-Regular.ttf", CFontTable::ALIGN::LTOR, "Indic Siyaq Numbers", CFontTable::ADDON::RESOURCE_FONT_LIMITED},
   //{0x01ED00, 0x01ED4F, "", CFontTable::ALIGN::LTOR, "Ottoman Siyaq Numbers", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x01EE00, 0x01EEFF, "NotoSansMath-Regular.ttf", CFontTable::ALIGN::LTOR, "Arabic Mathematical Alphabetic Symbols", CFontTable::ADDON::IN_KODI},
   {0x01F000, 0x01F02F, "NotoSansSymbols2-Regular.ttf", CFontTable::ALIGN::LTOR, "Mahjong Tiles", CFontTable::ADDON::IN_KODI},
   //{0x010F70, 0x010FAF, "", CFontTable::ALIGN::LTOR, "Old Uyghur", CFontTable::ADDON::RESOURCE_FONT_EXCLUDED},
   {0x01F0A0, 0x01F0FF, "NotoSansSymbols2-Regular.ttf", CFontTable::ALIGN::LTOR, "Playing Cards", CFontTable::ADDON::IN_KODI},
   {0x01F100, 0x01F1FF, "NotoSansSymbols-Regular.ttf", CFontTable::ALIGN::LTOR, "Enclosed Alphanumeric Supplement", CFontTable::ADDON::IN_KODI},
   {0x01F200, 0x01F2FF, "NotoSansSC-Regular.ttf", CFontTable::ALIGN::LTOR, "Enclosed Ideographic Supplement", CFontTable::ADDON::RESOURCE_LANGUAGE_ZH_CN},
   {0x01F300, 0x01F5FF, "NotoColorEmoji-Regular.ttf", CFontTable::ALIGN::LTOR, "Miscellaneous Symbols and Pictographs", CFontTable::ADDON::RESOURCE_FONT_EMOJI},
   {0x01F600, 0x01F64F, "NotoColorEmoji-Regular.ttf", CFontTable::ALIGN::LTOR, "Emoticons", CFontTable::ADDON::RESOURCE_FONT_EMOJI},
   {0x01F650, 0x01F67F, "NotoSansSymbols2-Regular.ttf", CFontTable::ALIGN::LTOR, "Omamental Dingbats", CFontTable::ADDON::IN_KODI},
   {0x01F680, 0x01F6FF, "NotoColorEmoji-Regular.ttf", CFontTable::ALIGN::LTOR, "Transport and Map Symbols", CFontTable::ADDON::RESOURCE_FONT_EMOJI},
   {0x01F700, 0x01F77F, "NotoSansSymbols-Regular.ttf", CFontTable::ALIGN::LTOR, "Alchemical Symbols", CFontTable::ADDON::IN_KODI},
   {0x01F780, 0x01F7FF, "NotoSansSymbols2-Regular.ttf", CFontTable::ALIGN::LTOR, "Geometric Shapes Extended", CFontTable::ADDON::IN_KODI},
   {0x01F800, 0x01F8FF, "NotoSansSymbols2-Regular.ttf", CFontTable::ALIGN::LTOR, "Supplemental Arrows-C", CFontTable::ADDON::IN_KODI},
   {0x01F900, 0x01F9FF, "NotoColorEmoji-Regular.ttf", CFontTable::ALIGN::LTOR, "Supplemental Symbols and Pictographs", CFontTable::ADDON::RESOURCE_FONT_EMOJI},
   {0x01FA00, 0x01FA6F, "NotoSansSymbols2-Regular.ttf", CFontTable::ALIGN::LTOR, "Chess Symbols", CFontTable::ADDON::IN_KODI},
   {0x01FA70, 0x01FAFF, "NotoSansSymbols2-Regular.ttf", CFontTable::ALIGN::LTOR, "Symbols and Pictographs Extended-A", CFontTable::ADDON::IN_KODI},
   {0x01FB00, 0x01FBFF, "NotoSansSymbols2-Regular.ttf", CFontTable::ALIGN::LTOR, "Symbols for Legacy Computing", CFontTable::ADDON::IN_KODI},
   // About here and below to strange to look, swaps between JP, SC, TC and KR, buah :-(
   //{0x020000, 0x02A6DF, "", CFontTable::ALIGN::LTOR, "CJK Unified Ideographics Extension B", CFontTable::ADDON::IN_KODI},
   //{0x02A700, 0x02B73F, "", CFontTable::ALIGN::LTOR, "CJK Unified Ideographics Extension C", CFontTable::ADDON::IN_KODI},
   //{0x02A740, 0x02B81F, "", CFontTable::ALIGN::LTOR, "CJK Unified Ideographics Extension D", CFontTable::ADDON::IN_KODI},
   //{0x02B820, 0x02CEAF, "", CFontTable::ALIGN::LTOR, "CJK Unified Ideographics Extension E", CFontTable::ADDON::IN_KODI},
   //{0x02CEB0, 0x02EBEF, "", CFontTable::ALIGN::LTOR, "CJK Unified Ideographics Extension F", CFontTable::ADDON::IN_KODI},
   //{0x02F800, 0x02FA1F, "", CFontTable::ALIGN::LTOR, "CJK Compatibility Ideographic Supplement", CFontTable::ADDON::IN_KODI},
   //{0x030000, 0x03134F, "", CFontTable::ALIGN::LTOR, "CJK Unified Ideographics Extension F", CFontTable::ADDON::IN_KODI},
   // Not relates to character for view
   //{0x0E0000, 0x0E007F, "", CFontTable::ALIGN::LTOR, "Tags", CFontTable::ADDON::NOT_USED},
   //{0x0E0100, 0x0E01EF, "", CFontTable::ALIGN::LTOR, "Variation Selectors Supplement", CFontTable::ADDON::NOT_USED},
   //{0x0F0000, 0x0FFFFF, "", CFontTable::ALIGN::LTOR, "Supplementary Private Use Area-A", CFontTable::ADDON::NOT_USED},
   //{0x100000, 0x10FFFF, "", CFontTable::ALIGN::LTOR, "Supplementary Private Use Area-B", CFontTable::ADDON::NOT_USED},
  }};
// clang-format on

class InstallLanguageJob : public CJob
{
public:
  inline InstallLanguageJob(const CFontTable& fontTable, const std::string& addonId)
    : m_fontTable(fontTable), m_addonId(addonId)
  {
  }

  bool DoWork() override
  {
    using namespace ADDON;
    using namespace KODI::MESSAGING;
    using KODI::MESSAGING::HELPERS::DialogResponse;

    std::unique_lock<CCriticalSection> lock(m_installLock);

    if (WasFailed(m_addonId) || CServiceBroker::GetAddonMgr().IsAddonInstalled(m_addonId))
      return false;

    const InstallMethod method =
        static_cast<InstallMethod>(CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(
            CSettings::SETTING_LOCALE_FONTINSTALL));
    if (method == InstallMethod::NEVER)
    {
      m_installFailed.emplace(m_addonId, JOB_CANCEL_TIMEOUT);
      return false;
    }

    std::shared_ptr<IAddon> addon;
    if (!CServiceBroker::GetAddonMgr().FindInstallableById(m_addonId, addon))
    {
      CLog::Log(LOGERROR, "InstallLanguageJob::{}: Failed to get {} from installable repo content",
                __func__, m_addonId);
      return false;
    }

    const std::string heading = g_localizeStrings.Get(2105);
    const std::string message = StringUtils::Format(g_localizeStrings.Get(2106), m_addonId,
                                                    addon->Name(), m_fontTable.m_name);
    if (method == InstallMethod::NO_ASK ||
        HELPERS::ShowYesNoDialogText(heading, message) == DialogResponse::CHOICE_YES)
    {
      const bool ret = CAddonInstaller::GetInstance().InstallOrUpdate(
          m_addonId, BackgroundJob::CHOICE_NO, ModalJob::CHOICE_NO,
          AddonOptPostInstValue::POST_INSTALL_LANGUAGE_RESOURCE_NO_SELECT);
      if (!ret)
      {
        {
          // If failed, prevent future install works
          std::unique_lock<CCriticalSection> lock(m_installFailedLock);
          m_installFailed.emplace(m_addonId, JOB_RETRY_TIMEOUT);
        }

        CLog::Log(LOGERROR, "InstallLanguageJob::{}: Failed to install {}", __func__, m_addonId);
        const std::string message =
            StringUtils::Format(g_localizeStrings.Get(2107), m_addonId, m_fontTable.m_name);
        HELPERS::ShowOKDialogText(257 /*"Error"*/, message);
      }

      return ret;
    }
    else
    {
      // If cancelled, prevent future install until Kodi is restarted and font needed again
      std::unique_lock<CCriticalSection> lock(m_installFailedLock);
      m_installFailed.emplace(m_addonId, JOB_CANCEL_TIMEOUT);
      return false;
    }

    return false;
  }

  static bool WasFailed(const std::string& addonId)
  {
    std::unique_lock<CCriticalSection> lock(m_installFailedLock);

    auto itr = m_installFailed.find(addonId);
    if (itr != m_installFailed.end())
    {
      if (itr->second.GetInitialTimeoutValue() == JOB_CANCEL_TIMEOUT || !itr->second.IsTimePast())
        return true;

      m_installFailed.erase(itr);
    }

    return false;
  }

private:
  enum class InstallMethod
  {
    NEVER = 0,
    NO_ASK,
    ASK_USER,
  };

  static CCriticalSection m_installLock;
  static CCriticalSection m_installFailedLock;
  static std::unordered_map<std::string, XbmcThreads::EndTime<>> m_installFailed;

  const CFontTable& m_fontTable;
  const std::string m_addonId;
};

CCriticalSection InstallLanguageJob::m_installLock;
CCriticalSection InstallLanguageJob::m_installFailedLock;
std::unordered_map<std::string, XbmcThreads::EndTime<>> InstallLanguageJob::m_installFailed;

} /* namespace */

class CFreeTypeLibrary
{
public:
  CFreeTypeLibrary() = default;
  virtual ~CFreeTypeLibrary()
  {
    if (m_library)
      FT_Done_FreeType(m_library);
  }

  FT_Face GetFont(const std::string& filename,
                  float size,
                  float aspect,
                  std::vector<uint8_t>& memoryBuf)
  {
    // don't have it yet - create it
    if (!m_library)
      FT_Init_FreeType(&m_library);
    if (!m_library)
    {
      CLog::LogF(LOGERROR, "Unable to initialize freetype library");
      return nullptr;
    }

    FT_Face face = nullptr;

    // ok, now load the font face
    CURL realFile(CSpecialProtocol::TranslatePath(filename));
    if (realFile.GetFileName().empty())
      return nullptr;

    memoryBuf.clear();
#ifndef TARGET_WINDOWS
    if (!realFile.GetProtocol().empty())
#endif // ! TARGET_WINDOWS
    {
      // load file into memory if it is not on local drive
      // in case of win32: always load file into memory as filename is in UTF-8,
      //                   but freetype expect filename in ANSI encoding
      XFILE::CFile f;
      if (f.LoadFile(realFile, memoryBuf) <= 0)
        return nullptr;

      if (FT_New_Memory_Face(m_library, reinterpret_cast<const FT_Byte*>(memoryBuf.data()),
                             memoryBuf.size(), 0, &face) != 0)
        return nullptr;
    }
#ifndef TARGET_WINDOWS
    else if (FT_New_Face(m_library, realFile.GetFileName().c_str(), 0, &face))
      return nullptr;
#endif // ! TARGET_WINDOWS

    unsigned int ydpi = 72; // 72 points to the inch is the freetype default
    unsigned int xdpi =
        static_cast<unsigned int>(MathUtils::round_int(static_cast<double>(ydpi * aspect)));

    // we set our screen res currently to 96dpi in both directions (windows default)
    // we cache our characters (for rendering speed) so it's probably
    // not a good idea to allow free scaling of fonts - rather, just
    // scaling to pixel ratio on screen perhaps?
    if (FT_Set_Char_Size(face, 0, static_cast<int>(size * 64 + 0.5f), xdpi, ydpi))
    {
      FT_Done_Face(face);
      return nullptr;
    }

    return face;
  };

  FT_Stroker GetStroker()
  {
    if (!m_library)
      return nullptr;

    FT_Stroker stroker;
    if (FT_Stroker_New(m_library, &stroker))
      return nullptr;

    return stroker;
  };

  static void ReleaseFont(FT_Face face)
  {
    assert(face);
    FT_Done_Face(face);
  };

  static void ReleaseStroker(FT_Stroker stroker)
  {
    assert(stroker);
    FT_Stroker_Done(stroker);
  }

private:
  FT_Library m_library{nullptr};
};

XBMC_GLOBAL_REF(CFreeTypeLibrary, g_freeTypeLibrary); // our freetype library
#define g_freeTypeLibrary XBMC_GLOBAL_USE(CFreeTypeLibrary)

CFontData::~CFontData()
{
  Clear();
}

void CFontData::Clear()
{
  if (m_hbFont)
    hb_font_destroy(m_hbFont);
  m_hbFont = nullptr;
  if (m_face)
    g_freeTypeLibrary.ReleaseFont(m_face);
  m_face = nullptr;
  if (m_stroker)
    g_freeTypeLibrary.ReleaseStroker(m_stroker);
  m_stroker = nullptr;
  m_fontFileInMemory.clear();
  m_char.clear();
}

void CFontData::ClearCharacterCache()
{
  m_char.clear();
  m_char.reserve(CHAR_CHUNK);
}

CGUIFontTTF::CGUIFontTTF(const std::string& fontIdent)
  : m_fontIdent(fontIdent),
    m_staticCache(*this),
    m_dynamicCache(*this),
    m_renderSystem(CServiceBroker::GetRenderSystem())
{
}

CGUIFontTTF::~CGUIFontTTF(void)
{
  Clear();
}

void CGUIFontTTF::AddReference()
{
  m_referenceCount++;
}

void CGUIFontTTF::RemoveReference()
{
  // delete this object when it's reference count hits zero
  m_referenceCount--;
  if (!m_referenceCount)
    g_fontManager.FreeFontFile(this);
}

void CGUIFontTTF::ClearCharacterCache()
{
  m_texture.reset();

  DeleteHardwareTexture();

  m_texture = nullptr;
  m_fontMain.ClearCharacterCache();
  for (const auto& data : m_fallbackUsed)
    data.second->ClearCharacterCache();
  memset(m_charquick, 0, sizeof(m_charquick));
  // set the posX and posY so that our texture will be created on first character write.
  m_posX = m_textureWidth;
  m_posY = -static_cast<int>(GetTextureLineHeight());
  m_textureHeight = 0;
}

void CGUIFontTTF::Clear()
{
  m_texture.reset();
  m_texture = nullptr;
  memset(m_charquick, 0, sizeof(m_charquick));
  m_posX = 0;
  m_posY = 0;
  m_nestedBeginCount = 0;

  m_fontMain.Clear();
  for (const auto& data : m_fallbackUsed)
    data.second->Clear();

  m_vertexTrans.clear();
  m_vertex.clear();
}

bool CGUIFontTTF::LoadFont(const std::string& strFileName,
                           float height,
                           float aspect,
                           float lineSpacing,
                           bool border,
                           bool mainFont,
                           CFontData& data)
{
  // we now know that this object is unique - only the GUIFont objects are non-unique, so no need
  // for reference tracking these fonts
  data.m_face = g_freeTypeLibrary.GetFont(strFileName, height, aspect, data.m_fontFileInMemory);
  if (!data.m_face)
    return false;

  data.m_hbFont = hb_ft_font_create_referenced(data.m_face);
  if (!data.m_hbFont)
    return false;

  /*
   the values used are described below

      XBMC coords                                     Freetype coords

                0  _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _  bbox.yMax, ascender
                        A                 \
                       A A                |
                      A   A               |
                      AAAAA  pppp   cellAscender
                      A   A  p   p        |
                      A   A  p   p        |
   m_cellBaseLine  _ _A_ _A_ pppp_ _ _ _ _/_ _ _ _ _  0, base line.
                             p            \
                             p      cellDescender
     m_cellHeight  _ _ _ _ _ p _ _ _ _ _ _/_ _ _ _ _  bbox.yMin, descender

   */
  int cellDescender = std::min<int>(data.m_face->bbox.yMin, data.m_face->descender);
  int cellAscender = std::max<int>(data.m_face->bbox.yMax, data.m_face->ascender);

  if (border)
  {
    /*
     add on the strength of any border - the non-bordered font needs
     aligning with the bordered font by utilising GetTextBaseLine()
     */
    FT_Pos strength = FT_MulFix(data.m_face->units_per_EM, data.m_face->size->metrics.y_scale) / 12;
    if (strength < 128)
      strength = 128;

    cellDescender -= strength;
    cellAscender += strength;

    data.m_stroker = g_freeTypeLibrary.GetStroker();
    if (data.m_stroker)
      FT_Stroker_Set(data.m_stroker, strength, FT_STROKER_LINECAP_ROUND, FT_STROKER_LINEJOIN_ROUND,
                     0);
  }

  // scale to pixel sizing, rounding so that maximal extent is obtained
  float scaler = height / data.m_face->units_per_EM;

  cellDescender =
      MathUtils::round_int(cellDescender * static_cast<double>(scaler) - 0.5); // round down
  cellAscender = MathUtils::round_int(cellAscender * static_cast<double>(scaler) + 0.5); // round up

  data.m_cellBaseLine = cellAscender;
  data.m_cellHeight = cellAscender - cellDescender;
  data.m_mainFont = mainFont;
  return true;
}

bool CGUIFontTTF::Load(
    const std::string& strFilename, float height, float aspect, float lineSpacing, bool border)
{
  // we now know that this object is unique - only the GUIFont objects are non-unique, so no need
  // for reference tracking these fonts

  if (!LoadFont(strFilename, height, aspect, lineSpacing, border, true, m_fontMain))
    return false;

  m_height = height;
  m_aspect = aspect;
  m_lineSpacing = lineSpacing;
  m_border = border;

  m_texture.reset();
  m_texture = nullptr;

  m_textureHeight = 0;
  m_textureWidth = ((m_fontMain.m_cellHeight * CHARS_PER_TEXTURE_LINE) & ~63) + 64;

  m_textureWidth = CTexture::PadPow2(m_textureWidth);

  if (m_textureWidth > m_renderSystem->GetMaxTextureSize())
    m_textureWidth = m_renderSystem->GetMaxTextureSize();
  m_textureScaleX = 1.0f / m_textureWidth;

  // set the posX and posY so that our texture will be created on first character write.
  m_posX = m_textureWidth;
  m_posY = -static_cast<int>(GetTextureLineHeight());

  return true;
}

void CGUIFontTTF::Begin()
{
  if (m_nestedBeginCount == 0 && m_texture && FirstBegin())
  {
    m_vertexTrans.clear();
    m_vertex.clear();
  }
  // Keep track of the nested begin/end calls.
  m_nestedBeginCount++;
}

void CGUIFontTTF::End()
{
  if (m_nestedBeginCount == 0)
    return;

  if (--m_nestedBeginCount > 0)
    return;

  LastEnd();
}

void CGUIFontTTF::DrawTextInternal(CGraphicContext& context,
                                   float x,
                                   float y,
                                   const std::vector<UTILS::COLOR::Color>& colors,
                                   const vecText& text,
                                   uint32_t alignment,
                                   float maxPixelWidth,
                                   bool scrolling)
{
  if (text.empty())
  {
    return;
  }

  // Check first character (by RTOL is them the last) about his standard alignment and change to related language
  // NOTE: "maxPixelWidth" needs set to use as this, without we not know on which position the right side start.
  bool useRTOLChar = false;
  if (maxPixelWidth > 0)
    alignment = GetNeededAlignment(text[text.size() - 1].letter, alignment, useRTOLChar);

  Begin();
  uint32_t rawAlignment = alignment;
  bool dirtyCache(false);
  const bool hardwareClipping = m_renderSystem->ScissorsCanEffectClipping();
  CGUIFontCacheStaticPosition staticPos(x, y);
  CGUIFontCacheDynamicPosition dynamicPos;
  if (hardwareClipping)
  {
    dynamicPos =
        CGUIFontCacheDynamicPosition(context.ScaleFinalXCoord(x, y), context.ScaleFinalYCoord(x, y),
                                     context.ScaleFinalZCoord(x, y));
  }
  CVertexBuffer unusedVertexBuffer;
  CVertexBuffer& vertexBuffer =
      hardwareClipping
          ? m_dynamicCache.Lookup(context, dynamicPos, colors, text, alignment, maxPixelWidth,
                                  scrolling, std::chrono::steady_clock::now(), dirtyCache)
          : unusedVertexBuffer;
  std::shared_ptr<std::vector<SVertex>> tempVertices = std::make_shared<std::vector<SVertex>>();
  std::shared_ptr<std::vector<SVertex>>& vertices =
      hardwareClipping ? tempVertices
                       : static_cast<std::shared_ptr<std::vector<SVertex>>&>(m_staticCache.Lookup(
                             context, staticPos, colors, text, alignment, maxPixelWidth, scrolling,
                             std::chrono::steady_clock::now(), dirtyCache));

  // reserves vertex vector capacity, only the ones that are going to be used
  if (hardwareClipping)
  {
    if (m_vertexTrans.capacity() == 0)
      m_vertexTrans.reserve(MAX_TRANSLATED_VERTEX);
  }
  else
  {
    if (m_vertex.capacity() == 0)
      m_vertex.reserve(VERTEX_PER_GLYPH * MAX_GLYPHS_PER_TEXT_LINE);
  }

  if (dirtyCache)
  {
    const std::vector<Glyph> glyphs = GetHarfBuzzShapedGlyphs(text);
    // save the origin, which is scaled separately
    m_originX = x;
    m_originY = y;

    // cache the ellipses width
    if (!m_ellipseCached)
    {
      m_ellipseCached = true;
      Character* ellipse = GetCharacter(DUMMY_POINT, 0, &m_fontMain);
      if (ellipse)
        m_ellipsesWidth = ellipse->m_advance;
    }

    // Check if we will really need to truncate or justify the text
    if (alignment & XBFONT_TRUNCATED)
    {
      if (maxPixelWidth <= 0.0f || GetTextWidthInternal(text, glyphs) <= maxPixelWidth)
        alignment &= ~XBFONT_TRUNCATED;
    }
    else if (alignment & XBFONT_JUSTIFIED)
    {
      if (maxPixelWidth <= 0.0f)
        alignment &= ~XBFONT_JUSTIFIED;
    }

    // calculate sizing information
    float startX = 0;
    float startY =
        (alignment & XBFONT_CENTER_Y) ? -0.5f * m_fontMain.m_cellHeight : 0; // vertical centering

    if (!useRTOLChar && alignment & (XBFONT_RIGHT | XBFONT_CENTER_X))
    {
      // Get the extent of this line
      float w = GetTextWidthInternal(text, glyphs);

      if (alignment & XBFONT_TRUNCATED && w > maxPixelWidth + 0.5f) // + 0.5f due to rounding issues
        w = maxPixelWidth;

      if (alignment & XBFONT_CENTER_X)
        w *= 0.5f;
      // Offset this line's starting position
      startX -= w;
    }

    float spacePerSpaceCharacter = 0; // for justification effects
    if (alignment & XBFONT_JUSTIFIED)
    {
      // first compute the size of the text to render in both characters and pixels
      unsigned int numSpaces = 0;
      float linePixels = 0;
      for (const auto& glyph : glyphs)
      {
        Character* ch = GetCharacter(text[glyph.m_glyphInfo.cluster], glyph.m_glyphInfo.codepoint,
                                     glyph.m_fontData);
        if (ch)
        {
          if (text[glyph.m_glyphInfo.cluster].letter == static_cast<char32_t>(' '))
            numSpaces += 1;
          linePixels += ch->m_advance;
        }
      }
      if (numSpaces > 0)
        spacePerSpaceCharacter = (maxPixelWidth - linePixels) / numSpaces;
    }

    float cursorX = 0; // current position along the line
    float offsetX = 0;
    float offsetY = 0;

    // Reserve vector space: 4 vertex for each glyph
    tempVertices->reserve(VERTEX_PER_GLYPH * glyphs.size());

    // Collect all the Character info in a first pass, in case any of them
    // are not currently cached and cause the texture to be enlarged, which
    // would invalidate the texture coordinates.
    std::queue<Character> characters;
    if (alignment & XBFONT_TRUNCATED)
      GetCharacter(DUMMY_POINT, 0, &m_fontMain);

    if (!useRTOLChar)
    {
      for (const auto& glyph : glyphs)
      {
        Character* ch = GetCharacter(text[glyph.m_glyphInfo.cluster], glyph.m_glyphInfo.codepoint,
                                     glyph.m_fontData);
        if (!ch)
        {
          Character null = {};
          characters.push(null);
          continue;
        }
        characters.push(*ch);

        if (maxPixelWidth > 0 &&
            cursorX + ((alignment & XBFONT_TRUNCATED) ? ch->m_advance + 3 * m_ellipsesWidth : 0) >
                maxPixelWidth)
          break;
        cursorX += ch->m_advance;
      }

      cursorX = 0;

      for (const auto& glyph : glyphs)
      {
        // If starting text on a new line, determine justification effects
        // Get the current letter in the CStdString
        UTILS::COLOR::Color color = text[glyph.m_glyphInfo.cluster].color;
        if (color >= colors.size())
          color = 0;
        color = colors[color];

        // grab the next character
        Character* ch = &characters.front();
        if (ch->m_glyphAndStyle == 0)
        {
          characters.pop();
          continue;
        }

        if (text[glyph.m_glyphInfo.cluster].letter == static_cast<char32_t>('\t'))
        {
          const float tabwidth = GetTabSpaceLength();
          const float a = cursorX / tabwidth;
          cursorX += tabwidth - ((a - floorf(a)) * tabwidth);
          characters.pop();
          continue;
        }

        if (alignment & XBFONT_TRUNCATED)
        {
          // Check if we will be exceeded the max allowed width
          if (cursorX + ch->m_advance + 3 * m_ellipsesWidth > maxPixelWidth)
          {
            // Yup. Let's draw the ellipses, then bail
            // Perhaps we should really bail to the next line in this case??
            Character* period = GetCharacter(DUMMY_POINT, 0, &m_fontMain);
            if (!period)
              break;

            for (int i = 0; i < 3; i++)
            {
              RenderCharacter(context, startX + cursorX, startY, period, color, !scrolling,
                              *tempVertices);
              cursorX += period->m_advance;
            }
            break;
          }
        }
        else if (maxPixelWidth > 0 && cursorX > maxPixelWidth)
          break; // exceeded max allowed width - stop rendering

        offsetX = static_cast<float>(
            MathUtils::round_int(static_cast<double>(glyph.m_glyphPosition.x_offset) / 64));
        offsetY = static_cast<float>(
            MathUtils::round_int(static_cast<double>(glyph.m_glyphPosition.y_offset) / 64));
        RenderCharacter(context, startX + cursorX + offsetX, startY - offsetY, ch, color,
                        !scrolling, *tempVertices);
        if (alignment & XBFONT_JUSTIFIED)
        {
          if (text[glyph.m_glyphInfo.cluster].letter == static_cast<char32_t>(' '))
            cursorX += ch->m_advance + spacePerSpaceCharacter;
          else
            cursorX += ch->m_advance;
        }
        else
          cursorX += ch->m_advance;
        characters.pop();
      }
    }
    else
    {
      // With RTOL use we needs to go backward about the used string and to have in right
      // character flow and positions.
      auto glyph = glyphs.end();
      while (glyph != glyphs.begin())
      {
        --glyph;
        Character* ch = GetCharacter(text[glyph->m_glyphInfo.cluster], glyph->m_glyphInfo.codepoint,
                                     glyph->m_fontData);
        if (!ch)
        {
          Character null = {};
          characters.push(null);
          continue;
        }
        characters.push(*ch);

        if (maxPixelWidth > 0 &&
            cursorX + ((alignment & XBFONT_TRUNCATED) ? ch->m_advance + 3 * m_ellipsesWidth : 0) >
                maxPixelWidth)
          break;
        cursorX += ch->m_advance;
      }

      cursorX = maxPixelWidth;

      glyph = glyphs.end();
      while (glyph != glyphs.begin())
      {
        --glyph;

        // If starting text on a new line, determine justification effects
        // Get the current letter in the CStdString
        UTILS::COLOR::Color color = text[glyph->m_glyphInfo.cluster].color;
        if (color >= colors.size())
          color = 0;
        color = colors[color];

        // grab the next character
        Character* ch = &characters.front();
        if (ch->m_glyphAndStyle == 0)
        {
          characters.pop();
          continue;
        }

        if (text[glyph->m_glyphInfo.cluster].letter == static_cast<char32_t>('\t'))
        {
          const float tabwidth = GetTabSpaceLength();
          const float a = cursorX / tabwidth;
          cursorX -= ((a - floorf(a)) * tabwidth);
          characters.pop();
          continue;
        }

        if (alignment & XBFONT_TRUNCATED)
        {
          // Check if we will be exceeded the max allowed width
          if (cursorX + ch->m_advance + 3 * m_ellipsesWidth > maxPixelWidth)
          {
            // Yup. Let's draw the ellipses, then bail
            // Perhaps we should really bail to the next line in this case??
            Character* period = GetCharacter(DUMMY_POINT, 0, &m_fontMain);
            if (!period)
              break;

            for (int i = 0; i < 3; i++)
            {
              cursorX -= period->m_advance;
              RenderCharacter(context, startX + cursorX, startY, period, color, !scrolling,
                              *tempVertices);
            }
            break;
          }
        }
        else if (cursorX < 0)
          break; // exceeded max allowed width - stop rendering

        offsetX = static_cast<float>(
            MathUtils::round_int(static_cast<double>(glyph->m_glyphPosition.x_offset -
                                                     glyph->m_glyphPosition.x_advance) /
                                 64));
        offsetY = static_cast<float>(
            MathUtils::round_int(static_cast<double>(glyph->m_glyphPosition.y_offset) / 64));
        RenderCharacter(context, startX + cursorX + offsetX, startY - offsetY, ch, color,
                        !scrolling, *tempVertices);

        cursorX -= ch->m_advance;
        characters.pop();
      }
    }

    if (hardwareClipping)
    {
      CVertexBuffer& vertexBuffer =
          m_dynamicCache.Lookup(context, dynamicPos, colors, text, rawAlignment, maxPixelWidth,
                                scrolling, std::chrono::steady_clock::now(), dirtyCache);
      CVertexBuffer newVertexBuffer = CreateVertexBuffer(*tempVertices);
      vertexBuffer = newVertexBuffer;
      m_vertexTrans.emplace_back(.0f, .0f, .0f, &vertexBuffer, context.GetClipRegion());
    }
    else
    {
      m_staticCache.Lookup(context, staticPos, colors, text, rawAlignment, maxPixelWidth, scrolling,
                           std::chrono::steady_clock::now(), dirtyCache) =
          *static_cast<CGUIFontCacheStaticValue*>(&tempVertices);
      /* Append the new vertices to the set collected since the first Begin() call */
      m_vertex.insert(m_vertex.end(), tempVertices->begin(), tempVertices->end());
    }
  }
  else
  {
    if (hardwareClipping)
      m_vertexTrans.emplace_back(dynamicPos.m_x, dynamicPos.m_y, dynamicPos.m_z, &vertexBuffer,
                                 context.GetClipRegion());
    else
      /* Append the vertices from the cache to the set collected since the first Begin() call */
      m_vertex.insert(m_vertex.end(), vertices->begin(), vertices->end());
  }

  End();
}

float CGUIFontTTF::GetTextWidthInternal(const vecText& text)
{
  const std::vector<Glyph> glyphs = GetHarfBuzzShapedGlyphs(text);
  return GetTextWidthInternal(text, glyphs);
}

// this routine assumes a single line (i.e. it was called from GUITextLayout)
float CGUIFontTTF::GetTextWidthInternal(const vecText& text, const std::vector<Glyph>& glyphs)
{
  float width = 0;
  for (auto it = glyphs.begin(); it != glyphs.end(); it++)
  {
    const character_t& ch = text[(*it).m_glyphInfo.cluster];
    Character* c = GetCharacter(ch, (*it).m_glyphInfo.codepoint, (*it).m_fontData);
    if (c)
    {
      // If last character in line, we want to add render width
      // and not advance distance - this makes sure that italic text isn't
      // choped on the end (as render width is larger than advance then).
      if (std::next(it) == glyphs.end())
        width += std::max(c->m_right - c->m_left + c->m_offsetX, c->m_advance);
      else if (ch.letter == static_cast<decltype(ch.letter)>('\t'))
        width += GetTabSpaceLength();
      else
        width += c->m_advance;
    }
  }

  return width;
}

float CGUIFontTTF::GetCharWidthInternal(const character_t& ch)
{
  Character* c = GetCharacter(ch, 0, nullptr);
  if (c)
  {
    if (ch.letter == static_cast<decltype(ch.letter)>('\t'))
      return GetTabSpaceLength();
    else
      return c->m_advance;
  }

  return 0;
}

float CGUIFontTTF::GetTextHeight(float lineSpacing, int numLines) const
{
  return static_cast<float>(numLines - 1) * GetLineHeight(lineSpacing) + m_fontMain.m_cellHeight;
}

float CGUIFontTTF::GetLineHeight(float lineSpacing) const
{
  if (m_fontMain.m_face)
    return lineSpacing * m_fontMain.m_face->size->metrics.height / 64.0f;
  return 0.0f;
}

unsigned int CGUIFontTTF::GetTextureLineHeight() const
{
  return m_fontMain.m_cellHeight + SPACING_BETWEEN_CHARACTERS_IN_TEXTURE;
}

unsigned int CGUIFontTTF::GetMaxFontHeight() const
{
  return m_maxFontHeight + SPACING_BETWEEN_CHARACTERS_IN_TEXTURE;
}

std::vector<CGUIFontTTF::Glyph> CGUIFontTTF::GetHarfBuzzShapedGlyphs(const vecText& text)
{
  std::vector<Glyph> glyphs;
  if (text.empty())
    return glyphs;

  struct RunInfo
  {
    unsigned int startOffset;
    unsigned int endOffset;
    hb_script_t script;
    hb_glyph_info_t* glyphInfos;
    hb_glyph_position_t* glyphPositions;
    CFontData* font;
    const CFontTable* fontTable{nullptr};
  };

  std::vector<RunInfo> runs;
  unsigned int start = 0;
  unsigned int end = 0;
  hb_unicode_funcs_t* ufuncs = hb_unicode_funcs_get_default();
  hb_script_t scriptNow = HB_SCRIPT_UNKNOWN;
  hb_script_t scriptLast = HB_SCRIPT_UNKNOWN;

  do
  {
    const char32_t wchar = text[end].letter;

    // if below 256 force to use latin ANSI characters
    scriptNow = (wchar < 256) ? HB_SCRIPT_LATIN : hb_unicode_script(ufuncs, wchar);
    if (scriptNow != scriptLast)
    {
      if (end > 0)
      {
        RunInfo info;
        info.startOffset = start;
        info.endOffset = end;
        info.script = scriptLast;
        info.font = FindFallback(text[info.startOffset].letter, info.fontTable);
        runs.emplace_back(info);
        start = end;
      }
      scriptLast = scriptNow;
    }
  } while (++end < text.size());

  RunInfo info;
  info.startOffset = start;
  info.endOffset = text.size();
  info.script = scriptLast;
  info.font = FindFallback(text[info.endOffset - 1].letter, info.fontTable);
  runs.emplace_back(info);

  hb_buffer_t* buffer = hb_buffer_create();
  for (auto& run : runs)
  {
    hb_buffer_reset(buffer);
    hb_buffer_set_direction(buffer, static_cast<hb_direction_t>(HB_DIRECTION_LTR));
    hb_buffer_set_script(buffer, run.script);
    hb_buffer_set_content_type(buffer, HB_BUFFER_CONTENT_TYPE_UNICODE);

    for (unsigned int j = run.startOffset; j < run.endOffset; j++)
    {
      hb_buffer_add(buffer, text[j].letter, j);
    }

    if (run.font)
      hb_shape(run.font->m_hbFont, buffer, nullptr, 0);

    unsigned int glyphCount;
    run.glyphInfos = hb_buffer_get_glyph_infos(buffer, &glyphCount);
    run.glyphPositions = hb_buffer_get_glyph_positions(buffer, &glyphCount);

    for (size_t k = 0; k < glyphCount; k++)
    {
      glyphs.emplace_back(run.glyphInfos[k], run.glyphPositions[k], run.font);
    }
  }
  hb_buffer_destroy(buffer);

  return glyphs;
}

Character* CGUIFontTTF::GetCharacter(const character_t& chr,
                                     FT_UInt glyphIndex,
                                     CFontData* fontData)
{
  const auto letter = chr.letter;

  // ignore linebreaks
  if (letter == static_cast<decltype(chr.letter)>('\r'))
    return nullptr;

  const auto style = chr.style;

  if (!fontData)
  {
    glyphIndex = FT_Get_Char_Index(m_fontMain.m_face, chr.letter);
    if (glyphIndex != 0)
      fontData = &m_fontMain;
    else
    {
      const CFontTable* fontTable;
      fontData = FindFallback(chr.letter, fontTable);
      if (!fontData || (glyphIndex = FT_Get_Char_Index(fontData->m_face, chr.letter)) == 0)
      {
        CLog::LogF(LOGERROR, "Unable to find font about character {:x}, font data found: {}",
                   static_cast<uint32_t>(chr.letter), fontData ? "yes" : "no");
        return nullptr;
      }
    }
  }

  if (!glyphIndex)
    glyphIndex = FT_Get_Char_Index(fontData->m_face, chr.letter);

  // quick access to the most frequently used glyphs
  if (glyphIndex < MAX_GLYPH_IDX && fontData->m_mainFont)
  {
    const uint32_t ch = (style << 12) | glyphIndex; // 2^12 = 4096

    if (ch < LOOKUPTABLE_SIZE && m_charquick[ch])
      return m_charquick[ch];
  }

  // letters are stored based on style and glyph
  const glyph_and_style_t ch = (static_cast<glyph_and_style_t>(style) << 32) | glyphIndex;

  // perform binary search on sorted array by m_glyphAndStyle and
  // if not found obtains position to insert the new m_char to keep sorted
  int low = 0;
  int high = fontData->m_char.size() - 1;
  while (low <= high)
  {
    int mid = (low + high) >> 1;
    if (ch > fontData->m_char[mid].m_glyphAndStyle)
      low = mid + 1;
    else if (ch < fontData->m_char[mid].m_glyphAndStyle)
      high = mid - 1;
    else
      return &fontData->m_char[mid];
  }
  // if we get to here, then low is where we should insert the new character

  int startIndex = low;

  // increase the size of the buffer if we need it
  if (fontData->m_char.size() == fontData->m_char.capacity())
  {
    fontData->m_char.reserve(fontData->m_char.capacity() + CHAR_CHUNK);
    startIndex = 0;
  }

  // render the character to our texture
  // must End() as we can't render text to our texture during a Begin(), End() block
  unsigned int nestedBeginCount = m_nestedBeginCount;
  m_nestedBeginCount = 1;

  if (nestedBeginCount)
    End();

  fontData->m_char.emplace(fontData->m_char.begin() + low);
  if (!CacheCharacter(glyphIndex, style, fontData->m_char.data() + low, fontData))
  { // unable to cache character - try clearing them all out and starting over
    CLog::LogF(LOGDEBUG, "Unable to cache character. Clearing character cache of {} characters",
               fontData->m_char.size());
    ClearCharacterCache();
    low = 0;
    startIndex = 0;
    fontData->m_char.emplace(fontData->m_char.begin());
    if (!CacheCharacter(glyphIndex, style, fontData->m_char.data(), fontData))
    {
      CLog::LogF(LOGERROR, "Unable to cache character (out of memory?)");
      if (nestedBeginCount)
        Begin();
      m_nestedBeginCount = nestedBeginCount;
      return nullptr;
    }
  }

  if (nestedBeginCount)
    Begin();
  m_nestedBeginCount = nestedBeginCount;

  if (fontData->m_mainFont)
  {
    // update the lookup table with only the m_char addresses that have changed
    for (size_t i = startIndex; i < fontData->m_char.size(); ++i)
    {
      if (fontData->m_char[i].m_glyphIndex < MAX_GLYPH_IDX)
      {
        // >> 32 is style (0-6), then 32 - 12 (>> 20) is equivalent to style * 4096
        const uint32_t ch = ((fontData->m_char[i].m_glyphAndStyle & 0xFFFFFFFF00000000) >> 20) |
                            fontData->m_char[i].m_glyphIndex;

        if (ch < LOOKUPTABLE_SIZE)
          m_charquick[ch] = fontData->m_char.data() + i;
      }
    }
  }

  return fontData->m_char.data() + low;
}

CFontData* CGUIFontTTF::FindFallback(char32_t letter, const CFontTable*& fontTable)
{
  FT_UInt glyphIndex = FT_Get_Char_Index(m_fontMain.m_face, letter);
  if (glyphIndex)
    return &m_fontMain;

  // Check font already known and active
  auto it = std::find_if(m_fallbackUsed.begin(), m_fallbackUsed.end(),
                         [letter](std::pair<CFontTable, std::shared_ptr<CFontData>>& data) {
                           return (letter >= data.first.m_unicode_code_begin &&
                                   letter <= data.first.m_unicode_code_end);
                         });
  if (it != m_fallbackUsed.end())
  {
    fontTable = &it->first;
    return it->second.get();
  }

  // Check font by Kodi
  auto it2 =
      std::find_if(fontFallbacks.begin(), fontFallbacks.end(), [letter](const CFontTable& data) {
        return (letter >= data.m_unicode_code_begin && letter <= data.m_unicode_code_end);
      });
  if (it2 == fontFallbacks.end())
  {
    fontTable = nullptr;
    return &m_fontMain;
  }

  // Check now font already used in on another unicode range
  auto it3 = std::find_if(m_fallbackUsed.begin(), m_fallbackUsed.end(),
                          [it2](std::pair<CFontTable, std::shared_ptr<CFontData>>& data) {
                            return (it2->m_filename == data.first.m_filename);
                          });
  if (it3 != m_fallbackUsed.end())
  {
    m_fallbackUsed.emplace_back(it3->first, it3->second);
    fontTable = &it3->first;
    return it3->second.get();
  }

  // If supported and before not used create it here
  std::string strPath;
  if (it2->m_addon == CFontTable::ADDON::IN_KODI)
  {
    strPath = StringUtils::Format("special://xbmc/media/Fonts/{}", it2->m_filename);
  }
  else
  {
    if (!CServiceBroker::IsAddonInterfaceUp())
      return &m_fontMain;

    auto it4 = std::find_if(fontAddons.begin(), fontAddons.end(),
                            [it2](std::pair<CFontTable::ADDON, const char*> data) {
                              return (it2->m_addon == data.first);
                            });
    if (it4 == fontAddons.end())
    {
      fontTable = nullptr;
      return &m_fontMain;
    }

    using namespace ADDON;

    std::shared_ptr<CAddonInfo> addonInfo = CServiceBroker::GetAddonMgr().GetAddonInfo(it4->second);
    if (addonInfo)
    {
      strPath = CSpecialProtocol::TranslatePath(
          URIUtils::AddFileToFolder(addonInfo->Path(), "resources/fonts/", it2->m_filename));
    }
    else
    {
      // Check it was already tried to install, if yes prevent any further tries
      if (!InstallLanguageJob::WasFailed(it4->second))
        CServiceBroker::GetJobManager()->AddJob(new InstallLanguageJob(*it2, it4->second), nullptr);

      // Return as fail with main, update becomes done on job
      return &m_fontMain;
    }
  }

  std::shared_ptr<CFontData> data = std::make_shared<CFontData>();
  if (!LoadFont(strPath, m_height, m_aspect, m_lineSpacing, m_border, false, *data.get()))
  {
    fontTable = nullptr;
    return &m_fontMain;
  }

  fontTable = &*it2;
  m_fallbackUsed.emplace_back(*it2, data);

  return data.get();
}

uint32_t CGUIFontTTF::GetNeededAlignment(char32_t firstLetter,
                                         uint32_t alignmentDefault,
                                         bool& useRTOLChar)
{
  // Check character is an standard left aligned about typical UTF codes to prevent unneeded works.
  // Also not use if already defined as centered character.
  if (firstLetter < 0x00052F || alignmentDefault & XBFONT_CENTER_X ||
      alignmentDefault & XBFONT_JUSTIFIED)
    return alignmentDefault;

  // Check font by Kodi and make alignment to opposite.
  auto it = std::find_if(
      fontFallbacks.begin(), fontFallbacks.end(), [firstLetter](const CFontTable& data) {
        return (firstLetter >= data.m_unicode_code_begin && firstLetter <= data.m_unicode_code_end);
      });
  if (it != fontFallbacks.end())
  {
    if (it->alignment == CFontTable::ALIGN::RTOL)
    {
      useRTOLChar = true;

      if (alignmentDefault & XBFONT_RIGHT)
      {
        alignmentDefault &= ~XBFONT_RIGHT;
      }
      else
      {
        alignmentDefault |= XBFONT_RIGHT;
      }
    }
  }

  return alignmentDefault;
}

bool CGUIFontTTF::CacheCharacter(FT_UInt glyphIndex,
                                 uint32_t style,
                                 Character* ch,
                                 CFontData* fontData)
{
  FT_Glyph glyph = nullptr;
  if (FT_Load_Glyph(fontData->m_face, glyphIndex, FT_LOAD_TARGET_LIGHT))
  {
    CLog::LogF(LOGDEBUG, "Failed to load glyph {:x}", glyphIndex);
    return false;
  }

  // make bold if applicable
  if (style & FONT_STYLE_BOLD)
    SetGlyphStrength(fontData->m_face->glyph, fontData->m_face, GLYPH_STRENGTH_BOLD);
  // and italics if applicable
  if (style & FONT_STYLE_ITALICS)
    ObliqueGlyph(fontData->m_face->glyph);
  // and light if applicable
  if (style & FONT_STYLE_LIGHT)
    SetGlyphStrength(fontData->m_face->glyph, fontData->m_face, GLYPH_STRENGTH_LIGHT);
  // grab the glyph
  if (FT_Get_Glyph(fontData->m_face->glyph, &glyph))
  {
    CLog::LogF(LOGDEBUG, "Failed to get glyph {:x}", glyphIndex);
    return false;
  }
  if (fontData->m_stroker)
    FT_Glyph_StrokeBorder(&glyph, fontData->m_stroker, 0, 1);
  // render the glyph
  if (FT_Glyph_To_Bitmap(&glyph, FT_RENDER_MODE_NORMAL, nullptr, 1))
  {
    CLog::LogF(LOGDEBUG, "Failed to render glyph {:x} to a bitmap", glyphIndex);
    return false;
  }

  FT_BitmapGlyph bitGlyph = (FT_BitmapGlyph)glyph;
  FT_Bitmap bitmap = bitGlyph->bitmap;
  bool isEmptyGlyph = (bitmap.width == 0 || bitmap.rows == 0);

  if (!isEmptyGlyph)
  {
    if (bitGlyph->left < 0)
      m_posX += -bitGlyph->left;

    // check we have enough room for the character.
    // cast-fest is here to avoid warnings due to freeetype version differences (signedness of width).
    if (static_cast<int>(m_posX + bitGlyph->left + bitmap.width +
                         SPACING_BETWEEN_CHARACTERS_IN_TEXTURE) > static_cast<int>(m_textureWidth))
    { // no space - gotta drop to the next line (which means creating a new texture and copying it across)
      m_posX = 1;
      m_posY += GetTextureLineHeight();
      if (bitGlyph->left < 0)
        m_posX += -bitGlyph->left;

      if (m_posY + GetTextureLineHeight() >= m_textureHeight)
      {
        // create the new larger texture
        unsigned int newHeight = m_posY + GetTextureLineHeight();
        // check for max height
        if (newHeight > m_renderSystem->GetMaxTextureSize())
        {
          CLog::LogF(LOGDEBUG, "New cache texture is too large ({} > {} pixels long)", newHeight,
                     m_renderSystem->GetMaxTextureSize());
          FT_Done_Glyph(glyph);
          return false;
        }

        std::unique_ptr<CTexture> newTexture = ReallocTexture(newHeight);
        if (!newTexture)
        {
          FT_Done_Glyph(glyph);
          CLog::LogF(LOGDEBUG, "Failed to allocate new texture of height {}", newHeight);
          return false;
        }
        m_texture = std::move(newTexture);
      }
      m_posY = GetMaxFontHeight();
    }

    if (!m_texture)
    {
      FT_Done_Glyph(glyph);
      CLog::LogF(LOGDEBUG, "no texture to cache character to");
      return false;
    }
  }

  // set the character in our table
  ch->m_glyphAndStyle = (static_cast<glyph_and_style_t>(style) << 32) | glyphIndex;
  ch->m_glyphIndex = glyphIndex;
  ch->m_offsetX = static_cast<short>(bitGlyph->left);
  ch->m_offsetY = static_cast<short>(m_fontMain.m_cellBaseLine - bitGlyph->top);
  ch->m_left = isEmptyGlyph ? 0.0f : (static_cast<float>(m_posX));
  ch->m_top = isEmptyGlyph ? 0.0f : (static_cast<float>(m_posY));
  ch->m_right = ch->m_left + bitmap.width;
  ch->m_bottom = ch->m_top + bitmap.rows;
  ch->m_advance = static_cast<float>(
      MathUtils::round_int(static_cast<double>(fontData->m_face->glyph->advance.x) / 64));

  // we need only render if we actually have some pixels
  if (!isEmptyGlyph)
  {
    // ensure our rect will stay inside the texture (it *should* but we need to be certain)
    unsigned int x1 = std::max(m_posX, 0);
    unsigned int y1 = std::max(m_posY, 0);
    unsigned int x2 = std::min(x1 + bitmap.width, m_textureWidth);
    unsigned int y2 = std::min(y1 + bitmap.rows, m_textureHeight);
    m_maxFontHeight = std::max(m_maxFontHeight, y2);
    CopyCharToTexture(bitGlyph, x1, y1, x2, y2);

    m_posX += SPACING_BETWEEN_CHARACTERS_IN_TEXTURE +
              static_cast<unsigned short>(ch->m_right - ch->m_left);
  }

  // free the glyph
  FT_Done_Glyph(glyph);

  return true;
}

void CGUIFontTTF::RenderCharacter(CGraphicContext& context,
                                  float posX,
                                  float posY,
                                  const Character* ch,
                                  UTILS::COLOR::Color color,
                                  bool roundX,
                                  std::vector<SVertex>& vertices)
{
  // actual image width isn't same as the character width as that is
  // just baseline width and height should include the descent
  const float width = ch->m_right - ch->m_left;
  const float height = ch->m_bottom - ch->m_top;

  // return early if nothing to render
  if (width == 0 || height == 0)
    return;

  // posX and posY are relative to our origin, and the textcell is offset
  // from our (posX, posY).  Plus, these are unscaled quantities compared to the underlying GUI resolution
  CRect vertex((posX + ch->m_offsetX) * context.GetGUIScaleX(),
               (posY + ch->m_offsetY) * context.GetGUIScaleY(),
               (posX + ch->m_offsetX + width) * context.GetGUIScaleX(),
               (posY + ch->m_offsetY + height) * context.GetGUIScaleY());
  vertex += CPoint(m_originX, m_originY);
  CRect texture(ch->m_left, ch->m_top, ch->m_right, ch->m_bottom);
  if (!m_renderSystem->ScissorsCanEffectClipping())
    context.ClipRect(vertex, texture);

  // transform our positions - note, no scaling due to GUI calibration/resolution occurs
  float x[VERTEX_PER_GLYPH] = {context.ScaleFinalXCoord(vertex.x1, vertex.y1),
                               context.ScaleFinalXCoord(vertex.x2, vertex.y1),
                               context.ScaleFinalXCoord(vertex.x2, vertex.y2),
                               context.ScaleFinalXCoord(vertex.x1, vertex.y2)};

  if (roundX)
  {
    // We only round the "left" side of the character, and then use the direction of rounding to
    // move the "right" side of the character.  This ensures that a constant width is kept when rendering
    // the same letter at the same size at different places of the screen, avoiding the problem
    // of the "left" side rounding one way while the "right" side rounds the other way, thus getting
    // altering the width of thin characters substantially.  This only really works for positive
    // coordinates (due to the direction of truncation for negatives) but this is the only case that
    // really interests us anyway.
    float rx0 = static_cast<float>(MathUtils::round_int(static_cast<double>(x[0])));
    float rx3 = static_cast<float>(MathUtils::round_int(static_cast<double>(x[3])));
    x[1] = static_cast<float>(MathUtils::truncate_int(static_cast<double>(x[1])));
    x[2] = static_cast<float>(MathUtils::truncate_int(static_cast<double>(x[2])));
    if (x[0] > 0.0f && rx0 > x[0])
      x[1] += 1;
    else if (x[0] < 0.0f && rx0 < x[0])
      x[1] -= 1;
    if (x[3] > 0.0f && rx3 > x[3])
      x[2] += 1;
    else if (x[3] < 0.0f && rx3 < x[3])
      x[2] -= 1;
    x[0] = rx0;
    x[3] = rx3;
  }

  const float y[VERTEX_PER_GLYPH] = {
      static_cast<float>(MathUtils::round_int(
          static_cast<double>(context.ScaleFinalYCoord(vertex.x1, vertex.y1)))),
      static_cast<float>(MathUtils::round_int(
          static_cast<double>(context.ScaleFinalYCoord(vertex.x2, vertex.y1)))),
      static_cast<float>(MathUtils::round_int(
          static_cast<double>(context.ScaleFinalYCoord(vertex.x2, vertex.y2)))),
      static_cast<float>(MathUtils::round_int(
          static_cast<double>(context.ScaleFinalYCoord(vertex.x1, vertex.y2))))};

  const float z[VERTEX_PER_GLYPH] = {
      static_cast<float>(MathUtils::round_int(
          static_cast<double>(context.ScaleFinalZCoord(vertex.x1, vertex.y1)))),
      static_cast<float>(MathUtils::round_int(
          static_cast<double>(context.ScaleFinalZCoord(vertex.x2, vertex.y1)))),
      static_cast<float>(MathUtils::round_int(
          static_cast<double>(context.ScaleFinalZCoord(vertex.x2, vertex.y2)))),
      static_cast<float>(MathUtils::round_int(
          static_cast<double>(context.ScaleFinalZCoord(vertex.x1, vertex.y2))))};

  // tex coords converted to 0..1 range
  const float tl = texture.x1 * m_textureScaleX;
  const float tr = texture.x2 * m_textureScaleX;
  const float tt = texture.y1 * m_textureScaleY;
  const float tb = texture.y2 * m_textureScaleY;

  vertices.resize(vertices.size() + VERTEX_PER_GLYPH);
  SVertex* v = &vertices[vertices.size() - VERTEX_PER_GLYPH];
  m_color = color;

#if !defined(HAS_DX)
  uint8_t r = KODI::UTILS::GL::GetChannelFromARGB(KODI::UTILS::GL::ColorChannel::R, color);
  uint8_t g = KODI::UTILS::GL::GetChannelFromARGB(KODI::UTILS::GL::ColorChannel::G, color);
  uint8_t b = KODI::UTILS::GL::GetChannelFromARGB(KODI::UTILS::GL::ColorChannel::B, color);
  uint8_t a = KODI::UTILS::GL::GetChannelFromARGB(KODI::UTILS::GL::ColorChannel::A, color);
#endif

  for (int i = 0; i < VERTEX_PER_GLYPH; i++)
  {
#ifdef HAS_DX
    CD3DHelper::XMStoreColor(&v[i].col, color);
#else
    v[i].r = r;
    v[i].g = g;
    v[i].b = b;
    v[i].a = a;
#endif
  }

#if defined(HAS_DX)
  for (int i = 0; i < VERTEX_PER_GLYPH; i++)
  {
    v[i].x = x[i];
    v[i].y = y[i];
    v[i].z = z[i];
  }

  v[0].u = tl;
  v[0].v = tt;

  v[1].u = tr;
  v[1].v = tt;

  v[2].u = tr;
  v[2].v = tb;

  v[3].u = tl;
  v[3].v = tb;
#else
  // GL / GLES uses triangle strips, not quads, so have to rearrange the vertex order
  v[0].u = tl;
  v[0].v = tt;
  v[0].x = x[0];
  v[0].y = y[0];
  v[0].z = z[0];

  v[1].u = tl;
  v[1].v = tb;
  v[1].x = x[3];
  v[1].y = y[3];
  v[1].z = z[3];

  v[2].u = tr;
  v[2].v = tt;
  v[2].x = x[1];
  v[2].y = y[1];
  v[2].z = z[1];

  v[3].u = tr;
  v[3].v = tb;
  v[3].x = x[2];
  v[3].y = y[2];
  v[3].z = z[2];
#endif
}

// Oblique code - original taken from freetype2 (ftsynth.c)
void CGUIFontTTF::ObliqueGlyph(FT_GlyphSlot slot)
{
  /* only oblique outline glyphs */
  if (slot->format != FT_GLYPH_FORMAT_OUTLINE)
    return;

  /* we don't touch the advance width */

  /* For italic, simply apply a shear transform, with an angle */
  /* of about 12 degrees.                                      */

  FT_Matrix transform;
  transform.xx = 0x10000L;
  transform.yx = 0x00000L;

  transform.xy = 0x06000L;
  transform.yy = 0x10000L;

  FT_Outline_Transform(&slot->outline, &transform);
}

// Embolden code - original taken from freetype2 (ftsynth.c)
void CGUIFontTTF::SetGlyphStrength(FT_GlyphSlot slot, FT_Face face, int glyphStrength)
{
  if (slot->format != FT_GLYPH_FORMAT_OUTLINE)
    return;

  /* some reasonable strength */
  FT_Pos strength = FT_MulFix(face->units_per_EM, face->size->metrics.y_scale) / glyphStrength;

  FT_BBox bbox_before, bbox_after;
  FT_Outline_Get_CBox(&slot->outline, &bbox_before);
  FT_Outline_Embolden(&slot->outline, strength); // ignore error
  FT_Outline_Get_CBox(&slot->outline, &bbox_after);

  FT_Pos dx = bbox_after.xMax - bbox_before.xMax;
  FT_Pos dy = bbox_after.yMax - bbox_before.yMax;

  if (slot->advance.x)
    slot->advance.x += dx;

  if (slot->advance.y)
    slot->advance.y += dy;

  slot->metrics.width += dx;
  slot->metrics.height += dy;
  slot->metrics.horiBearingY += dy;
  slot->metrics.horiAdvance += dx;
  slot->metrics.vertBearingX -= dx / 2;
  slot->metrics.vertBearingY += dy;
  slot->metrics.vertAdvance += dy;
}

float CGUIFontTTF::GetTabSpaceLength()
{
  const Character* c = GetCharacter(DUMMY_SPACE, 0, &m_fontMain);
  return c ? c->m_advance * TAB_SPACE_LENGTH : 28.0f * TAB_SPACE_LENGTH;
}
