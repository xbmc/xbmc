/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */
//-----------------------------------------------------------------------
//
//  File:      StringUtils.cpp
//
//  Purpose:   ATL split string utility
//  Author:    Paul J. Weiss
//
//  Modified to use J O'Leary's std::string class by kraqh3d
//
//------------------------------------------------------------------------

#ifdef HAVE_NEW_CROSSGUID
#include <crossguid/guid.hpp>
#else
#include <guid.h>
#endif

#if defined(TARGET_ANDROID)
#include <androidjni/JNIThreading.h>
#endif

#include "CharsetConverter.h"
#include "LangInfo.h"
#include "StringUtils.h"
#include "XBDateTime.h"
#include "utils/log.h"

#include <algorithm>
#include <array>
#include <assert.h>
#include <codecvt>
#include <functional>
#include <inttypes.h>
#include <iomanip>
#include <locale>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <fstrcmp.h>
#include <memory.h>

// don't move or std functions end up in PCRE namespace
// clang-format off
#include "utils/RegExp.h"
// clang-format on

#define FORMAT_BLOCK_SIZE 512 // # of bytes for initial allocation for printf

namespace
{
/*!
 * \brief Converts a string to a number of a specified type, by using istringstream.
 * \param str The string to convert
 * \param fallback [OPT] The number to return when the conversion fails
 * \return The converted number, otherwise fallback if conversion fails
 */
template<typename T>
T NumberFromSS(std::string_view str, T fallback) noexcept
{
  std::istringstream iss{str.data()};
  T result{fallback};
  iss >> result;
  return result;
}
} // unnamed namespace

static constexpr const char* ADDON_GUID_RE = "^(\\{){0,1}[0-9a-fA-F]{8}\\-[0-9a-fA-F]{4}\\-[0-9a-fA-F]{4}\\-[0-9a-fA-F]{4}\\-[0-9a-fA-F]{12}(\\}){0,1}$";

/* empty string for use in returns by ref */
const std::string StringUtils::Empty = "";

// TODO: Review use of unicode_lowers & unicode_uppers. I believe they should be eliminated
// and replaced with std::tolower/toupper
//
// The use of unicode_lowers and unicode_uppers predates Kodi 20. It is used for wstring ToLower
// and ToUpper methods instead of c++ tolower/toupper. This is different from std::string Tolower
// and ToUpper which do use c++ tolower/toupper. This is puzzling since the behavior is
// significantly different. One of the differences is that these tables ignore locale, while
// std::tolower(c, locale) & std:toupper(c, locale) ONLY change the case of characters
// which are in the given (or current) locale.
//
// It is also possible that these tables were used to get some of the benefits of "FoldCase"
// (perhaps not realizing it). FoldCase uses much newer and more complete tables from ICU
// (see unicode_fold_upper & unicode_fold_lower, below).
//
// Review the comments and decide whether to continue using unicode_uppers/unicode_lowers
// and related code. In my opinion it is a mistake to continue their use.

//	Copyright (c) Leigh Brasington 2012.  All rights reserved.
//  This code may be used and reproduced without written permission.
//  http://www.leighb.com/tounicupper.htm
//
//	The tables were constructed from
//	http://publib.boulder.ibm.com/infocenter/iseries/v7r1m0/index.jsp?topic=%2Fnls%2Frbagslowtoupmaptable.htm

// These characters can be converted to/from upper/lower case independent of locale.
// This means that there is no locale where the the conversion gives different
// results, like some characters do.

static constexpr wchar_t unicode_lowers[] = {
  (wchar_t)0x0061, (wchar_t)0x0062, (wchar_t)0x0063, (wchar_t)0x0064, (wchar_t)0x0065, (wchar_t)0x0066, (wchar_t)0x0067, (wchar_t)0x0068, (wchar_t)0x0069,
  (wchar_t)0x006A, (wchar_t)0x006B, (wchar_t)0x006C, (wchar_t)0x006D, (wchar_t)0x006E, (wchar_t)0x006F, (wchar_t)0x0070, (wchar_t)0x0071, (wchar_t)0x0072,
  (wchar_t)0x0073, (wchar_t)0x0074, (wchar_t)0x0075, (wchar_t)0x0076, (wchar_t)0x0077, (wchar_t)0x0078, (wchar_t)0x0079, (wchar_t)0x007A, (wchar_t)0x00E0,
  (wchar_t)0x00E1, (wchar_t)0x00E2, (wchar_t)0x00E3, (wchar_t)0x00E4, (wchar_t)0x00E5, (wchar_t)0x00E6, (wchar_t)0x00E7, (wchar_t)0x00E8, (wchar_t)0x00E9,
  (wchar_t)0x00EA, (wchar_t)0x00EB, (wchar_t)0x00EC, (wchar_t)0x00ED, (wchar_t)0x00EE, (wchar_t)0x00EF, (wchar_t)0x00F0, (wchar_t)0x00F1, (wchar_t)0x00F2,
  (wchar_t)0x00F3, (wchar_t)0x00F4, (wchar_t)0x00F5, (wchar_t)0x00F6, (wchar_t)0x00F8, (wchar_t)0x00F9, (wchar_t)0x00FA, (wchar_t)0x00FB, (wchar_t)0x00FC,
  (wchar_t)0x00FD, (wchar_t)0x00FE, (wchar_t)0x00FF, (wchar_t)0x0101, (wchar_t)0x0103, (wchar_t)0x0105, (wchar_t)0x0107, (wchar_t)0x0109, (wchar_t)0x010B,
  (wchar_t)0x010D, (wchar_t)0x010F, (wchar_t)0x0111, (wchar_t)0x0113, (wchar_t)0x0115, (wchar_t)0x0117, (wchar_t)0x0119, (wchar_t)0x011B, (wchar_t)0x011D,
  (wchar_t)0x011F, (wchar_t)0x0121, (wchar_t)0x0123, (wchar_t)0x0125, (wchar_t)0x0127, (wchar_t)0x0129, (wchar_t)0x012B, (wchar_t)0x012D, (wchar_t)0x012F,
  (wchar_t)0x0131, (wchar_t)0x0133, (wchar_t)0x0135, (wchar_t)0x0137, (wchar_t)0x013A, (wchar_t)0x013C, (wchar_t)0x013E, (wchar_t)0x0140, (wchar_t)0x0142,
  (wchar_t)0x0144, (wchar_t)0x0146, (wchar_t)0x0148, (wchar_t)0x014B, (wchar_t)0x014D, (wchar_t)0x014F, (wchar_t)0x0151, (wchar_t)0x0153, (wchar_t)0x0155,
  (wchar_t)0x0157, (wchar_t)0x0159, (wchar_t)0x015B, (wchar_t)0x015D, (wchar_t)0x015F, (wchar_t)0x0161, (wchar_t)0x0163, (wchar_t)0x0165, (wchar_t)0x0167,
  (wchar_t)0x0169, (wchar_t)0x016B, (wchar_t)0x016D, (wchar_t)0x016F, (wchar_t)0x0171, (wchar_t)0x0173, (wchar_t)0x0175, (wchar_t)0x0177, (wchar_t)0x017A,
  (wchar_t)0x017C, (wchar_t)0x017E, (wchar_t)0x0183, (wchar_t)0x0185, (wchar_t)0x0188, (wchar_t)0x018C, (wchar_t)0x0192, (wchar_t)0x0199, (wchar_t)0x01A1,
  (wchar_t)0x01A3, (wchar_t)0x01A5, (wchar_t)0x01A8, (wchar_t)0x01AD, (wchar_t)0x01B0, (wchar_t)0x01B4, (wchar_t)0x01B6, (wchar_t)0x01B9, (wchar_t)0x01BD,
  (wchar_t)0x01C6, (wchar_t)0x01C9, (wchar_t)0x01CC, (wchar_t)0x01CE, (wchar_t)0x01D0, (wchar_t)0x01D2, (wchar_t)0x01D4, (wchar_t)0x01D6, (wchar_t)0x01D8,
  (wchar_t)0x01DA, (wchar_t)0x01DC, (wchar_t)0x01DF, (wchar_t)0x01E1, (wchar_t)0x01E3, (wchar_t)0x01E5, (wchar_t)0x01E7, (wchar_t)0x01E9, (wchar_t)0x01EB,
  (wchar_t)0x01ED, (wchar_t)0x01EF, (wchar_t)0x01F3, (wchar_t)0x01F5, (wchar_t)0x01FB, (wchar_t)0x01FD, (wchar_t)0x01FF, (wchar_t)0x0201, (wchar_t)0x0203,
  (wchar_t)0x0205, (wchar_t)0x0207, (wchar_t)0x0209, (wchar_t)0x020B, (wchar_t)0x020D, (wchar_t)0x020F, (wchar_t)0x0211, (wchar_t)0x0213, (wchar_t)0x0215,
  (wchar_t)0x0217, (wchar_t)0x0253, (wchar_t)0x0254, (wchar_t)0x0257, (wchar_t)0x0258, (wchar_t)0x0259, (wchar_t)0x025B, (wchar_t)0x0260, (wchar_t)0x0263,
  (wchar_t)0x0268, (wchar_t)0x0269, (wchar_t)0x026F, (wchar_t)0x0272, (wchar_t)0x0275, (wchar_t)0x0283, (wchar_t)0x0288, (wchar_t)0x028A, (wchar_t)0x028B,
  (wchar_t)0x0292, (wchar_t)0x03AC, (wchar_t)0x03AD, (wchar_t)0x03AE, (wchar_t)0x03AF, (wchar_t)0x03B1, (wchar_t)0x03B2, (wchar_t)0x03B3, (wchar_t)0x03B4,
  (wchar_t)0x03B5, (wchar_t)0x03B6, (wchar_t)0x03B7, (wchar_t)0x03B8, (wchar_t)0x03B9, (wchar_t)0x03BA, (wchar_t)0x03BB, (wchar_t)0x03BC, (wchar_t)0x03BD,
  (wchar_t)0x03BE, (wchar_t)0x03BF, (wchar_t)0x03C0, (wchar_t)0x03C1, (wchar_t)0x03C3, (wchar_t)0x03C4, (wchar_t)0x03C5, (wchar_t)0x03C6, (wchar_t)0x03C7,
  (wchar_t)0x03C8, (wchar_t)0x03C9, (wchar_t)0x03CA, (wchar_t)0x03CB, (wchar_t)0x03CC, (wchar_t)0x03CD, (wchar_t)0x03CE, (wchar_t)0x03E3, (wchar_t)0x03E5,
  (wchar_t)0x03E7, (wchar_t)0x03E9, (wchar_t)0x03EB, (wchar_t)0x03ED, (wchar_t)0x03EF, (wchar_t)0x0430, (wchar_t)0x0431, (wchar_t)0x0432, (wchar_t)0x0433,
  (wchar_t)0x0434, (wchar_t)0x0435, (wchar_t)0x0436, (wchar_t)0x0437, (wchar_t)0x0438, (wchar_t)0x0439, (wchar_t)0x043A, (wchar_t)0x043B, (wchar_t)0x043C,
  (wchar_t)0x043D, (wchar_t)0x043E, (wchar_t)0x043F, (wchar_t)0x0440, (wchar_t)0x0441, (wchar_t)0x0442, (wchar_t)0x0443, (wchar_t)0x0444, (wchar_t)0x0445,
  (wchar_t)0x0446, (wchar_t)0x0447, (wchar_t)0x0448, (wchar_t)0x0449, (wchar_t)0x044A, (wchar_t)0x044B, (wchar_t)0x044C, (wchar_t)0x044D, (wchar_t)0x044E,
  (wchar_t)0x044F, (wchar_t)0x0451, (wchar_t)0x0452, (wchar_t)0x0453, (wchar_t)0x0454, (wchar_t)0x0455, (wchar_t)0x0456, (wchar_t)0x0457, (wchar_t)0x0458,
  (wchar_t)0x0459, (wchar_t)0x045A, (wchar_t)0x045B, (wchar_t)0x045C, (wchar_t)0x045E, (wchar_t)0x045F, (wchar_t)0x0461, (wchar_t)0x0463, (wchar_t)0x0465,
  (wchar_t)0x0467, (wchar_t)0x0469, (wchar_t)0x046B, (wchar_t)0x046D, (wchar_t)0x046F, (wchar_t)0x0471, (wchar_t)0x0473, (wchar_t)0x0475, (wchar_t)0x0477,
  (wchar_t)0x0479, (wchar_t)0x047B, (wchar_t)0x047D, (wchar_t)0x047F, (wchar_t)0x0481, (wchar_t)0x0491, (wchar_t)0x0493, (wchar_t)0x0495, (wchar_t)0x0497,
  (wchar_t)0x0499, (wchar_t)0x049B, (wchar_t)0x049D, (wchar_t)0x049F, (wchar_t)0x04A1, (wchar_t)0x04A3, (wchar_t)0x04A5, (wchar_t)0x04A7, (wchar_t)0x04A9,
  (wchar_t)0x04AB, (wchar_t)0x04AD, (wchar_t)0x04AF, (wchar_t)0x04B1, (wchar_t)0x04B3, (wchar_t)0x04B5, (wchar_t)0x04B7, (wchar_t)0x04B9, (wchar_t)0x04BB,
  (wchar_t)0x04BD, (wchar_t)0x04BF, (wchar_t)0x04C2, (wchar_t)0x04C4, (wchar_t)0x04C8, (wchar_t)0x04CC, (wchar_t)0x04D1, (wchar_t)0x04D3, (wchar_t)0x04D5,
  (wchar_t)0x04D7, (wchar_t)0x04D9, (wchar_t)0x04DB, (wchar_t)0x04DD, (wchar_t)0x04DF, (wchar_t)0x04E1, (wchar_t)0x04E3, (wchar_t)0x04E5, (wchar_t)0x04E7,
  (wchar_t)0x04E9, (wchar_t)0x04EB, (wchar_t)0x04EF, (wchar_t)0x04F1, (wchar_t)0x04F3, (wchar_t)0x04F5, (wchar_t)0x04F9, (wchar_t)0x0561, (wchar_t)0x0562,
  (wchar_t)0x0563, (wchar_t)0x0564, (wchar_t)0x0565, (wchar_t)0x0566, (wchar_t)0x0567, (wchar_t)0x0568, (wchar_t)0x0569, (wchar_t)0x056A, (wchar_t)0x056B,
  (wchar_t)0x056C, (wchar_t)0x056D, (wchar_t)0x056E, (wchar_t)0x056F, (wchar_t)0x0570, (wchar_t)0x0571, (wchar_t)0x0572, (wchar_t)0x0573, (wchar_t)0x0574,
  (wchar_t)0x0575, (wchar_t)0x0576, (wchar_t)0x0577, (wchar_t)0x0578, (wchar_t)0x0579, (wchar_t)0x057A, (wchar_t)0x057B, (wchar_t)0x057C, (wchar_t)0x057D,
  (wchar_t)0x057E, (wchar_t)0x057F, (wchar_t)0x0580, (wchar_t)0x0581, (wchar_t)0x0582, (wchar_t)0x0583, (wchar_t)0x0584, (wchar_t)0x0585, (wchar_t)0x0586,
  (wchar_t)0x10D0, (wchar_t)0x10D1, (wchar_t)0x10D2, (wchar_t)0x10D3, (wchar_t)0x10D4, (wchar_t)0x10D5, (wchar_t)0x10D6, (wchar_t)0x10D7, (wchar_t)0x10D8,
  (wchar_t)0x10D9, (wchar_t)0x10DA, (wchar_t)0x10DB, (wchar_t)0x10DC, (wchar_t)0x10DD, (wchar_t)0x10DE, (wchar_t)0x10DF, (wchar_t)0x10E0, (wchar_t)0x10E1,
  (wchar_t)0x10E2, (wchar_t)0x10E3, (wchar_t)0x10E4, (wchar_t)0x10E5, (wchar_t)0x10E6, (wchar_t)0x10E7, (wchar_t)0x10E8, (wchar_t)0x10E9, (wchar_t)0x10EA,
  (wchar_t)0x10EB, (wchar_t)0x10EC, (wchar_t)0x10ED, (wchar_t)0x10EE, (wchar_t)0x10EF, (wchar_t)0x10F0, (wchar_t)0x10F1, (wchar_t)0x10F2, (wchar_t)0x10F3,
  (wchar_t)0x10F4, (wchar_t)0x10F5, (wchar_t)0x1E01, (wchar_t)0x1E03, (wchar_t)0x1E05, (wchar_t)0x1E07, (wchar_t)0x1E09, (wchar_t)0x1E0B, (wchar_t)0x1E0D,
  (wchar_t)0x1E0F, (wchar_t)0x1E11, (wchar_t)0x1E13, (wchar_t)0x1E15, (wchar_t)0x1E17, (wchar_t)0x1E19, (wchar_t)0x1E1B, (wchar_t)0x1E1D, (wchar_t)0x1E1F,
  (wchar_t)0x1E21, (wchar_t)0x1E23, (wchar_t)0x1E25, (wchar_t)0x1E27, (wchar_t)0x1E29, (wchar_t)0x1E2B, (wchar_t)0x1E2D, (wchar_t)0x1E2F, (wchar_t)0x1E31,
  (wchar_t)0x1E33, (wchar_t)0x1E35, (wchar_t)0x1E37, (wchar_t)0x1E39, (wchar_t)0x1E3B, (wchar_t)0x1E3D, (wchar_t)0x1E3F, (wchar_t)0x1E41, (wchar_t)0x1E43,
  (wchar_t)0x1E45, (wchar_t)0x1E47, (wchar_t)0x1E49, (wchar_t)0x1E4B, (wchar_t)0x1E4D, (wchar_t)0x1E4F, (wchar_t)0x1E51, (wchar_t)0x1E53, (wchar_t)0x1E55,
  (wchar_t)0x1E57, (wchar_t)0x1E59, (wchar_t)0x1E5B, (wchar_t)0x1E5D, (wchar_t)0x1E5F, (wchar_t)0x1E61, (wchar_t)0x1E63, (wchar_t)0x1E65, (wchar_t)0x1E67,
  (wchar_t)0x1E69, (wchar_t)0x1E6B, (wchar_t)0x1E6D, (wchar_t)0x1E6F, (wchar_t)0x1E71, (wchar_t)0x1E73, (wchar_t)0x1E75, (wchar_t)0x1E77, (wchar_t)0x1E79,
  (wchar_t)0x1E7B, (wchar_t)0x1E7D, (wchar_t)0x1E7F, (wchar_t)0x1E81, (wchar_t)0x1E83, (wchar_t)0x1E85, (wchar_t)0x1E87, (wchar_t)0x1E89, (wchar_t)0x1E8B,
  (wchar_t)0x1E8D, (wchar_t)0x1E8F, (wchar_t)0x1E91, (wchar_t)0x1E93, (wchar_t)0x1E95, (wchar_t)0x1EA1, (wchar_t)0x1EA3, (wchar_t)0x1EA5, (wchar_t)0x1EA7,
  (wchar_t)0x1EA9, (wchar_t)0x1EAB, (wchar_t)0x1EAD, (wchar_t)0x1EAF, (wchar_t)0x1EB1, (wchar_t)0x1EB3, (wchar_t)0x1EB5, (wchar_t)0x1EB7, (wchar_t)0x1EB9,
  (wchar_t)0x1EBB, (wchar_t)0x1EBD, (wchar_t)0x1EBF, (wchar_t)0x1EC1, (wchar_t)0x1EC3, (wchar_t)0x1EC5, (wchar_t)0x1EC7, (wchar_t)0x1EC9, (wchar_t)0x1ECB,
  (wchar_t)0x1ECD, (wchar_t)0x1ECF, (wchar_t)0x1ED1, (wchar_t)0x1ED3, (wchar_t)0x1ED5, (wchar_t)0x1ED7, (wchar_t)0x1ED9, (wchar_t)0x1EDB, (wchar_t)0x1EDD,
  (wchar_t)0x1EDF, (wchar_t)0x1EE1, (wchar_t)0x1EE3, (wchar_t)0x1EE5, (wchar_t)0x1EE7, (wchar_t)0x1EE9, (wchar_t)0x1EEB, (wchar_t)0x1EED, (wchar_t)0x1EEF,
  (wchar_t)0x1EF1, (wchar_t)0x1EF3, (wchar_t)0x1EF5, (wchar_t)0x1EF7, (wchar_t)0x1EF9, (wchar_t)0x1F00, (wchar_t)0x1F01, (wchar_t)0x1F02, (wchar_t)0x1F03,
  (wchar_t)0x1F04, (wchar_t)0x1F05, (wchar_t)0x1F06, (wchar_t)0x1F07, (wchar_t)0x1F10, (wchar_t)0x1F11, (wchar_t)0x1F12, (wchar_t)0x1F13, (wchar_t)0x1F14,
  (wchar_t)0x1F15, (wchar_t)0x1F20, (wchar_t)0x1F21, (wchar_t)0x1F22, (wchar_t)0x1F23, (wchar_t)0x1F24, (wchar_t)0x1F25, (wchar_t)0x1F26, (wchar_t)0x1F27,
  (wchar_t)0x1F30, (wchar_t)0x1F31, (wchar_t)0x1F32, (wchar_t)0x1F33, (wchar_t)0x1F34, (wchar_t)0x1F35, (wchar_t)0x1F36, (wchar_t)0x1F37, (wchar_t)0x1F40,
  (wchar_t)0x1F41, (wchar_t)0x1F42, (wchar_t)0x1F43, (wchar_t)0x1F44, (wchar_t)0x1F45, (wchar_t)0x1F51, (wchar_t)0x1F53, (wchar_t)0x1F55, (wchar_t)0x1F57,
  (wchar_t)0x1F60, (wchar_t)0x1F61, (wchar_t)0x1F62, (wchar_t)0x1F63, (wchar_t)0x1F64, (wchar_t)0x1F65, (wchar_t)0x1F66, (wchar_t)0x1F67, (wchar_t)0x1F80,
  (wchar_t)0x1F81, (wchar_t)0x1F82, (wchar_t)0x1F83, (wchar_t)0x1F84, (wchar_t)0x1F85, (wchar_t)0x1F86, (wchar_t)0x1F87, (wchar_t)0x1F90, (wchar_t)0x1F91,
  (wchar_t)0x1F92, (wchar_t)0x1F93, (wchar_t)0x1F94, (wchar_t)0x1F95, (wchar_t)0x1F96, (wchar_t)0x1F97, (wchar_t)0x1FA0, (wchar_t)0x1FA1, (wchar_t)0x1FA2,
  (wchar_t)0x1FA3, (wchar_t)0x1FA4, (wchar_t)0x1FA5, (wchar_t)0x1FA6, (wchar_t)0x1FA7, (wchar_t)0x1FB0, (wchar_t)0x1FB1, (wchar_t)0x1FD0, (wchar_t)0x1FD1,
  (wchar_t)0x1FE0, (wchar_t)0x1FE1, (wchar_t)0x24D0, (wchar_t)0x24D1, (wchar_t)0x24D2, (wchar_t)0x24D3, (wchar_t)0x24D4, (wchar_t)0x24D5, (wchar_t)0x24D6,
  (wchar_t)0x24D7, (wchar_t)0x24D8, (wchar_t)0x24D9, (wchar_t)0x24DA, (wchar_t)0x24DB, (wchar_t)0x24DC, (wchar_t)0x24DD, (wchar_t)0x24DE, (wchar_t)0x24DF,
  (wchar_t)0x24E0, (wchar_t)0x24E1, (wchar_t)0x24E2, (wchar_t)0x24E3, (wchar_t)0x24E4, (wchar_t)0x24E5, (wchar_t)0x24E6, (wchar_t)0x24E7, (wchar_t)0x24E8,
  (wchar_t)0x24E9, (wchar_t)0xFF41, (wchar_t)0xFF42, (wchar_t)0xFF43, (wchar_t)0xFF44, (wchar_t)0xFF45, (wchar_t)0xFF46, (wchar_t)0xFF47, (wchar_t)0xFF48,
  (wchar_t)0xFF49, (wchar_t)0xFF4A, (wchar_t)0xFF4B, (wchar_t)0xFF4C, (wchar_t)0xFF4D, (wchar_t)0xFF4E, (wchar_t)0xFF4F, (wchar_t)0xFF50, (wchar_t)0xFF51,
  (wchar_t)0xFF52, (wchar_t)0xFF53, (wchar_t)0xFF54, (wchar_t)0xFF55, (wchar_t)0xFF56, (wchar_t)0xFF57, (wchar_t)0xFF58, (wchar_t)0xFF59, (wchar_t)0xFF5A
};

static const wchar_t unicode_uppers[] = {
  (wchar_t)0x0041, (wchar_t)0x0042, (wchar_t)0x0043, (wchar_t)0x0044, (wchar_t)0x0045, (wchar_t)0x0046, (wchar_t)0x0047, (wchar_t)0x0048, (wchar_t)0x0049,
  (wchar_t)0x004A, (wchar_t)0x004B, (wchar_t)0x004C, (wchar_t)0x004D, (wchar_t)0x004E, (wchar_t)0x004F, (wchar_t)0x0050, (wchar_t)0x0051, (wchar_t)0x0052,
  (wchar_t)0x0053, (wchar_t)0x0054, (wchar_t)0x0055, (wchar_t)0x0056, (wchar_t)0x0057, (wchar_t)0x0058, (wchar_t)0x0059, (wchar_t)0x005A, (wchar_t)0x00C0,
  (wchar_t)0x00C1, (wchar_t)0x00C2, (wchar_t)0x00C3, (wchar_t)0x00C4, (wchar_t)0x00C5, (wchar_t)0x00C6, (wchar_t)0x00C7, (wchar_t)0x00C8, (wchar_t)0x00C9,
  (wchar_t)0x00CA, (wchar_t)0x00CB, (wchar_t)0x00CC, (wchar_t)0x00CD, (wchar_t)0x00CE, (wchar_t)0x00CF, (wchar_t)0x00D0, (wchar_t)0x00D1, (wchar_t)0x00D2,
  (wchar_t)0x00D3, (wchar_t)0x00D4, (wchar_t)0x00D5, (wchar_t)0x00D6, (wchar_t)0x00D8, (wchar_t)0x00D9, (wchar_t)0x00DA, (wchar_t)0x00DB, (wchar_t)0x00DC,
  (wchar_t)0x00DD, (wchar_t)0x00DE, (wchar_t)0x0178, (wchar_t)0x0100, (wchar_t)0x0102, (wchar_t)0x0104, (wchar_t)0x0106, (wchar_t)0x0108, (wchar_t)0x010A,
  (wchar_t)0x010C, (wchar_t)0x010E, (wchar_t)0x0110, (wchar_t)0x0112, (wchar_t)0x0114, (wchar_t)0x0116, (wchar_t)0x0118, (wchar_t)0x011A, (wchar_t)0x011C,
  (wchar_t)0x011E, (wchar_t)0x0120, (wchar_t)0x0122, (wchar_t)0x0124, (wchar_t)0x0126, (wchar_t)0x0128, (wchar_t)0x012A, (wchar_t)0x012C, (wchar_t)0x012E,
  (wchar_t)0x0049, (wchar_t)0x0132, (wchar_t)0x0134, (wchar_t)0x0136, (wchar_t)0x0139, (wchar_t)0x013B, (wchar_t)0x013D, (wchar_t)0x013F, (wchar_t)0x0141,
  (wchar_t)0x0143, (wchar_t)0x0145, (wchar_t)0x0147, (wchar_t)0x014A, (wchar_t)0x014C, (wchar_t)0x014E, (wchar_t)0x0150, (wchar_t)0x0152, (wchar_t)0x0154,
  (wchar_t)0x0156, (wchar_t)0x0158, (wchar_t)0x015A, (wchar_t)0x015C, (wchar_t)0x015E, (wchar_t)0x0160, (wchar_t)0x0162, (wchar_t)0x0164, (wchar_t)0x0166,
  (wchar_t)0x0168, (wchar_t)0x016A, (wchar_t)0x016C, (wchar_t)0x016E, (wchar_t)0x0170, (wchar_t)0x0172, (wchar_t)0x0174, (wchar_t)0x0176, (wchar_t)0x0179,
  (wchar_t)0x017B, (wchar_t)0x017D, (wchar_t)0x0182, (wchar_t)0x0184, (wchar_t)0x0187, (wchar_t)0x018B, (wchar_t)0x0191, (wchar_t)0x0198, (wchar_t)0x01A0,
  (wchar_t)0x01A2, (wchar_t)0x01A4, (wchar_t)0x01A7, (wchar_t)0x01AC, (wchar_t)0x01AF, (wchar_t)0x01B3, (wchar_t)0x01B5, (wchar_t)0x01B8, (wchar_t)0x01BC,
  (wchar_t)0x01C4, (wchar_t)0x01C7, (wchar_t)0x01CA, (wchar_t)0x01CD, (wchar_t)0x01CF, (wchar_t)0x01D1, (wchar_t)0x01D3, (wchar_t)0x01D5, (wchar_t)0x01D7,
  (wchar_t)0x01D9, (wchar_t)0x01DB, (wchar_t)0x01DE, (wchar_t)0x01E0, (wchar_t)0x01E2, (wchar_t)0x01E4, (wchar_t)0x01E6, (wchar_t)0x01E8, (wchar_t)0x01EA,
  (wchar_t)0x01EC, (wchar_t)0x01EE, (wchar_t)0x01F1, (wchar_t)0x01F4, (wchar_t)0x01FA, (wchar_t)0x01FC, (wchar_t)0x01FE, (wchar_t)0x0200, (wchar_t)0x0202,
  (wchar_t)0x0204, (wchar_t)0x0206, (wchar_t)0x0208, (wchar_t)0x020A, (wchar_t)0x020C, (wchar_t)0x020E, (wchar_t)0x0210, (wchar_t)0x0212, (wchar_t)0x0214,
  (wchar_t)0x0216, (wchar_t)0x0181, (wchar_t)0x0186, (wchar_t)0x018A, (wchar_t)0x018E, (wchar_t)0x018F, (wchar_t)0x0190, (wchar_t)0x0193, (wchar_t)0x0194,
  (wchar_t)0x0197, (wchar_t)0x0196, (wchar_t)0x019C, (wchar_t)0x019D, (wchar_t)0x019F, (wchar_t)0x01A9, (wchar_t)0x01AE, (wchar_t)0x01B1, (wchar_t)0x01B2,
  (wchar_t)0x01B7, (wchar_t)0x0386, (wchar_t)0x0388, (wchar_t)0x0389, (wchar_t)0x038A, (wchar_t)0x0391, (wchar_t)0x0392, (wchar_t)0x0393, (wchar_t)0x0394,
  (wchar_t)0x0395, (wchar_t)0x0396, (wchar_t)0x0397, (wchar_t)0x0398, (wchar_t)0x0399, (wchar_t)0x039A, (wchar_t)0x039B, (wchar_t)0x039C, (wchar_t)0x039D,
  (wchar_t)0x039E, (wchar_t)0x039F, (wchar_t)0x03A0, (wchar_t)0x03A1, (wchar_t)0x03A3, (wchar_t)0x03A4, (wchar_t)0x03A5, (wchar_t)0x03A6, (wchar_t)0x03A7,
  (wchar_t)0x03A8, (wchar_t)0x03A9, (wchar_t)0x03AA, (wchar_t)0x03AB, (wchar_t)0x038C, (wchar_t)0x038E, (wchar_t)0x038F, (wchar_t)0x03E2, (wchar_t)0x03E4,
  (wchar_t)0x03E6, (wchar_t)0x03E8, (wchar_t)0x03EA, (wchar_t)0x03EC, (wchar_t)0x03EE, (wchar_t)0x0410, (wchar_t)0x0411, (wchar_t)0x0412, (wchar_t)0x0413,
  (wchar_t)0x0414, (wchar_t)0x0415, (wchar_t)0x0416, (wchar_t)0x0417, (wchar_t)0x0418, (wchar_t)0x0419, (wchar_t)0x041A, (wchar_t)0x041B, (wchar_t)0x041C,
  (wchar_t)0x041D, (wchar_t)0x041E, (wchar_t)0x041F, (wchar_t)0x0420, (wchar_t)0x0421, (wchar_t)0x0422, (wchar_t)0x0423, (wchar_t)0x0424, (wchar_t)0x0425,
  (wchar_t)0x0426, (wchar_t)0x0427, (wchar_t)0x0428, (wchar_t)0x0429, (wchar_t)0x042A, (wchar_t)0x042B, (wchar_t)0x042C, (wchar_t)0x042D, (wchar_t)0x042E,
  (wchar_t)0x042F, (wchar_t)0x0401, (wchar_t)0x0402, (wchar_t)0x0403, (wchar_t)0x0404, (wchar_t)0x0405, (wchar_t)0x0406, (wchar_t)0x0407, (wchar_t)0x0408,
  (wchar_t)0x0409, (wchar_t)0x040A, (wchar_t)0x040B, (wchar_t)0x040C, (wchar_t)0x040E, (wchar_t)0x040F, (wchar_t)0x0460, (wchar_t)0x0462, (wchar_t)0x0464,
  (wchar_t)0x0466, (wchar_t)0x0468, (wchar_t)0x046A, (wchar_t)0x046C, (wchar_t)0x046E, (wchar_t)0x0470, (wchar_t)0x0472, (wchar_t)0x0474, (wchar_t)0x0476,
  (wchar_t)0x0478, (wchar_t)0x047A, (wchar_t)0x047C, (wchar_t)0x047E, (wchar_t)0x0480, (wchar_t)0x0490, (wchar_t)0x0492, (wchar_t)0x0494, (wchar_t)0x0496,
  (wchar_t)0x0498, (wchar_t)0x049A, (wchar_t)0x049C, (wchar_t)0x049E, (wchar_t)0x04A0, (wchar_t)0x04A2, (wchar_t)0x04A4, (wchar_t)0x04A6, (wchar_t)0x04A8,
  (wchar_t)0x04AA, (wchar_t)0x04AC, (wchar_t)0x04AE, (wchar_t)0x04B0, (wchar_t)0x04B2, (wchar_t)0x04B4, (wchar_t)0x04B6, (wchar_t)0x04B8, (wchar_t)0x04BA,
  (wchar_t)0x04BC, (wchar_t)0x04BE, (wchar_t)0x04C1, (wchar_t)0x04C3, (wchar_t)0x04C7, (wchar_t)0x04CB, (wchar_t)0x04D0, (wchar_t)0x04D2, (wchar_t)0x04D4,
  (wchar_t)0x04D6, (wchar_t)0x04D8, (wchar_t)0x04DA, (wchar_t)0x04DC, (wchar_t)0x04DE, (wchar_t)0x04E0, (wchar_t)0x04E2, (wchar_t)0x04E4, (wchar_t)0x04E6,
  (wchar_t)0x04E8, (wchar_t)0x04EA, (wchar_t)0x04EE, (wchar_t)0x04F0, (wchar_t)0x04F2, (wchar_t)0x04F4, (wchar_t)0x04F8, (wchar_t)0x0531, (wchar_t)0x0532,
  (wchar_t)0x0533, (wchar_t)0x0534, (wchar_t)0x0535, (wchar_t)0x0536, (wchar_t)0x0537, (wchar_t)0x0538, (wchar_t)0x0539, (wchar_t)0x053A, (wchar_t)0x053B,
  (wchar_t)0x053C, (wchar_t)0x053D, (wchar_t)0x053E, (wchar_t)0x053F, (wchar_t)0x0540, (wchar_t)0x0541, (wchar_t)0x0542, (wchar_t)0x0543, (wchar_t)0x0544,
  (wchar_t)0x0545, (wchar_t)0x0546, (wchar_t)0x0547, (wchar_t)0x0548, (wchar_t)0x0549, (wchar_t)0x054A, (wchar_t)0x054B, (wchar_t)0x054C, (wchar_t)0x054D,
  (wchar_t)0x054E, (wchar_t)0x054F, (wchar_t)0x0550, (wchar_t)0x0551, (wchar_t)0x0552, (wchar_t)0x0553, (wchar_t)0x0554, (wchar_t)0x0555, (wchar_t)0x0556,
  (wchar_t)0x10A0, (wchar_t)0x10A1, (wchar_t)0x10A2, (wchar_t)0x10A3, (wchar_t)0x10A4, (wchar_t)0x10A5, (wchar_t)0x10A6, (wchar_t)0x10A7, (wchar_t)0x10A8,
  (wchar_t)0x10A9, (wchar_t)0x10AA, (wchar_t)0x10AB, (wchar_t)0x10AC, (wchar_t)0x10AD, (wchar_t)0x10AE, (wchar_t)0x10AF, (wchar_t)0x10B0, (wchar_t)0x10B1,
  (wchar_t)0x10B2, (wchar_t)0x10B3, (wchar_t)0x10B4, (wchar_t)0x10B5, (wchar_t)0x10B6, (wchar_t)0x10B7, (wchar_t)0x10B8, (wchar_t)0x10B9, (wchar_t)0x10BA,
  (wchar_t)0x10BB, (wchar_t)0x10BC, (wchar_t)0x10BD, (wchar_t)0x10BE, (wchar_t)0x10BF, (wchar_t)0x10C0, (wchar_t)0x10C1, (wchar_t)0x10C2, (wchar_t)0x10C3,
  (wchar_t)0x10C4, (wchar_t)0x10C5, (wchar_t)0x1E00, (wchar_t)0x1E02, (wchar_t)0x1E04, (wchar_t)0x1E06, (wchar_t)0x1E08, (wchar_t)0x1E0A, (wchar_t)0x1E0C,
  (wchar_t)0x1E0E, (wchar_t)0x1E10, (wchar_t)0x1E12, (wchar_t)0x1E14, (wchar_t)0x1E16, (wchar_t)0x1E18, (wchar_t)0x1E1A, (wchar_t)0x1E1C, (wchar_t)0x1E1E,
  (wchar_t)0x1E20, (wchar_t)0x1E22, (wchar_t)0x1E24, (wchar_t)0x1E26, (wchar_t)0x1E28, (wchar_t)0x1E2A, (wchar_t)0x1E2C, (wchar_t)0x1E2E, (wchar_t)0x1E30,
  (wchar_t)0x1E32, (wchar_t)0x1E34, (wchar_t)0x1E36, (wchar_t)0x1E38, (wchar_t)0x1E3A, (wchar_t)0x1E3C, (wchar_t)0x1E3E, (wchar_t)0x1E40, (wchar_t)0x1E42,
  (wchar_t)0x1E44, (wchar_t)0x1E46, (wchar_t)0x1E48, (wchar_t)0x1E4A, (wchar_t)0x1E4C, (wchar_t)0x1E4E, (wchar_t)0x1E50, (wchar_t)0x1E52, (wchar_t)0x1E54,
  (wchar_t)0x1E56, (wchar_t)0x1E58, (wchar_t)0x1E5A, (wchar_t)0x1E5C, (wchar_t)0x1E5E, (wchar_t)0x1E60, (wchar_t)0x1E62, (wchar_t)0x1E64, (wchar_t)0x1E66,
  (wchar_t)0x1E68, (wchar_t)0x1E6A, (wchar_t)0x1E6C, (wchar_t)0x1E6E, (wchar_t)0x1E70, (wchar_t)0x1E72, (wchar_t)0x1E74, (wchar_t)0x1E76, (wchar_t)0x1E78,
  (wchar_t)0x1E7A, (wchar_t)0x1E7C, (wchar_t)0x1E7E, (wchar_t)0x1E80, (wchar_t)0x1E82, (wchar_t)0x1E84, (wchar_t)0x1E86, (wchar_t)0x1E88, (wchar_t)0x1E8A,
  (wchar_t)0x1E8C, (wchar_t)0x1E8E, (wchar_t)0x1E90, (wchar_t)0x1E92, (wchar_t)0x1E94, (wchar_t)0x1EA0, (wchar_t)0x1EA2, (wchar_t)0x1EA4, (wchar_t)0x1EA6,
  (wchar_t)0x1EA8, (wchar_t)0x1EAA, (wchar_t)0x1EAC, (wchar_t)0x1EAE, (wchar_t)0x1EB0, (wchar_t)0x1EB2, (wchar_t)0x1EB4, (wchar_t)0x1EB6, (wchar_t)0x1EB8,
  (wchar_t)0x1EBA, (wchar_t)0x1EBC, (wchar_t)0x1EBE, (wchar_t)0x1EC0, (wchar_t)0x1EC2, (wchar_t)0x1EC4, (wchar_t)0x1EC6, (wchar_t)0x1EC8, (wchar_t)0x1ECA,
  (wchar_t)0x1ECC, (wchar_t)0x1ECE, (wchar_t)0x1ED0, (wchar_t)0x1ED2, (wchar_t)0x1ED4, (wchar_t)0x1ED6, (wchar_t)0x1ED8, (wchar_t)0x1EDA, (wchar_t)0x1EDC,
  (wchar_t)0x1EDE, (wchar_t)0x1EE0, (wchar_t)0x1EE2, (wchar_t)0x1EE4, (wchar_t)0x1EE6, (wchar_t)0x1EE8, (wchar_t)0x1EEA, (wchar_t)0x1EEC, (wchar_t)0x1EEE,
  (wchar_t)0x1EF0, (wchar_t)0x1EF2, (wchar_t)0x1EF4, (wchar_t)0x1EF6, (wchar_t)0x1EF8, (wchar_t)0x1F08, (wchar_t)0x1F09, (wchar_t)0x1F0A, (wchar_t)0x1F0B,
  (wchar_t)0x1F0C, (wchar_t)0x1F0D, (wchar_t)0x1F0E, (wchar_t)0x1F0F, (wchar_t)0x1F18, (wchar_t)0x1F19, (wchar_t)0x1F1A, (wchar_t)0x1F1B, (wchar_t)0x1F1C,
  (wchar_t)0x1F1D, (wchar_t)0x1F28, (wchar_t)0x1F29, (wchar_t)0x1F2A, (wchar_t)0x1F2B, (wchar_t)0x1F2C, (wchar_t)0x1F2D, (wchar_t)0x1F2E, (wchar_t)0x1F2F,
  (wchar_t)0x1F38, (wchar_t)0x1F39, (wchar_t)0x1F3A, (wchar_t)0x1F3B, (wchar_t)0x1F3C, (wchar_t)0x1F3D, (wchar_t)0x1F3E, (wchar_t)0x1F3F, (wchar_t)0x1F48,
  (wchar_t)0x1F49, (wchar_t)0x1F4A, (wchar_t)0x1F4B, (wchar_t)0x1F4C, (wchar_t)0x1F4D, (wchar_t)0x1F59, (wchar_t)0x1F5B, (wchar_t)0x1F5D, (wchar_t)0x1F5F,
  (wchar_t)0x1F68, (wchar_t)0x1F69, (wchar_t)0x1F6A, (wchar_t)0x1F6B, (wchar_t)0x1F6C, (wchar_t)0x1F6D, (wchar_t)0x1F6E, (wchar_t)0x1F6F, (wchar_t)0x1F88,
  (wchar_t)0x1F89, (wchar_t)0x1F8A, (wchar_t)0x1F8B, (wchar_t)0x1F8C, (wchar_t)0x1F8D, (wchar_t)0x1F8E, (wchar_t)0x1F8F, (wchar_t)0x1F98, (wchar_t)0x1F99,
  (wchar_t)0x1F9A, (wchar_t)0x1F9B, (wchar_t)0x1F9C, (wchar_t)0x1F9D, (wchar_t)0x1F9E, (wchar_t)0x1F9F, (wchar_t)0x1FA8, (wchar_t)0x1FA9, (wchar_t)0x1FAA,
  (wchar_t)0x1FAB, (wchar_t)0x1FAC, (wchar_t)0x1FAD, (wchar_t)0x1FAE, (wchar_t)0x1FAF, (wchar_t)0x1FB8, (wchar_t)0x1FB9, (wchar_t)0x1FD8, (wchar_t)0x1FD9,
  (wchar_t)0x1FE8, (wchar_t)0x1FE9, (wchar_t)0x24B6, (wchar_t)0x24B7, (wchar_t)0x24B8, (wchar_t)0x24B9, (wchar_t)0x24BA, (wchar_t)0x24BB, (wchar_t)0x24BC,
  (wchar_t)0x24BD, (wchar_t)0x24BE, (wchar_t)0x24BF, (wchar_t)0x24C0, (wchar_t)0x24C1, (wchar_t)0x24C2, (wchar_t)0x24C3, (wchar_t)0x24C4, (wchar_t)0x24C5,
  (wchar_t)0x24C6, (wchar_t)0x24C7, (wchar_t)0x24C8, (wchar_t)0x24C9, (wchar_t)0x24CA, (wchar_t)0x24CB, (wchar_t)0x24CC, (wchar_t)0x24CD, (wchar_t)0x24CE,
  (wchar_t)0x24CF, (wchar_t)0xFF21, (wchar_t)0xFF22, (wchar_t)0xFF23, (wchar_t)0xFF24, (wchar_t)0xFF25, (wchar_t)0xFF26, (wchar_t)0xFF27, (wchar_t)0xFF28,
  (wchar_t)0xFF29, (wchar_t)0xFF2A, (wchar_t)0xFF2B, (wchar_t)0xFF2C, (wchar_t)0xFF2D, (wchar_t)0xFF2E, (wchar_t)0xFF2F, (wchar_t)0xFF30, (wchar_t)0xFF31,
  (wchar_t)0xFF32, (wchar_t)0xFF33, (wchar_t)0xFF34, (wchar_t)0xFF35, (wchar_t)0xFF36, (wchar_t)0xFF37, (wchar_t)0xFF38, (wchar_t)0xFF39, (wchar_t)0xFF3A
};

/* Case folding tables (unicode_fold_upper & unicode_fold_lower) are derived from Unicode Inc.
 *  Data file, CaseFolding.txt (CaseFolding-14.0.0.txt, 2021-03-08). Copyright follows below.
 *
 * These tables provide for "simple case folding" that is not locale sensitive. They do NOT
 * support "Full Case Folding" which can fold single characters into multiple, or multiple
 * into single, etc. CaseFolding.txt can be found in the ICUC4 source directory:
 * icu/source/data/unidata.
 *
 * Terms of use:
 *
 * UNICODE, INC. LICENSE AGREEMENT - DATA FILES AND SOFTWARE
 *
 * See Terms of Use <https://www.unicode.org/copyright.html>
 * for definitions of Unicode Inc.’s Data Files and Software.
 *
 * NOTICE TO USER: Carefully read the following legal agreement.
 * BY DOWNLOADING, INSTALLING, COPYING OR OTHERWISE USING UNICODE INC.'S
 * DATA FILES ("DATA FILES"), AND/OR SOFTWARE ("SOFTWARE"),
 * YOU UNEQUIVOCALLY ACCEPT, AND AGREE TO BE BOUND BY, ALL OF THE
 * TERMS AND CONDITIONS OF THIS AGREEMENT.
 * IF YOU DO NOT AGREE, DO NOT DOWNLOAD, INSTALL, COPY, DISTRIBUTE OR USE
 * THE DATA FILES OR SOFTWARE.
 *
 * COPYRIGHT AND PERMISSION NOTICE
 *
 * Copyright © 1991-2022 Unicode, Inc. All rights reserved.
 * Distributed under the Terms of Use in https://www.unicode.org/copyright.html.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of the Unicode data files and any associated documentation
 * (the "Data Files") or Unicode software and any associated documentation
 * (the "Software") to deal in the Data Files or Software
 * without restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, and/or sell copies of
 * the Data Files or Software, and to permit persons to whom the Data Files
 * or Software are furnished to do so, provided that either
 * (a) this copyright and permission notice appear with all copies
 * of the Data Files or Software, or
 * (b) this copyright and permission notice appear in associated
 * Documentation.
 *
 * THE DATA FILES AND SOFTWARE ARE PROVIDED "AS IS", WITHOUT WARRANTY OF
 * ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT OF THIRD PARTY RIGHTS.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS INCLUDED IN THIS
 * NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT OR CONSEQUENTIAL
 * DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THE DATA FILES OR SOFTWARE.
 *
 * Except as contained in this notice, the name of a copyright holder
 * shall not be used in advertising or otherwise to promote the sale,
 * use or other dealings in these Data Files or Software without prior
 * written authorization of the copyright holder.
 */

// unicode_fold_upper is the upper-case equivalent to unicode_fold_lower.
// The lists are ordered such that the (lower) folded-case equivalent of unicode_fold_upper[n]
// is at unicode_fold_lower[n]. Note that the terms here "upper/lower case" are informal.

static const char32_t unicode_fold_upper[] = {
    (char32_t)0x0041,  (char32_t)0x0042,  (char32_t)0x0043,  (char32_t)0x0044,  (char32_t)0x0045,
    (char32_t)0x0046,  (char32_t)0x0047,  (char32_t)0x0048,  (char32_t)0x0049,  (char32_t)0x004A,
    (char32_t)0x004B,  (char32_t)0x004C,  (char32_t)0x004D,  (char32_t)0x004E,  (char32_t)0x004F,
    (char32_t)0x0050,  (char32_t)0x0051,  (char32_t)0x0052,  (char32_t)0x0053,  (char32_t)0x0054,
    (char32_t)0x0055,  (char32_t)0x0056,  (char32_t)0x0057,  (char32_t)0x0058,  (char32_t)0x0059,
    (char32_t)0x005A,  (char32_t)0x00B5,  (char32_t)0x00C0,  (char32_t)0x00C1,  (char32_t)0x00C2,
    (char32_t)0x00C3,  (char32_t)0x00C4,  (char32_t)0x00C5,  (char32_t)0x00C6,  (char32_t)0x00C7,
    (char32_t)0x00C8,  (char32_t)0x00C9,  (char32_t)0x00CA,  (char32_t)0x00CB,  (char32_t)0x00CC,
    (char32_t)0x00CD,  (char32_t)0x00CE,  (char32_t)0x00CF,  (char32_t)0x00D0,  (char32_t)0x00D1,
    (char32_t)0x00D2,  (char32_t)0x00D3,  (char32_t)0x00D4,  (char32_t)0x00D5,  (char32_t)0x00D6,
    (char32_t)0x00D8,  (char32_t)0x00D9,  (char32_t)0x00DA,  (char32_t)0x00DB,  (char32_t)0x00DC,
    (char32_t)0x00DD,  (char32_t)0x00DE,  (char32_t)0x0100,  (char32_t)0x0102,  (char32_t)0x0104,
    (char32_t)0x0106,  (char32_t)0x0108,  (char32_t)0x010A,  (char32_t)0x010C,  (char32_t)0x010E,
    (char32_t)0x0110,  (char32_t)0x0112,  (char32_t)0x0114,  (char32_t)0x0116,  (char32_t)0x0118,
    (char32_t)0x011A,  (char32_t)0x011C,  (char32_t)0x011E,  (char32_t)0x0120,  (char32_t)0x0122,
    (char32_t)0x0124,  (char32_t)0x0126,  (char32_t)0x0128,  (char32_t)0x012A,  (char32_t)0x012C,
    (char32_t)0x012E,  (char32_t)0x0132,  (char32_t)0x0134,  (char32_t)0x0136,  (char32_t)0x0139,
    (char32_t)0x013B,  (char32_t)0x013D,  (char32_t)0x013F,  (char32_t)0x0141,  (char32_t)0x0143,
    (char32_t)0x0145,  (char32_t)0x0147,  (char32_t)0x014A,  (char32_t)0x014C,  (char32_t)0x014E,
    (char32_t)0x0150,  (char32_t)0x0152,  (char32_t)0x0154,  (char32_t)0x0156,  (char32_t)0x0158,
    (char32_t)0x015A,  (char32_t)0x015C,  (char32_t)0x015E,  (char32_t)0x0160,  (char32_t)0x0162,
    (char32_t)0x0164,  (char32_t)0x0166,  (char32_t)0x0168,  (char32_t)0x016A,  (char32_t)0x016C,
    (char32_t)0x016E,  (char32_t)0x0170,  (char32_t)0x0172,  (char32_t)0x0174,  (char32_t)0x0176,
    (char32_t)0x0178,  (char32_t)0x0179,  (char32_t)0x017B,  (char32_t)0x017D,  (char32_t)0x017F,
    (char32_t)0x0181,  (char32_t)0x0182,  (char32_t)0x0184,  (char32_t)0x0186,  (char32_t)0x0187,
    (char32_t)0x0189,  (char32_t)0x018A,  (char32_t)0x018B,  (char32_t)0x018E,  (char32_t)0x018F,
    (char32_t)0x0190,  (char32_t)0x0191,  (char32_t)0x0193,  (char32_t)0x0194,  (char32_t)0x0196,
    (char32_t)0x0197,  (char32_t)0x0198,  (char32_t)0x019C,  (char32_t)0x019D,  (char32_t)0x019F,
    (char32_t)0x01A0,  (char32_t)0x01A2,  (char32_t)0x01A4,  (char32_t)0x01A6,  (char32_t)0x01A7,
    (char32_t)0x01A9,  (char32_t)0x01AC,  (char32_t)0x01AE,  (char32_t)0x01AF,  (char32_t)0x01B1,
    (char32_t)0x01B2,  (char32_t)0x01B3,  (char32_t)0x01B5,  (char32_t)0x01B7,  (char32_t)0x01B8,
    (char32_t)0x01BC,  (char32_t)0x01C4,  (char32_t)0x01C5,  (char32_t)0x01C7,  (char32_t)0x01C8,
    (char32_t)0x01CA,  (char32_t)0x01CB,  (char32_t)0x01CD,  (char32_t)0x01CF,  (char32_t)0x01D1,
    (char32_t)0x01D3,  (char32_t)0x01D5,  (char32_t)0x01D7,  (char32_t)0x01D9,  (char32_t)0x01DB,
    (char32_t)0x01DE,  (char32_t)0x01E0,  (char32_t)0x01E2,  (char32_t)0x01E4,  (char32_t)0x01E6,
    (char32_t)0x01E8,  (char32_t)0x01EA,  (char32_t)0x01EC,  (char32_t)0x01EE,  (char32_t)0x01F1,
    (char32_t)0x01F2,  (char32_t)0x01F4,  (char32_t)0x01F6,  (char32_t)0x01F7,  (char32_t)0x01F8,
    (char32_t)0x01FA,  (char32_t)0x01FC,  (char32_t)0x01FE,  (char32_t)0x0200,  (char32_t)0x0202,
    (char32_t)0x0204,  (char32_t)0x0206,  (char32_t)0x0208,  (char32_t)0x020A,  (char32_t)0x020C,
    (char32_t)0x020E,  (char32_t)0x0210,  (char32_t)0x0212,  (char32_t)0x0214,  (char32_t)0x0216,
    (char32_t)0x0218,  (char32_t)0x021A,  (char32_t)0x021C,  (char32_t)0x021E,  (char32_t)0x0220,
    (char32_t)0x0222,  (char32_t)0x0224,  (char32_t)0x0226,  (char32_t)0x0228,  (char32_t)0x022A,
    (char32_t)0x022C,  (char32_t)0x022E,  (char32_t)0x0230,  (char32_t)0x0232,  (char32_t)0x023A,
    (char32_t)0x023B,  (char32_t)0x023D,  (char32_t)0x023E,  (char32_t)0x0241,  (char32_t)0x0243,
    (char32_t)0x0244,  (char32_t)0x0245,  (char32_t)0x0246,  (char32_t)0x0248,  (char32_t)0x024A,
    (char32_t)0x024C,  (char32_t)0x024E,  (char32_t)0x0345,  (char32_t)0x0370,  (char32_t)0x0372,
    (char32_t)0x0376,  (char32_t)0x037F,  (char32_t)0x0386,  (char32_t)0x0388,  (char32_t)0x0389,
    (char32_t)0x038A,  (char32_t)0x038C,  (char32_t)0x038E,  (char32_t)0x038F,  (char32_t)0x0391,
    (char32_t)0x0392,  (char32_t)0x0393,  (char32_t)0x0394,  (char32_t)0x0395,  (char32_t)0x0396,
    (char32_t)0x0397,  (char32_t)0x0398,  (char32_t)0x0399,  (char32_t)0x039A,  (char32_t)0x039B,
    (char32_t)0x039C,  (char32_t)0x039D,  (char32_t)0x039E,  (char32_t)0x039F,  (char32_t)0x03A0,
    (char32_t)0x03A1,  (char32_t)0x03A3,  (char32_t)0x03A4,  (char32_t)0x03A5,  (char32_t)0x03A6,
    (char32_t)0x03A7,  (char32_t)0x03A8,  (char32_t)0x03A9,  (char32_t)0x03AA,  (char32_t)0x03AB,
    (char32_t)0x03C2,  (char32_t)0x03CF,  (char32_t)0x03D0,  (char32_t)0x03D1,  (char32_t)0x03D5,
    (char32_t)0x03D6,  (char32_t)0x03D8,  (char32_t)0x03DA,  (char32_t)0x03DC,  (char32_t)0x03DE,
    (char32_t)0x03E0,  (char32_t)0x03E2,  (char32_t)0x03E4,  (char32_t)0x03E6,  (char32_t)0x03E8,
    (char32_t)0x03EA,  (char32_t)0x03EC,  (char32_t)0x03EE,  (char32_t)0x03F0,  (char32_t)0x03F1,
    (char32_t)0x03F4,  (char32_t)0x03F5,  (char32_t)0x03F7,  (char32_t)0x03F9,  (char32_t)0x03FA,
    (char32_t)0x03FD,  (char32_t)0x03FE,  (char32_t)0x03FF,  (char32_t)0x0400,  (char32_t)0x0401,
    (char32_t)0x0402,  (char32_t)0x0403,  (char32_t)0x0404,  (char32_t)0x0405,  (char32_t)0x0406,
    (char32_t)0x0407,  (char32_t)0x0408,  (char32_t)0x0409,  (char32_t)0x040A,  (char32_t)0x040B,
    (char32_t)0x040C,  (char32_t)0x040D,  (char32_t)0x040E,  (char32_t)0x040F,  (char32_t)0x0410,
    (char32_t)0x0411,  (char32_t)0x0412,  (char32_t)0x0413,  (char32_t)0x0414,  (char32_t)0x0415,
    (char32_t)0x0416,  (char32_t)0x0417,  (char32_t)0x0418,  (char32_t)0x0419,  (char32_t)0x041A,
    (char32_t)0x041B,  (char32_t)0x041C,  (char32_t)0x041D,  (char32_t)0x041E,  (char32_t)0x041F,
    (char32_t)0x0420,  (char32_t)0x0421,  (char32_t)0x0422,  (char32_t)0x0423,  (char32_t)0x0424,
    (char32_t)0x0425,  (char32_t)0x0426,  (char32_t)0x0427,  (char32_t)0x0428,  (char32_t)0x0429,
    (char32_t)0x042A,  (char32_t)0x042B,  (char32_t)0x042C,  (char32_t)0x042D,  (char32_t)0x042E,
    (char32_t)0x042F,  (char32_t)0x0460,  (char32_t)0x0462,  (char32_t)0x0464,  (char32_t)0x0466,
    (char32_t)0x0468,  (char32_t)0x046A,  (char32_t)0x046C,  (char32_t)0x046E,  (char32_t)0x0470,
    (char32_t)0x0472,  (char32_t)0x0474,  (char32_t)0x0476,  (char32_t)0x0478,  (char32_t)0x047A,
    (char32_t)0x047C,  (char32_t)0x047E,  (char32_t)0x0480,  (char32_t)0x048A,  (char32_t)0x048C,
    (char32_t)0x048E,  (char32_t)0x0490,  (char32_t)0x0492,  (char32_t)0x0494,  (char32_t)0x0496,
    (char32_t)0x0498,  (char32_t)0x049A,  (char32_t)0x049C,  (char32_t)0x049E,  (char32_t)0x04A0,
    (char32_t)0x04A2,  (char32_t)0x04A4,  (char32_t)0x04A6,  (char32_t)0x04A8,  (char32_t)0x04AA,
    (char32_t)0x04AC,  (char32_t)0x04AE,  (char32_t)0x04B0,  (char32_t)0x04B2,  (char32_t)0x04B4,
    (char32_t)0x04B6,  (char32_t)0x04B8,  (char32_t)0x04BA,  (char32_t)0x04BC,  (char32_t)0x04BE,
    (char32_t)0x04C0,  (char32_t)0x04C1,  (char32_t)0x04C3,  (char32_t)0x04C5,  (char32_t)0x04C7,
    (char32_t)0x04C9,  (char32_t)0x04CB,  (char32_t)0x04CD,  (char32_t)0x04D0,  (char32_t)0x04D2,
    (char32_t)0x04D4,  (char32_t)0x04D6,  (char32_t)0x04D8,  (char32_t)0x04DA,  (char32_t)0x04DC,
    (char32_t)0x04DE,  (char32_t)0x04E0,  (char32_t)0x04E2,  (char32_t)0x04E4,  (char32_t)0x04E6,
    (char32_t)0x04E8,  (char32_t)0x04EA,  (char32_t)0x04EC,  (char32_t)0x04EE,  (char32_t)0x04F0,
    (char32_t)0x04F2,  (char32_t)0x04F4,  (char32_t)0x04F6,  (char32_t)0x04F8,  (char32_t)0x04FA,
    (char32_t)0x04FC,  (char32_t)0x04FE,  (char32_t)0x0500,  (char32_t)0x0502,  (char32_t)0x0504,
    (char32_t)0x0506,  (char32_t)0x0508,  (char32_t)0x050A,  (char32_t)0x050C,  (char32_t)0x050E,
    (char32_t)0x0510,  (char32_t)0x0512,  (char32_t)0x0514,  (char32_t)0x0516,  (char32_t)0x0518,
    (char32_t)0x051A,  (char32_t)0x051C,  (char32_t)0x051E,  (char32_t)0x0520,  (char32_t)0x0522,
    (char32_t)0x0524,  (char32_t)0x0526,  (char32_t)0x0528,  (char32_t)0x052A,  (char32_t)0x052C,
    (char32_t)0x052E,  (char32_t)0x0531,  (char32_t)0x0532,  (char32_t)0x0533,  (char32_t)0x0534,
    (char32_t)0x0535,  (char32_t)0x0536,  (char32_t)0x0537,  (char32_t)0x0538,  (char32_t)0x0539,
    (char32_t)0x053A,  (char32_t)0x053B,  (char32_t)0x053C,  (char32_t)0x053D,  (char32_t)0x053E,
    (char32_t)0x053F,  (char32_t)0x0540,  (char32_t)0x0541,  (char32_t)0x0542,  (char32_t)0x0543,
    (char32_t)0x0544,  (char32_t)0x0545,  (char32_t)0x0546,  (char32_t)0x0547,  (char32_t)0x0548,
    (char32_t)0x0549,  (char32_t)0x054A,  (char32_t)0x054B,  (char32_t)0x054C,  (char32_t)0x054D,
    (char32_t)0x054E,  (char32_t)0x054F,  (char32_t)0x0550,  (char32_t)0x0551,  (char32_t)0x0552,
    (char32_t)0x0553,  (char32_t)0x0554,  (char32_t)0x0555,  (char32_t)0x0556,  (char32_t)0x10A0,
    (char32_t)0x10A1,  (char32_t)0x10A2,  (char32_t)0x10A3,  (char32_t)0x10A4,  (char32_t)0x10A5,
    (char32_t)0x10A6,  (char32_t)0x10A7,  (char32_t)0x10A8,  (char32_t)0x10A9,  (char32_t)0x10AA,
    (char32_t)0x10AB,  (char32_t)0x10AC,  (char32_t)0x10AD,  (char32_t)0x10AE,  (char32_t)0x10AF,
    (char32_t)0x10B0,  (char32_t)0x10B1,  (char32_t)0x10B2,  (char32_t)0x10B3,  (char32_t)0x10B4,
    (char32_t)0x10B5,  (char32_t)0x10B6,  (char32_t)0x10B7,  (char32_t)0x10B8,  (char32_t)0x10B9,
    (char32_t)0x10BA,  (char32_t)0x10BB,  (char32_t)0x10BC,  (char32_t)0x10BD,  (char32_t)0x10BE,
    (char32_t)0x10BF,  (char32_t)0x10C0,  (char32_t)0x10C1,  (char32_t)0x10C2,  (char32_t)0x10C3,
    (char32_t)0x10C4,  (char32_t)0x10C5,  (char32_t)0x10C7,  (char32_t)0x10CD,  (char32_t)0x13F8,
    (char32_t)0x13F9,  (char32_t)0x13FA,  (char32_t)0x13FB,  (char32_t)0x13FC,  (char32_t)0x13FD,
    (char32_t)0x1C80,  (char32_t)0x1C81,  (char32_t)0x1C82,  (char32_t)0x1C83,  (char32_t)0x1C84,
    (char32_t)0x1C85,  (char32_t)0x1C86,  (char32_t)0x1C87,  (char32_t)0x1C88,  (char32_t)0x1C90,
    (char32_t)0x1C91,  (char32_t)0x1C92,  (char32_t)0x1C93,  (char32_t)0x1C94,  (char32_t)0x1C95,
    (char32_t)0x1C96,  (char32_t)0x1C97,  (char32_t)0x1C98,  (char32_t)0x1C99,  (char32_t)0x1C9A,
    (char32_t)0x1C9B,  (char32_t)0x1C9C,  (char32_t)0x1C9D,  (char32_t)0x1C9E,  (char32_t)0x1C9F,
    (char32_t)0x1CA0,  (char32_t)0x1CA1,  (char32_t)0x1CA2,  (char32_t)0x1CA3,  (char32_t)0x1CA4,
    (char32_t)0x1CA5,  (char32_t)0x1CA6,  (char32_t)0x1CA7,  (char32_t)0x1CA8,  (char32_t)0x1CA9,
    (char32_t)0x1CAA,  (char32_t)0x1CAB,  (char32_t)0x1CAC,  (char32_t)0x1CAD,  (char32_t)0x1CAE,
    (char32_t)0x1CAF,  (char32_t)0x1CB0,  (char32_t)0x1CB1,  (char32_t)0x1CB2,  (char32_t)0x1CB3,
    (char32_t)0x1CB4,  (char32_t)0x1CB5,  (char32_t)0x1CB6,  (char32_t)0x1CB7,  (char32_t)0x1CB8,
    (char32_t)0x1CB9,  (char32_t)0x1CBA,  (char32_t)0x1CBD,  (char32_t)0x1CBE,  (char32_t)0x1CBF,
    (char32_t)0x1E00,  (char32_t)0x1E02,  (char32_t)0x1E04,  (char32_t)0x1E06,  (char32_t)0x1E08,
    (char32_t)0x1E0A,  (char32_t)0x1E0C,  (char32_t)0x1E0E,  (char32_t)0x1E10,  (char32_t)0x1E12,
    (char32_t)0x1E14,  (char32_t)0x1E16,  (char32_t)0x1E18,  (char32_t)0x1E1A,  (char32_t)0x1E1C,
    (char32_t)0x1E1E,  (char32_t)0x1E20,  (char32_t)0x1E22,  (char32_t)0x1E24,  (char32_t)0x1E26,
    (char32_t)0x1E28,  (char32_t)0x1E2A,  (char32_t)0x1E2C,  (char32_t)0x1E2E,  (char32_t)0x1E30,
    (char32_t)0x1E32,  (char32_t)0x1E34,  (char32_t)0x1E36,  (char32_t)0x1E38,  (char32_t)0x1E3A,
    (char32_t)0x1E3C,  (char32_t)0x1E3E,  (char32_t)0x1E40,  (char32_t)0x1E42,  (char32_t)0x1E44,
    (char32_t)0x1E46,  (char32_t)0x1E48,  (char32_t)0x1E4A,  (char32_t)0x1E4C,  (char32_t)0x1E4E,
    (char32_t)0x1E50,  (char32_t)0x1E52,  (char32_t)0x1E54,  (char32_t)0x1E56,  (char32_t)0x1E58,
    (char32_t)0x1E5A,  (char32_t)0x1E5C,  (char32_t)0x1E5E,  (char32_t)0x1E60,  (char32_t)0x1E62,
    (char32_t)0x1E64,  (char32_t)0x1E66,  (char32_t)0x1E68,  (char32_t)0x1E6A,  (char32_t)0x1E6C,
    (char32_t)0x1E6E,  (char32_t)0x1E70,  (char32_t)0x1E72,  (char32_t)0x1E74,  (char32_t)0x1E76,
    (char32_t)0x1E78,  (char32_t)0x1E7A,  (char32_t)0x1E7C,  (char32_t)0x1E7E,  (char32_t)0x1E80,
    (char32_t)0x1E82,  (char32_t)0x1E84,  (char32_t)0x1E86,  (char32_t)0x1E88,  (char32_t)0x1E8A,
    (char32_t)0x1E8C,  (char32_t)0x1E8E,  (char32_t)0x1E90,  (char32_t)0x1E92,  (char32_t)0x1E94,
    (char32_t)0x1E9B,  (char32_t)0x1E9E,  (char32_t)0x1EA0,  (char32_t)0x1EA2,  (char32_t)0x1EA4,
    (char32_t)0x1EA6,  (char32_t)0x1EA8,  (char32_t)0x1EAA,  (char32_t)0x1EAC,  (char32_t)0x1EAE,
    (char32_t)0x1EB0,  (char32_t)0x1EB2,  (char32_t)0x1EB4,  (char32_t)0x1EB6,  (char32_t)0x1EB8,
    (char32_t)0x1EBA,  (char32_t)0x1EBC,  (char32_t)0x1EBE,  (char32_t)0x1EC0,  (char32_t)0x1EC2,
    (char32_t)0x1EC4,  (char32_t)0x1EC6,  (char32_t)0x1EC8,  (char32_t)0x1ECA,  (char32_t)0x1ECC,
    (char32_t)0x1ECE,  (char32_t)0x1ED0,  (char32_t)0x1ED2,  (char32_t)0x1ED4,  (char32_t)0x1ED6,
    (char32_t)0x1ED8,  (char32_t)0x1EDA,  (char32_t)0x1EDC,  (char32_t)0x1EDE,  (char32_t)0x1EE0,
    (char32_t)0x1EE2,  (char32_t)0x1EE4,  (char32_t)0x1EE6,  (char32_t)0x1EE8,  (char32_t)0x1EEA,
    (char32_t)0x1EEC,  (char32_t)0x1EEE,  (char32_t)0x1EF0,  (char32_t)0x1EF2,  (char32_t)0x1EF4,
    (char32_t)0x1EF6,  (char32_t)0x1EF8,  (char32_t)0x1EFA,  (char32_t)0x1EFC,  (char32_t)0x1EFE,
    (char32_t)0x1F08,  (char32_t)0x1F09,  (char32_t)0x1F0A,  (char32_t)0x1F0B,  (char32_t)0x1F0C,
    (char32_t)0x1F0D,  (char32_t)0x1F0E,  (char32_t)0x1F0F,  (char32_t)0x1F18,  (char32_t)0x1F19,
    (char32_t)0x1F1A,  (char32_t)0x1F1B,  (char32_t)0x1F1C,  (char32_t)0x1F1D,  (char32_t)0x1F28,
    (char32_t)0x1F29,  (char32_t)0x1F2A,  (char32_t)0x1F2B,  (char32_t)0x1F2C,  (char32_t)0x1F2D,
    (char32_t)0x1F2E,  (char32_t)0x1F2F,  (char32_t)0x1F38,  (char32_t)0x1F39,  (char32_t)0x1F3A,
    (char32_t)0x1F3B,  (char32_t)0x1F3C,  (char32_t)0x1F3D,  (char32_t)0x1F3E,  (char32_t)0x1F3F,
    (char32_t)0x1F48,  (char32_t)0x1F49,  (char32_t)0x1F4A,  (char32_t)0x1F4B,  (char32_t)0x1F4C,
    (char32_t)0x1F4D,  (char32_t)0x1F59,  (char32_t)0x1F5B,  (char32_t)0x1F5D,  (char32_t)0x1F5F,
    (char32_t)0x1F68,  (char32_t)0x1F69,  (char32_t)0x1F6A,  (char32_t)0x1F6B,  (char32_t)0x1F6C,
    (char32_t)0x1F6D,  (char32_t)0x1F6E,  (char32_t)0x1F6F,  (char32_t)0x1F88,  (char32_t)0x1F89,
    (char32_t)0x1F8A,  (char32_t)0x1F8B,  (char32_t)0x1F8C,  (char32_t)0x1F8D,  (char32_t)0x1F8E,
    (char32_t)0x1F8F,  (char32_t)0x1F98,  (char32_t)0x1F99,  (char32_t)0x1F9A,  (char32_t)0x1F9B,
    (char32_t)0x1F9C,  (char32_t)0x1F9D,  (char32_t)0x1F9E,  (char32_t)0x1F9F,  (char32_t)0x1FA8,
    (char32_t)0x1FA9,  (char32_t)0x1FAA,  (char32_t)0x1FAB,  (char32_t)0x1FAC,  (char32_t)0x1FAD,
    (char32_t)0x1FAE,  (char32_t)0x1FAF,  (char32_t)0x1FB8,  (char32_t)0x1FB9,  (char32_t)0x1FBA,
    (char32_t)0x1FBB,  (char32_t)0x1FBC,  (char32_t)0x1FBE,  (char32_t)0x1FC8,  (char32_t)0x1FC9,
    (char32_t)0x1FCA,  (char32_t)0x1FCB,  (char32_t)0x1FCC,  (char32_t)0x1FD8,  (char32_t)0x1FD9,
    (char32_t)0x1FDA,  (char32_t)0x1FDB,  (char32_t)0x1FE8,  (char32_t)0x1FE9,  (char32_t)0x1FEA,
    (char32_t)0x1FEB,  (char32_t)0x1FEC,  (char32_t)0x1FF8,  (char32_t)0x1FF9,  (char32_t)0x1FFA,
    (char32_t)0x1FFB,  (char32_t)0x1FFC,  (char32_t)0x2126,  (char32_t)0x212A,  (char32_t)0x212B,
    (char32_t)0x2132,  (char32_t)0x2160,  (char32_t)0x2161,  (char32_t)0x2162,  (char32_t)0x2163,
    (char32_t)0x2164,  (char32_t)0x2165,  (char32_t)0x2166,  (char32_t)0x2167,  (char32_t)0x2168,
    (char32_t)0x2169,  (char32_t)0x216A,  (char32_t)0x216B,  (char32_t)0x216C,  (char32_t)0x216D,
    (char32_t)0x216E,  (char32_t)0x216F,  (char32_t)0x2183,  (char32_t)0x24B6,  (char32_t)0x24B7,
    (char32_t)0x24B8,  (char32_t)0x24B9,  (char32_t)0x24BA,  (char32_t)0x24BB,  (char32_t)0x24BC,
    (char32_t)0x24BD,  (char32_t)0x24BE,  (char32_t)0x24BF,  (char32_t)0x24C0,  (char32_t)0x24C1,
    (char32_t)0x24C2,  (char32_t)0x24C3,  (char32_t)0x24C4,  (char32_t)0x24C5,  (char32_t)0x24C6,
    (char32_t)0x24C7,  (char32_t)0x24C8,  (char32_t)0x24C9,  (char32_t)0x24CA,  (char32_t)0x24CB,
    (char32_t)0x24CC,  (char32_t)0x24CD,  (char32_t)0x24CE,  (char32_t)0x24CF,  (char32_t)0x2C00,
    (char32_t)0x2C01,  (char32_t)0x2C02,  (char32_t)0x2C03,  (char32_t)0x2C04,  (char32_t)0x2C05,
    (char32_t)0x2C06,  (char32_t)0x2C07,  (char32_t)0x2C08,  (char32_t)0x2C09,  (char32_t)0x2C0A,
    (char32_t)0x2C0B,  (char32_t)0x2C0C,  (char32_t)0x2C0D,  (char32_t)0x2C0E,  (char32_t)0x2C0F,
    (char32_t)0x2C10,  (char32_t)0x2C11,  (char32_t)0x2C12,  (char32_t)0x2C13,  (char32_t)0x2C14,
    (char32_t)0x2C15,  (char32_t)0x2C16,  (char32_t)0x2C17,  (char32_t)0x2C18,  (char32_t)0x2C19,
    (char32_t)0x2C1A,  (char32_t)0x2C1B,  (char32_t)0x2C1C,  (char32_t)0x2C1D,  (char32_t)0x2C1E,
    (char32_t)0x2C1F,  (char32_t)0x2C20,  (char32_t)0x2C21,  (char32_t)0x2C22,  (char32_t)0x2C23,
    (char32_t)0x2C24,  (char32_t)0x2C25,  (char32_t)0x2C26,  (char32_t)0x2C27,  (char32_t)0x2C28,
    (char32_t)0x2C29,  (char32_t)0x2C2A,  (char32_t)0x2C2B,  (char32_t)0x2C2C,  (char32_t)0x2C2D,
    (char32_t)0x2C2E,  (char32_t)0x2C2F,  (char32_t)0x2C60,  (char32_t)0x2C62,  (char32_t)0x2C63,
    (char32_t)0x2C64,  (char32_t)0x2C67,  (char32_t)0x2C69,  (char32_t)0x2C6B,  (char32_t)0x2C6D,
    (char32_t)0x2C6E,  (char32_t)0x2C6F,  (char32_t)0x2C70,  (char32_t)0x2C72,  (char32_t)0x2C75,
    (char32_t)0x2C7E,  (char32_t)0x2C7F,  (char32_t)0x2C80,  (char32_t)0x2C82,  (char32_t)0x2C84,
    (char32_t)0x2C86,  (char32_t)0x2C88,  (char32_t)0x2C8A,  (char32_t)0x2C8C,  (char32_t)0x2C8E,
    (char32_t)0x2C90,  (char32_t)0x2C92,  (char32_t)0x2C94,  (char32_t)0x2C96,  (char32_t)0x2C98,
    (char32_t)0x2C9A,  (char32_t)0x2C9C,  (char32_t)0x2C9E,  (char32_t)0x2CA0,  (char32_t)0x2CA2,
    (char32_t)0x2CA4,  (char32_t)0x2CA6,  (char32_t)0x2CA8,  (char32_t)0x2CAA,  (char32_t)0x2CAC,
    (char32_t)0x2CAE,  (char32_t)0x2CB0,  (char32_t)0x2CB2,  (char32_t)0x2CB4,  (char32_t)0x2CB6,
    (char32_t)0x2CB8,  (char32_t)0x2CBA,  (char32_t)0x2CBC,  (char32_t)0x2CBE,  (char32_t)0x2CC0,
    (char32_t)0x2CC2,  (char32_t)0x2CC4,  (char32_t)0x2CC6,  (char32_t)0x2CC8,  (char32_t)0x2CCA,
    (char32_t)0x2CCC,  (char32_t)0x2CCE,  (char32_t)0x2CD0,  (char32_t)0x2CD2,  (char32_t)0x2CD4,
    (char32_t)0x2CD6,  (char32_t)0x2CD8,  (char32_t)0x2CDA,  (char32_t)0x2CDC,  (char32_t)0x2CDE,
    (char32_t)0x2CE0,  (char32_t)0x2CE2,  (char32_t)0x2CEB,  (char32_t)0x2CED,  (char32_t)0x2CF2,
    (char32_t)0xA640,  (char32_t)0xA642,  (char32_t)0xA644,  (char32_t)0xA646,  (char32_t)0xA648,
    (char32_t)0xA64A,  (char32_t)0xA64C,  (char32_t)0xA64E,  (char32_t)0xA650,  (char32_t)0xA652,
    (char32_t)0xA654,  (char32_t)0xA656,  (char32_t)0xA658,  (char32_t)0xA65A,  (char32_t)0xA65C,
    (char32_t)0xA65E,  (char32_t)0xA660,  (char32_t)0xA662,  (char32_t)0xA664,  (char32_t)0xA666,
    (char32_t)0xA668,  (char32_t)0xA66A,  (char32_t)0xA66C,  (char32_t)0xA680,  (char32_t)0xA682,
    (char32_t)0xA684,  (char32_t)0xA686,  (char32_t)0xA688,  (char32_t)0xA68A,  (char32_t)0xA68C,
    (char32_t)0xA68E,  (char32_t)0xA690,  (char32_t)0xA692,  (char32_t)0xA694,  (char32_t)0xA696,
    (char32_t)0xA698,  (char32_t)0xA69A,  (char32_t)0xA722,  (char32_t)0xA724,  (char32_t)0xA726,
    (char32_t)0xA728,  (char32_t)0xA72A,  (char32_t)0xA72C,  (char32_t)0xA72E,  (char32_t)0xA732,
    (char32_t)0xA734,  (char32_t)0xA736,  (char32_t)0xA738,  (char32_t)0xA73A,  (char32_t)0xA73C,
    (char32_t)0xA73E,  (char32_t)0xA740,  (char32_t)0xA742,  (char32_t)0xA744,  (char32_t)0xA746,
    (char32_t)0xA748,  (char32_t)0xA74A,  (char32_t)0xA74C,  (char32_t)0xA74E,  (char32_t)0xA750,
    (char32_t)0xA752,  (char32_t)0xA754,  (char32_t)0xA756,  (char32_t)0xA758,  (char32_t)0xA75A,
    (char32_t)0xA75C,  (char32_t)0xA75E,  (char32_t)0xA760,  (char32_t)0xA762,  (char32_t)0xA764,
    (char32_t)0xA766,  (char32_t)0xA768,  (char32_t)0xA76A,  (char32_t)0xA76C,  (char32_t)0xA76E,
    (char32_t)0xA779,  (char32_t)0xA77B,  (char32_t)0xA77D,  (char32_t)0xA77E,  (char32_t)0xA780,
    (char32_t)0xA782,  (char32_t)0xA784,  (char32_t)0xA786,  (char32_t)0xA78B,  (char32_t)0xA78D,
    (char32_t)0xA790,  (char32_t)0xA792,  (char32_t)0xA796,  (char32_t)0xA798,  (char32_t)0xA79A,
    (char32_t)0xA79C,  (char32_t)0xA79E,  (char32_t)0xA7A0,  (char32_t)0xA7A2,  (char32_t)0xA7A4,
    (char32_t)0xA7A6,  (char32_t)0xA7A8,  (char32_t)0xA7AA,  (char32_t)0xA7AB,  (char32_t)0xA7AC,
    (char32_t)0xA7AD,  (char32_t)0xA7AE,  (char32_t)0xA7B0,  (char32_t)0xA7B1,  (char32_t)0xA7B2,
    (char32_t)0xA7B3,  (char32_t)0xA7B4,  (char32_t)0xA7B6,  (char32_t)0xA7B8,  (char32_t)0xA7BA,
    (char32_t)0xA7BC,  (char32_t)0xA7BE,  (char32_t)0xA7C0,  (char32_t)0xA7C2,  (char32_t)0xA7C4,
    (char32_t)0xA7C5,  (char32_t)0xA7C6,  (char32_t)0xA7C7,  (char32_t)0xA7C9,  (char32_t)0xA7D0,
    (char32_t)0xA7D6,  (char32_t)0xA7D8,  (char32_t)0xA7F5,  (char32_t)0xAB70,  (char32_t)0xAB71,
    (char32_t)0xAB72,  (char32_t)0xAB73,  (char32_t)0xAB74,  (char32_t)0xAB75,  (char32_t)0xAB76,
    (char32_t)0xAB77,  (char32_t)0xAB78,  (char32_t)0xAB79,  (char32_t)0xAB7A,  (char32_t)0xAB7B,
    (char32_t)0xAB7C,  (char32_t)0xAB7D,  (char32_t)0xAB7E,  (char32_t)0xAB7F,  (char32_t)0xAB80,
    (char32_t)0xAB81,  (char32_t)0xAB82,  (char32_t)0xAB83,  (char32_t)0xAB84,  (char32_t)0xAB85,
    (char32_t)0xAB86,  (char32_t)0xAB87,  (char32_t)0xAB88,  (char32_t)0xAB89,  (char32_t)0xAB8A,
    (char32_t)0xAB8B,  (char32_t)0xAB8C,  (char32_t)0xAB8D,  (char32_t)0xAB8E,  (char32_t)0xAB8F,
    (char32_t)0xAB90,  (char32_t)0xAB91,  (char32_t)0xAB92,  (char32_t)0xAB93,  (char32_t)0xAB94,
    (char32_t)0xAB95,  (char32_t)0xAB96,  (char32_t)0xAB97,  (char32_t)0xAB98,  (char32_t)0xAB99,
    (char32_t)0xAB9A,  (char32_t)0xAB9B,  (char32_t)0xAB9C,  (char32_t)0xAB9D,  (char32_t)0xAB9E,
    (char32_t)0xAB9F,  (char32_t)0xABA0,  (char32_t)0xABA1,  (char32_t)0xABA2,  (char32_t)0xABA3,
    (char32_t)0xABA4,  (char32_t)0xABA5,  (char32_t)0xABA6,  (char32_t)0xABA7,  (char32_t)0xABA8,
    (char32_t)0xABA9,  (char32_t)0xABAA,  (char32_t)0xABAB,  (char32_t)0xABAC,  (char32_t)0xABAD,
    (char32_t)0xABAE,  (char32_t)0xABAF,  (char32_t)0xABB0,  (char32_t)0xABB1,  (char32_t)0xABB2,
    (char32_t)0xABB3,  (char32_t)0xABB4,  (char32_t)0xABB5,  (char32_t)0xABB6,  (char32_t)0xABB7,
    (char32_t)0xABB8,  (char32_t)0xABB9,  (char32_t)0xABBA,  (char32_t)0xABBB,  (char32_t)0xABBC,
    (char32_t)0xABBD,  (char32_t)0xABBE,  (char32_t)0xABBF,  (char32_t)0xFF21,  (char32_t)0xFF22,
    (char32_t)0xFF23,  (char32_t)0xFF24,  (char32_t)0xFF25,  (char32_t)0xFF26,  (char32_t)0xFF27,
    (char32_t)0xFF28,  (char32_t)0xFF29,  (char32_t)0xFF2A,  (char32_t)0xFF2B,  (char32_t)0xFF2C,
    (char32_t)0xFF2D,  (char32_t)0xFF2E,  (char32_t)0xFF2F,  (char32_t)0xFF30,  (char32_t)0xFF31,
    (char32_t)0xFF32,  (char32_t)0xFF33,  (char32_t)0xFF34,  (char32_t)0xFF35,  (char32_t)0xFF36,
    (char32_t)0xFF37,  (char32_t)0xFF38,  (char32_t)0xFF39,  (char32_t)0xFF3A,  (char32_t)0x10400,
    (char32_t)0x10401, (char32_t)0x10402, (char32_t)0x10403, (char32_t)0x10404, (char32_t)0x10405,
    (char32_t)0x10406, (char32_t)0x10407, (char32_t)0x10408, (char32_t)0x10409, (char32_t)0x1040A,
    (char32_t)0x1040B, (char32_t)0x1040C, (char32_t)0x1040D, (char32_t)0x1040E, (char32_t)0x1040F,
    (char32_t)0x10410, (char32_t)0x10411, (char32_t)0x10412, (char32_t)0x10413, (char32_t)0x10414,
    (char32_t)0x10415, (char32_t)0x10416, (char32_t)0x10417, (char32_t)0x10418, (char32_t)0x10419,
    (char32_t)0x1041A, (char32_t)0x1041B, (char32_t)0x1041C, (char32_t)0x1041D, (char32_t)0x1041E,
    (char32_t)0x1041F, (char32_t)0x10420, (char32_t)0x10421, (char32_t)0x10422, (char32_t)0x10423,
    (char32_t)0x10424, (char32_t)0x10425, (char32_t)0x10426, (char32_t)0x10427, (char32_t)0x104B0,
    (char32_t)0x104B1, (char32_t)0x104B2, (char32_t)0x104B3, (char32_t)0x104B4, (char32_t)0x104B5,
    (char32_t)0x104B6, (char32_t)0x104B7, (char32_t)0x104B8, (char32_t)0x104B9, (char32_t)0x104BA,
    (char32_t)0x104BB, (char32_t)0x104BC, (char32_t)0x104BD, (char32_t)0x104BE, (char32_t)0x104BF,
    (char32_t)0x104C0, (char32_t)0x104C1, (char32_t)0x104C2, (char32_t)0x104C3, (char32_t)0x104C4,
    (char32_t)0x104C5, (char32_t)0x104C6, (char32_t)0x104C7, (char32_t)0x104C8, (char32_t)0x104C9,
    (char32_t)0x104CA, (char32_t)0x104CB, (char32_t)0x104CC, (char32_t)0x104CD, (char32_t)0x104CE,
    (char32_t)0x104CF, (char32_t)0x104D0, (char32_t)0x104D1, (char32_t)0x104D2, (char32_t)0x104D3,
    (char32_t)0x10570, (char32_t)0x10571, (char32_t)0x10572, (char32_t)0x10573, (char32_t)0x10574,
    (char32_t)0x10575, (char32_t)0x10576, (char32_t)0x10577, (char32_t)0x10578, (char32_t)0x10579,
    (char32_t)0x1057A, (char32_t)0x1057C, (char32_t)0x1057D, (char32_t)0x1057E, (char32_t)0x1057F,
    (char32_t)0x10580, (char32_t)0x10581, (char32_t)0x10582, (char32_t)0x10583, (char32_t)0x10584,
    (char32_t)0x10585, (char32_t)0x10586, (char32_t)0x10587, (char32_t)0x10588, (char32_t)0x10589,
    (char32_t)0x1058A, (char32_t)0x1058C, (char32_t)0x1058D, (char32_t)0x1058E, (char32_t)0x1058F,
    (char32_t)0x10590, (char32_t)0x10591, (char32_t)0x10592, (char32_t)0x10594, (char32_t)0x10595,
    (char32_t)0x10C80, (char32_t)0x10C81, (char32_t)0x10C82, (char32_t)0x10C83, (char32_t)0x10C84,
    (char32_t)0x10C85, (char32_t)0x10C86, (char32_t)0x10C87, (char32_t)0x10C88, (char32_t)0x10C89,
    (char32_t)0x10C8A, (char32_t)0x10C8B, (char32_t)0x10C8C, (char32_t)0x10C8D, (char32_t)0x10C8E,
    (char32_t)0x10C8F, (char32_t)0x10C90, (char32_t)0x10C91, (char32_t)0x10C92, (char32_t)0x10C93,
    (char32_t)0x10C94, (char32_t)0x10C95, (char32_t)0x10C96, (char32_t)0x10C97, (char32_t)0x10C98,
    (char32_t)0x10C99, (char32_t)0x10C9A, (char32_t)0x10C9B, (char32_t)0x10C9C, (char32_t)0x10C9D,
    (char32_t)0x10C9E, (char32_t)0x10C9F, (char32_t)0x10CA0, (char32_t)0x10CA1, (char32_t)0x10CA2,
    (char32_t)0x10CA3, (char32_t)0x10CA4, (char32_t)0x10CA5, (char32_t)0x10CA6, (char32_t)0x10CA7,
    (char32_t)0x10CA8, (char32_t)0x10CA9, (char32_t)0x10CAA, (char32_t)0x10CAB, (char32_t)0x10CAC,
    (char32_t)0x10CAD, (char32_t)0x10CAE, (char32_t)0x10CAF, (char32_t)0x10CB0, (char32_t)0x10CB1,
    (char32_t)0x10CB2, (char32_t)0x118A0, (char32_t)0x118A1, (char32_t)0x118A2, (char32_t)0x118A3,
    (char32_t)0x118A4, (char32_t)0x118A5, (char32_t)0x118A6, (char32_t)0x118A7, (char32_t)0x118A8,
    (char32_t)0x118A9, (char32_t)0x118AA, (char32_t)0x118AB, (char32_t)0x118AC, (char32_t)0x118AD,
    (char32_t)0x118AE, (char32_t)0x118AF, (char32_t)0x118B0, (char32_t)0x118B1, (char32_t)0x118B2,
    (char32_t)0x118B3, (char32_t)0x118B4, (char32_t)0x118B5, (char32_t)0x118B6, (char32_t)0x118B7,
    (char32_t)0x118B8, (char32_t)0x118B9, (char32_t)0x118BA, (char32_t)0x118BB, (char32_t)0x118BC,
    (char32_t)0x118BD, (char32_t)0x118BE, (char32_t)0x118BF, (char32_t)0x16E40, (char32_t)0x16E41,
    (char32_t)0x16E42, (char32_t)0x16E43, (char32_t)0x16E44, (char32_t)0x16E45, (char32_t)0x16E46,
    (char32_t)0x16E47, (char32_t)0x16E48, (char32_t)0x16E49, (char32_t)0x16E4A, (char32_t)0x16E4B,
    (char32_t)0x16E4C, (char32_t)0x16E4D, (char32_t)0x16E4E, (char32_t)0x16E4F, (char32_t)0x16E50,
    (char32_t)0x16E51, (char32_t)0x16E52, (char32_t)0x16E53, (char32_t)0x16E54, (char32_t)0x16E55,
    (char32_t)0x16E56, (char32_t)0x16E57, (char32_t)0x16E58, (char32_t)0x16E59, (char32_t)0x16E5A,
    (char32_t)0x16E5B, (char32_t)0x16E5C, (char32_t)0x16E5D, (char32_t)0x16E5E, (char32_t)0x16E5F,
    (char32_t)0x1E900, (char32_t)0x1E901, (char32_t)0x1E902, (char32_t)0x1E903, (char32_t)0x1E904,
    (char32_t)0x1E905, (char32_t)0x1E906, (char32_t)0x1E907, (char32_t)0x1E908, (char32_t)0x1E909,
    (char32_t)0x1E90A, (char32_t)0x1E90B, (char32_t)0x1E90C, (char32_t)0x1E90D, (char32_t)0x1E90E,
    (char32_t)0x1E90F, (char32_t)0x1E910, (char32_t)0x1E911, (char32_t)0x1E912, (char32_t)0x1E913,
    (char32_t)0x1E914, (char32_t)0x1E915, (char32_t)0x1E916, (char32_t)0x1E917, (char32_t)0x1E918,
    (char32_t)0x1E919, (char32_t)0x1E91A, (char32_t)0x1E91B, (char32_t)0x1E91C, (char32_t)0x1E91D,
    (char32_t)0x1E91E, (char32_t)0x1E91F, (char32_t)0x1E920, (char32_t)0x1E921};

static const char32_t unicode_fold_lower[] = {
    (char32_t)0x0061,  (char32_t)0x0062,  (char32_t)0x0063,  (char32_t)0x0064,  (char32_t)0x0065,
    (char32_t)0x0066,  (char32_t)0x0067,  (char32_t)0x0068,  (char32_t)0x0069,  (char32_t)0x006A,
    (char32_t)0x006B,  (char32_t)0x006C,  (char32_t)0x006D,  (char32_t)0x006E,  (char32_t)0x006F,
    (char32_t)0x0070,  (char32_t)0x0071,  (char32_t)0x0072,  (char32_t)0x0073,  (char32_t)0x0074,
    (char32_t)0x0075,  (char32_t)0x0076,  (char32_t)0x0077,  (char32_t)0x0078,  (char32_t)0x0079,
    (char32_t)0x007A,  (char32_t)0x03BC,  (char32_t)0x00E0,  (char32_t)0x00E1,  (char32_t)0x00E2,
    (char32_t)0x00E3,  (char32_t)0x00E4,  (char32_t)0x00E5,  (char32_t)0x00E6,  (char32_t)0x00E7,
    (char32_t)0x00E8,  (char32_t)0x00E9,  (char32_t)0x00EA,  (char32_t)0x00EB,  (char32_t)0x00EC,
    (char32_t)0x00ED,  (char32_t)0x00EE,  (char32_t)0x00EF,  (char32_t)0x00F0,  (char32_t)0x00F1,
    (char32_t)0x00F2,  (char32_t)0x00F3,  (char32_t)0x00F4,  (char32_t)0x00F5,  (char32_t)0x00F6,
    (char32_t)0x00F8,  (char32_t)0x00F9,  (char32_t)0x00FA,  (char32_t)0x00FB,  (char32_t)0x00FC,
    (char32_t)0x00FD,  (char32_t)0x00FE,  (char32_t)0x0101,  (char32_t)0x0103,  (char32_t)0x0105,
    (char32_t)0x0107,  (char32_t)0x0109,  (char32_t)0x010B,  (char32_t)0x010D,  (char32_t)0x010F,
    (char32_t)0x0111,  (char32_t)0x0113,  (char32_t)0x0115,  (char32_t)0x0117,  (char32_t)0x0119,
    (char32_t)0x011B,  (char32_t)0x011D,  (char32_t)0x011F,  (char32_t)0x0121,  (char32_t)0x0123,
    (char32_t)0x0125,  (char32_t)0x0127,  (char32_t)0x0129,  (char32_t)0x012B,  (char32_t)0x012D,
    (char32_t)0x012F,  (char32_t)0x0133,  (char32_t)0x0135,  (char32_t)0x0137,  (char32_t)0x013A,
    (char32_t)0x013C,  (char32_t)0x013E,  (char32_t)0x0140,  (char32_t)0x0142,  (char32_t)0x0144,
    (char32_t)0x0146,  (char32_t)0x0148,  (char32_t)0x014B,  (char32_t)0x014D,  (char32_t)0x014F,
    (char32_t)0x0151,  (char32_t)0x0153,  (char32_t)0x0155,  (char32_t)0x0157,  (char32_t)0x0159,
    (char32_t)0x015B,  (char32_t)0x015D,  (char32_t)0x015F,  (char32_t)0x0161,  (char32_t)0x0163,
    (char32_t)0x0165,  (char32_t)0x0167,  (char32_t)0x0169,  (char32_t)0x016B,  (char32_t)0x016D,
    (char32_t)0x016F,  (char32_t)0x0171,  (char32_t)0x0173,  (char32_t)0x0175,  (char32_t)0x0177,
    (char32_t)0x00FF,  (char32_t)0x017A,  (char32_t)0x017C,  (char32_t)0x017E,  (char32_t)0x0073,
    (char32_t)0x0253,  (char32_t)0x0183,  (char32_t)0x0185,  (char32_t)0x0254,  (char32_t)0x0188,
    (char32_t)0x0256,  (char32_t)0x0257,  (char32_t)0x018C,  (char32_t)0x01DD,  (char32_t)0x0259,
    (char32_t)0x025B,  (char32_t)0x0192,  (char32_t)0x0260,  (char32_t)0x0263,  (char32_t)0x0269,
    (char32_t)0x0268,  (char32_t)0x0199,  (char32_t)0x026F,  (char32_t)0x0272,  (char32_t)0x0275,
    (char32_t)0x01A1,  (char32_t)0x01A3,  (char32_t)0x01A5,  (char32_t)0x0280,  (char32_t)0x01A8,
    (char32_t)0x0283,  (char32_t)0x01AD,  (char32_t)0x0288,  (char32_t)0x01B0,  (char32_t)0x028A,
    (char32_t)0x028B,  (char32_t)0x01B4,  (char32_t)0x01B6,  (char32_t)0x0292,  (char32_t)0x01B9,
    (char32_t)0x01BD,  (char32_t)0x01C6,  (char32_t)0x01C6,  (char32_t)0x01C9,  (char32_t)0x01C9,
    (char32_t)0x01CC,  (char32_t)0x01CC,  (char32_t)0x01CE,  (char32_t)0x01D0,  (char32_t)0x01D2,
    (char32_t)0x01D4,  (char32_t)0x01D6,  (char32_t)0x01D8,  (char32_t)0x01DA,  (char32_t)0x01DC,
    (char32_t)0x01DF,  (char32_t)0x01E1,  (char32_t)0x01E3,  (char32_t)0x01E5,  (char32_t)0x01E7,
    (char32_t)0x01E9,  (char32_t)0x01EB,  (char32_t)0x01ED,  (char32_t)0x01EF,  (char32_t)0x01F3,
    (char32_t)0x01F3,  (char32_t)0x01F5,  (char32_t)0x0195,  (char32_t)0x01BF,  (char32_t)0x01F9,
    (char32_t)0x01FB,  (char32_t)0x01FD,  (char32_t)0x01FF,  (char32_t)0x0201,  (char32_t)0x0203,
    (char32_t)0x0205,  (char32_t)0x0207,  (char32_t)0x0209,  (char32_t)0x020B,  (char32_t)0x020D,
    (char32_t)0x020F,  (char32_t)0x0211,  (char32_t)0x0213,  (char32_t)0x0215,  (char32_t)0x0217,
    (char32_t)0x0219,  (char32_t)0x021B,  (char32_t)0x021D,  (char32_t)0x021F,  (char32_t)0x019E,
    (char32_t)0x0223,  (char32_t)0x0225,  (char32_t)0x0227,  (char32_t)0x0229,  (char32_t)0x022B,
    (char32_t)0x022D,  (char32_t)0x022F,  (char32_t)0x0231,  (char32_t)0x0233,  (char32_t)0x2C65,
    (char32_t)0x023C,  (char32_t)0x019A,  (char32_t)0x2C66,  (char32_t)0x0242,  (char32_t)0x0180,
    (char32_t)0x0289,  (char32_t)0x028C,  (char32_t)0x0247,  (char32_t)0x0249,  (char32_t)0x024B,
    (char32_t)0x024D,  (char32_t)0x024F,  (char32_t)0x03B9,  (char32_t)0x0371,  (char32_t)0x0373,
    (char32_t)0x0377,  (char32_t)0x03F3,  (char32_t)0x03AC,  (char32_t)0x03AD,  (char32_t)0x03AE,
    (char32_t)0x03AF,  (char32_t)0x03CC,  (char32_t)0x03CD,  (char32_t)0x03CE,  (char32_t)0x03B1,
    (char32_t)0x03B2,  (char32_t)0x03B3,  (char32_t)0x03B4,  (char32_t)0x03B5,  (char32_t)0x03B6,
    (char32_t)0x03B7,  (char32_t)0x03B8,  (char32_t)0x03B9,  (char32_t)0x03BA,  (char32_t)0x03BB,
    (char32_t)0x03BC,  (char32_t)0x03BD,  (char32_t)0x03BE,  (char32_t)0x03BF,  (char32_t)0x03C0,
    (char32_t)0x03C1,  (char32_t)0x03C3,  (char32_t)0x03C4,  (char32_t)0x03C5,  (char32_t)0x03C6,
    (char32_t)0x03C7,  (char32_t)0x03C8,  (char32_t)0x03C9,  (char32_t)0x03CA,  (char32_t)0x03CB,
    (char32_t)0x03C3,  (char32_t)0x03D7,  (char32_t)0x03B2,  (char32_t)0x03B8,  (char32_t)0x03C6,
    (char32_t)0x03C0,  (char32_t)0x03D9,  (char32_t)0x03DB,  (char32_t)0x03DD,  (char32_t)0x03DF,
    (char32_t)0x03E1,  (char32_t)0x03E3,  (char32_t)0x03E5,  (char32_t)0x03E7,  (char32_t)0x03E9,
    (char32_t)0x03EB,  (char32_t)0x03ED,  (char32_t)0x03EF,  (char32_t)0x03BA,  (char32_t)0x03C1,
    (char32_t)0x03B8,  (char32_t)0x03B5,  (char32_t)0x03F8,  (char32_t)0x03F2,  (char32_t)0x03FB,
    (char32_t)0x037B,  (char32_t)0x037C,  (char32_t)0x037D,  (char32_t)0x0450,  (char32_t)0x0451,
    (char32_t)0x0452,  (char32_t)0x0453,  (char32_t)0x0454,  (char32_t)0x0455,  (char32_t)0x0456,
    (char32_t)0x0457,  (char32_t)0x0458,  (char32_t)0x0459,  (char32_t)0x045A,  (char32_t)0x045B,
    (char32_t)0x045C,  (char32_t)0x045D,  (char32_t)0x045E,  (char32_t)0x045F,  (char32_t)0x0430,
    (char32_t)0x0431,  (char32_t)0x0432,  (char32_t)0x0433,  (char32_t)0x0434,  (char32_t)0x0435,
    (char32_t)0x0436,  (char32_t)0x0437,  (char32_t)0x0438,  (char32_t)0x0439,  (char32_t)0x043A,
    (char32_t)0x043B,  (char32_t)0x043C,  (char32_t)0x043D,  (char32_t)0x043E,  (char32_t)0x043F,
    (char32_t)0x0440,  (char32_t)0x0441,  (char32_t)0x0442,  (char32_t)0x0443,  (char32_t)0x0444,
    (char32_t)0x0445,  (char32_t)0x0446,  (char32_t)0x0447,  (char32_t)0x0448,  (char32_t)0x0449,
    (char32_t)0x044A,  (char32_t)0x044B,  (char32_t)0x044C,  (char32_t)0x044D,  (char32_t)0x044E,
    (char32_t)0x044F,  (char32_t)0x0461,  (char32_t)0x0463,  (char32_t)0x0465,  (char32_t)0x0467,
    (char32_t)0x0469,  (char32_t)0x046B,  (char32_t)0x046D,  (char32_t)0x046F,  (char32_t)0x0471,
    (char32_t)0x0473,  (char32_t)0x0475,  (char32_t)0x0477,  (char32_t)0x0479,  (char32_t)0x047B,
    (char32_t)0x047D,  (char32_t)0x047F,  (char32_t)0x0481,  (char32_t)0x048B,  (char32_t)0x048D,
    (char32_t)0x048F,  (char32_t)0x0491,  (char32_t)0x0493,  (char32_t)0x0495,  (char32_t)0x0497,
    (char32_t)0x0499,  (char32_t)0x049B,  (char32_t)0x049D,  (char32_t)0x049F,  (char32_t)0x04A1,
    (char32_t)0x04A3,  (char32_t)0x04A5,  (char32_t)0x04A7,  (char32_t)0x04A9,  (char32_t)0x04AB,
    (char32_t)0x04AD,  (char32_t)0x04AF,  (char32_t)0x04B1,  (char32_t)0x04B3,  (char32_t)0x04B5,
    (char32_t)0x04B7,  (char32_t)0x04B9,  (char32_t)0x04BB,  (char32_t)0x04BD,  (char32_t)0x04BF,
    (char32_t)0x04CF,  (char32_t)0x04C2,  (char32_t)0x04C4,  (char32_t)0x04C6,  (char32_t)0x04C8,
    (char32_t)0x04CA,  (char32_t)0x04CC,  (char32_t)0x04CE,  (char32_t)0x04D1,  (char32_t)0x04D3,
    (char32_t)0x04D5,  (char32_t)0x04D7,  (char32_t)0x04D9,  (char32_t)0x04DB,  (char32_t)0x04DD,
    (char32_t)0x04DF,  (char32_t)0x04E1,  (char32_t)0x04E3,  (char32_t)0x04E5,  (char32_t)0x04E7,
    (char32_t)0x04E9,  (char32_t)0x04EB,  (char32_t)0x04ED,  (char32_t)0x04EF,  (char32_t)0x04F1,
    (char32_t)0x04F3,  (char32_t)0x04F5,  (char32_t)0x04F7,  (char32_t)0x04F9,  (char32_t)0x04FB,
    (char32_t)0x04FD,  (char32_t)0x04FF,  (char32_t)0x0501,  (char32_t)0x0503,  (char32_t)0x0505,
    (char32_t)0x0507,  (char32_t)0x0509,  (char32_t)0x050B,  (char32_t)0x050D,  (char32_t)0x050F,
    (char32_t)0x0511,  (char32_t)0x0513,  (char32_t)0x0515,  (char32_t)0x0517,  (char32_t)0x0519,
    (char32_t)0x051B,  (char32_t)0x051D,  (char32_t)0x051F,  (char32_t)0x0521,  (char32_t)0x0523,
    (char32_t)0x0525,  (char32_t)0x0527,  (char32_t)0x0529,  (char32_t)0x052B,  (char32_t)0x052D,
    (char32_t)0x052F,  (char32_t)0x0561,  (char32_t)0x0562,  (char32_t)0x0563,  (char32_t)0x0564,
    (char32_t)0x0565,  (char32_t)0x0566,  (char32_t)0x0567,  (char32_t)0x0568,  (char32_t)0x0569,
    (char32_t)0x056A,  (char32_t)0x056B,  (char32_t)0x056C,  (char32_t)0x056D,  (char32_t)0x056E,
    (char32_t)0x056F,  (char32_t)0x0570,  (char32_t)0x0571,  (char32_t)0x0572,  (char32_t)0x0573,
    (char32_t)0x0574,  (char32_t)0x0575,  (char32_t)0x0576,  (char32_t)0x0577,  (char32_t)0x0578,
    (char32_t)0x0579,  (char32_t)0x057A,  (char32_t)0x057B,  (char32_t)0x057C,  (char32_t)0x057D,
    (char32_t)0x057E,  (char32_t)0x057F,  (char32_t)0x0580,  (char32_t)0x0581,  (char32_t)0x0582,
    (char32_t)0x0583,  (char32_t)0x0584,  (char32_t)0x0585,  (char32_t)0x0586,  (char32_t)0x2D00,
    (char32_t)0x2D01,  (char32_t)0x2D02,  (char32_t)0x2D03,  (char32_t)0x2D04,  (char32_t)0x2D05,
    (char32_t)0x2D06,  (char32_t)0x2D07,  (char32_t)0x2D08,  (char32_t)0x2D09,  (char32_t)0x2D0A,
    (char32_t)0x2D0B,  (char32_t)0x2D0C,  (char32_t)0x2D0D,  (char32_t)0x2D0E,  (char32_t)0x2D0F,
    (char32_t)0x2D10,  (char32_t)0x2D11,  (char32_t)0x2D12,  (char32_t)0x2D13,  (char32_t)0x2D14,
    (char32_t)0x2D15,  (char32_t)0x2D16,  (char32_t)0x2D17,  (char32_t)0x2D18,  (char32_t)0x2D19,
    (char32_t)0x2D1A,  (char32_t)0x2D1B,  (char32_t)0x2D1C,  (char32_t)0x2D1D,  (char32_t)0x2D1E,
    (char32_t)0x2D1F,  (char32_t)0x2D20,  (char32_t)0x2D21,  (char32_t)0x2D22,  (char32_t)0x2D23,
    (char32_t)0x2D24,  (char32_t)0x2D25,  (char32_t)0x2D27,  (char32_t)0x2D2D,  (char32_t)0x13F0,
    (char32_t)0x13F1,  (char32_t)0x13F2,  (char32_t)0x13F3,  (char32_t)0x13F4,  (char32_t)0x13F5,
    (char32_t)0x0432,  (char32_t)0x0434,  (char32_t)0x043E,  (char32_t)0x0441,  (char32_t)0x0442,
    (char32_t)0x0442,  (char32_t)0x044A,  (char32_t)0x0463,  (char32_t)0xA64B,  (char32_t)0x10D0,
    (char32_t)0x10D1,  (char32_t)0x10D2,  (char32_t)0x10D3,  (char32_t)0x10D4,  (char32_t)0x10D5,
    (char32_t)0x10D6,  (char32_t)0x10D7,  (char32_t)0x10D8,  (char32_t)0x10D9,  (char32_t)0x10DA,
    (char32_t)0x10DB,  (char32_t)0x10DC,  (char32_t)0x10DD,  (char32_t)0x10DE,  (char32_t)0x10DF,
    (char32_t)0x10E0,  (char32_t)0x10E1,  (char32_t)0x10E2,  (char32_t)0x10E3,  (char32_t)0x10E4,
    (char32_t)0x10E5,  (char32_t)0x10E6,  (char32_t)0x10E7,  (char32_t)0x10E8,  (char32_t)0x10E9,
    (char32_t)0x10EA,  (char32_t)0x10EB,  (char32_t)0x10EC,  (char32_t)0x10ED,  (char32_t)0x10EE,
    (char32_t)0x10EF,  (char32_t)0x10F0,  (char32_t)0x10F1,  (char32_t)0x10F2,  (char32_t)0x10F3,
    (char32_t)0x10F4,  (char32_t)0x10F5,  (char32_t)0x10F6,  (char32_t)0x10F7,  (char32_t)0x10F8,
    (char32_t)0x10F9,  (char32_t)0x10FA,  (char32_t)0x10FD,  (char32_t)0x10FE,  (char32_t)0x10FF,
    (char32_t)0x1E01,  (char32_t)0x1E03,  (char32_t)0x1E05,  (char32_t)0x1E07,  (char32_t)0x1E09,
    (char32_t)0x1E0B,  (char32_t)0x1E0D,  (char32_t)0x1E0F,  (char32_t)0x1E11,  (char32_t)0x1E13,
    (char32_t)0x1E15,  (char32_t)0x1E17,  (char32_t)0x1E19,  (char32_t)0x1E1B,  (char32_t)0x1E1D,
    (char32_t)0x1E1F,  (char32_t)0x1E21,  (char32_t)0x1E23,  (char32_t)0x1E25,  (char32_t)0x1E27,
    (char32_t)0x1E29,  (char32_t)0x1E2B,  (char32_t)0x1E2D,  (char32_t)0x1E2F,  (char32_t)0x1E31,
    (char32_t)0x1E33,  (char32_t)0x1E35,  (char32_t)0x1E37,  (char32_t)0x1E39,  (char32_t)0x1E3B,
    (char32_t)0x1E3D,  (char32_t)0x1E3F,  (char32_t)0x1E41,  (char32_t)0x1E43,  (char32_t)0x1E45,
    (char32_t)0x1E47,  (char32_t)0x1E49,  (char32_t)0x1E4B,  (char32_t)0x1E4D,  (char32_t)0x1E4F,
    (char32_t)0x1E51,  (char32_t)0x1E53,  (char32_t)0x1E55,  (char32_t)0x1E57,  (char32_t)0x1E59,
    (char32_t)0x1E5B,  (char32_t)0x1E5D,  (char32_t)0x1E5F,  (char32_t)0x1E61,  (char32_t)0x1E63,
    (char32_t)0x1E65,  (char32_t)0x1E67,  (char32_t)0x1E69,  (char32_t)0x1E6B,  (char32_t)0x1E6D,
    (char32_t)0x1E6F,  (char32_t)0x1E71,  (char32_t)0x1E73,  (char32_t)0x1E75,  (char32_t)0x1E77,
    (char32_t)0x1E79,  (char32_t)0x1E7B,  (char32_t)0x1E7D,  (char32_t)0x1E7F,  (char32_t)0x1E81,
    (char32_t)0x1E83,  (char32_t)0x1E85,  (char32_t)0x1E87,  (char32_t)0x1E89,  (char32_t)0x1E8B,
    (char32_t)0x1E8D,  (char32_t)0x1E8F,  (char32_t)0x1E91,  (char32_t)0x1E93,  (char32_t)0x1E95,
    (char32_t)0x1E61,  (char32_t)0x00DF,  (char32_t)0x1EA1,  (char32_t)0x1EA3,  (char32_t)0x1EA5,
    (char32_t)0x1EA7,  (char32_t)0x1EA9,  (char32_t)0x1EAB,  (char32_t)0x1EAD,  (char32_t)0x1EAF,
    (char32_t)0x1EB1,  (char32_t)0x1EB3,  (char32_t)0x1EB5,  (char32_t)0x1EB7,  (char32_t)0x1EB9,
    (char32_t)0x1EBB,  (char32_t)0x1EBD,  (char32_t)0x1EBF,  (char32_t)0x1EC1,  (char32_t)0x1EC3,
    (char32_t)0x1EC5,  (char32_t)0x1EC7,  (char32_t)0x1EC9,  (char32_t)0x1ECB,  (char32_t)0x1ECD,
    (char32_t)0x1ECF,  (char32_t)0x1ED1,  (char32_t)0x1ED3,  (char32_t)0x1ED5,  (char32_t)0x1ED7,
    (char32_t)0x1ED9,  (char32_t)0x1EDB,  (char32_t)0x1EDD,  (char32_t)0x1EDF,  (char32_t)0x1EE1,
    (char32_t)0x1EE3,  (char32_t)0x1EE5,  (char32_t)0x1EE7,  (char32_t)0x1EE9,  (char32_t)0x1EEB,
    (char32_t)0x1EED,  (char32_t)0x1EEF,  (char32_t)0x1EF1,  (char32_t)0x1EF3,  (char32_t)0x1EF5,
    (char32_t)0x1EF7,  (char32_t)0x1EF9,  (char32_t)0x1EFB,  (char32_t)0x1EFD,  (char32_t)0x1EFF,
    (char32_t)0x1F00,  (char32_t)0x1F01,  (char32_t)0x1F02,  (char32_t)0x1F03,  (char32_t)0x1F04,
    (char32_t)0x1F05,  (char32_t)0x1F06,  (char32_t)0x1F07,  (char32_t)0x1F10,  (char32_t)0x1F11,
    (char32_t)0x1F12,  (char32_t)0x1F13,  (char32_t)0x1F14,  (char32_t)0x1F15,  (char32_t)0x1F20,
    (char32_t)0x1F21,  (char32_t)0x1F22,  (char32_t)0x1F23,  (char32_t)0x1F24,  (char32_t)0x1F25,
    (char32_t)0x1F26,  (char32_t)0x1F27,  (char32_t)0x1F30,  (char32_t)0x1F31,  (char32_t)0x1F32,
    (char32_t)0x1F33,  (char32_t)0x1F34,  (char32_t)0x1F35,  (char32_t)0x1F36,  (char32_t)0x1F37,
    (char32_t)0x1F40,  (char32_t)0x1F41,  (char32_t)0x1F42,  (char32_t)0x1F43,  (char32_t)0x1F44,
    (char32_t)0x1F45,  (char32_t)0x1F51,  (char32_t)0x1F53,  (char32_t)0x1F55,  (char32_t)0x1F57,
    (char32_t)0x1F60,  (char32_t)0x1F61,  (char32_t)0x1F62,  (char32_t)0x1F63,  (char32_t)0x1F64,
    (char32_t)0x1F65,  (char32_t)0x1F66,  (char32_t)0x1F67,  (char32_t)0x1F80,  (char32_t)0x1F81,
    (char32_t)0x1F82,  (char32_t)0x1F83,  (char32_t)0x1F84,  (char32_t)0x1F85,  (char32_t)0x1F86,
    (char32_t)0x1F87,  (char32_t)0x1F90,  (char32_t)0x1F91,  (char32_t)0x1F92,  (char32_t)0x1F93,
    (char32_t)0x1F94,  (char32_t)0x1F95,  (char32_t)0x1F96,  (char32_t)0x1F97,  (char32_t)0x1FA0,
    (char32_t)0x1FA1,  (char32_t)0x1FA2,  (char32_t)0x1FA3,  (char32_t)0x1FA4,  (char32_t)0x1FA5,
    (char32_t)0x1FA6,  (char32_t)0x1FA7,  (char32_t)0x1FB0,  (char32_t)0x1FB1,  (char32_t)0x1F70,
    (char32_t)0x1F71,  (char32_t)0x1FB3,  (char32_t)0x03B9,  (char32_t)0x1F72,  (char32_t)0x1F73,
    (char32_t)0x1F74,  (char32_t)0x1F75,  (char32_t)0x1FC3,  (char32_t)0x1FD0,  (char32_t)0x1FD1,
    (char32_t)0x1F76,  (char32_t)0x1F77,  (char32_t)0x1FE0,  (char32_t)0x1FE1,  (char32_t)0x1F7A,
    (char32_t)0x1F7B,  (char32_t)0x1FE5,  (char32_t)0x1F78,  (char32_t)0x1F79,  (char32_t)0x1F7C,
    (char32_t)0x1F7D,  (char32_t)0x1FF3,  (char32_t)0x03C9,  (char32_t)0x006B,  (char32_t)0x00E5,
    (char32_t)0x214E,  (char32_t)0x2170,  (char32_t)0x2171,  (char32_t)0x2172,  (char32_t)0x2173,
    (char32_t)0x2174,  (char32_t)0x2175,  (char32_t)0x2176,  (char32_t)0x2177,  (char32_t)0x2178,
    (char32_t)0x2179,  (char32_t)0x217A,  (char32_t)0x217B,  (char32_t)0x217C,  (char32_t)0x217D,
    (char32_t)0x217E,  (char32_t)0x217F,  (char32_t)0x2184,  (char32_t)0x24D0,  (char32_t)0x24D1,
    (char32_t)0x24D2,  (char32_t)0x24D3,  (char32_t)0x24D4,  (char32_t)0x24D5,  (char32_t)0x24D6,
    (char32_t)0x24D7,  (char32_t)0x24D8,  (char32_t)0x24D9,  (char32_t)0x24DA,  (char32_t)0x24DB,
    (char32_t)0x24DC,  (char32_t)0x24DD,  (char32_t)0x24DE,  (char32_t)0x24DF,  (char32_t)0x24E0,
    (char32_t)0x24E1,  (char32_t)0x24E2,  (char32_t)0x24E3,  (char32_t)0x24E4,  (char32_t)0x24E5,
    (char32_t)0x24E6,  (char32_t)0x24E7,  (char32_t)0x24E8,  (char32_t)0x24E9,  (char32_t)0x2C30,
    (char32_t)0x2C31,  (char32_t)0x2C32,  (char32_t)0x2C33,  (char32_t)0x2C34,  (char32_t)0x2C35,
    (char32_t)0x2C36,  (char32_t)0x2C37,  (char32_t)0x2C38,  (char32_t)0x2C39,  (char32_t)0x2C3A,
    (char32_t)0x2C3B,  (char32_t)0x2C3C,  (char32_t)0x2C3D,  (char32_t)0x2C3E,  (char32_t)0x2C3F,
    (char32_t)0x2C40,  (char32_t)0x2C41,  (char32_t)0x2C42,  (char32_t)0x2C43,  (char32_t)0x2C44,
    (char32_t)0x2C45,  (char32_t)0x2C46,  (char32_t)0x2C47,  (char32_t)0x2C48,  (char32_t)0x2C49,
    (char32_t)0x2C4A,  (char32_t)0x2C4B,  (char32_t)0x2C4C,  (char32_t)0x2C4D,  (char32_t)0x2C4E,
    (char32_t)0x2C4F,  (char32_t)0x2C50,  (char32_t)0x2C51,  (char32_t)0x2C52,  (char32_t)0x2C53,
    (char32_t)0x2C54,  (char32_t)0x2C55,  (char32_t)0x2C56,  (char32_t)0x2C57,  (char32_t)0x2C58,
    (char32_t)0x2C59,  (char32_t)0x2C5A,  (char32_t)0x2C5B,  (char32_t)0x2C5C,  (char32_t)0x2C5D,
    (char32_t)0x2C5E,  (char32_t)0x2C5F,  (char32_t)0x2C61,  (char32_t)0x026B,  (char32_t)0x1D7D,
    (char32_t)0x027D,  (char32_t)0x2C68,  (char32_t)0x2C6A,  (char32_t)0x2C6C,  (char32_t)0x0251,
    (char32_t)0x0271,  (char32_t)0x0250,  (char32_t)0x0252,  (char32_t)0x2C73,  (char32_t)0x2C76,
    (char32_t)0x023F,  (char32_t)0x0240,  (char32_t)0x2C81,  (char32_t)0x2C83,  (char32_t)0x2C85,
    (char32_t)0x2C87,  (char32_t)0x2C89,  (char32_t)0x2C8B,  (char32_t)0x2C8D,  (char32_t)0x2C8F,
    (char32_t)0x2C91,  (char32_t)0x2C93,  (char32_t)0x2C95,  (char32_t)0x2C97,  (char32_t)0x2C99,
    (char32_t)0x2C9B,  (char32_t)0x2C9D,  (char32_t)0x2C9F,  (char32_t)0x2CA1,  (char32_t)0x2CA3,
    (char32_t)0x2CA5,  (char32_t)0x2CA7,  (char32_t)0x2CA9,  (char32_t)0x2CAB,  (char32_t)0x2CAD,
    (char32_t)0x2CAF,  (char32_t)0x2CB1,  (char32_t)0x2CB3,  (char32_t)0x2CB5,  (char32_t)0x2CB7,
    (char32_t)0x2CB9,  (char32_t)0x2CBB,  (char32_t)0x2CBD,  (char32_t)0x2CBF,  (char32_t)0x2CC1,
    (char32_t)0x2CC3,  (char32_t)0x2CC5,  (char32_t)0x2CC7,  (char32_t)0x2CC9,  (char32_t)0x2CCB,
    (char32_t)0x2CCD,  (char32_t)0x2CCF,  (char32_t)0x2CD1,  (char32_t)0x2CD3,  (char32_t)0x2CD5,
    (char32_t)0x2CD7,  (char32_t)0x2CD9,  (char32_t)0x2CDB,  (char32_t)0x2CDD,  (char32_t)0x2CDF,
    (char32_t)0x2CE1,  (char32_t)0x2CE3,  (char32_t)0x2CEC,  (char32_t)0x2CEE,  (char32_t)0x2CF3,
    (char32_t)0xA641,  (char32_t)0xA643,  (char32_t)0xA645,  (char32_t)0xA647,  (char32_t)0xA649,
    (char32_t)0xA64B,  (char32_t)0xA64D,  (char32_t)0xA64F,  (char32_t)0xA651,  (char32_t)0xA653,
    (char32_t)0xA655,  (char32_t)0xA657,  (char32_t)0xA659,  (char32_t)0xA65B,  (char32_t)0xA65D,
    (char32_t)0xA65F,  (char32_t)0xA661,  (char32_t)0xA663,  (char32_t)0xA665,  (char32_t)0xA667,
    (char32_t)0xA669,  (char32_t)0xA66B,  (char32_t)0xA66D,  (char32_t)0xA681,  (char32_t)0xA683,
    (char32_t)0xA685,  (char32_t)0xA687,  (char32_t)0xA689,  (char32_t)0xA68B,  (char32_t)0xA68D,
    (char32_t)0xA68F,  (char32_t)0xA691,  (char32_t)0xA693,  (char32_t)0xA695,  (char32_t)0xA697,
    (char32_t)0xA699,  (char32_t)0xA69B,  (char32_t)0xA723,  (char32_t)0xA725,  (char32_t)0xA727,
    (char32_t)0xA729,  (char32_t)0xA72B,  (char32_t)0xA72D,  (char32_t)0xA72F,  (char32_t)0xA733,
    (char32_t)0xA735,  (char32_t)0xA737,  (char32_t)0xA739,  (char32_t)0xA73B,  (char32_t)0xA73D,
    (char32_t)0xA73F,  (char32_t)0xA741,  (char32_t)0xA743,  (char32_t)0xA745,  (char32_t)0xA747,
    (char32_t)0xA749,  (char32_t)0xA74B,  (char32_t)0xA74D,  (char32_t)0xA74F,  (char32_t)0xA751,
    (char32_t)0xA753,  (char32_t)0xA755,  (char32_t)0xA757,  (char32_t)0xA759,  (char32_t)0xA75B,
    (char32_t)0xA75D,  (char32_t)0xA75F,  (char32_t)0xA761,  (char32_t)0xA763,  (char32_t)0xA765,
    (char32_t)0xA767,  (char32_t)0xA769,  (char32_t)0xA76B,  (char32_t)0xA76D,  (char32_t)0xA76F,
    (char32_t)0xA77A,  (char32_t)0xA77C,  (char32_t)0x1D79,  (char32_t)0xA77F,  (char32_t)0xA781,
    (char32_t)0xA783,  (char32_t)0xA785,  (char32_t)0xA787,  (char32_t)0xA78C,  (char32_t)0x0265,
    (char32_t)0xA791,  (char32_t)0xA793,  (char32_t)0xA797,  (char32_t)0xA799,  (char32_t)0xA79B,
    (char32_t)0xA79D,  (char32_t)0xA79F,  (char32_t)0xA7A1,  (char32_t)0xA7A3,  (char32_t)0xA7A5,
    (char32_t)0xA7A7,  (char32_t)0xA7A9,  (char32_t)0x0266,  (char32_t)0x025C,  (char32_t)0x0261,
    (char32_t)0x026C,  (char32_t)0x026A,  (char32_t)0x029E,  (char32_t)0x0287,  (char32_t)0x029D,
    (char32_t)0xAB53,  (char32_t)0xA7B5,  (char32_t)0xA7B7,  (char32_t)0xA7B9,  (char32_t)0xA7BB,
    (char32_t)0xA7BD,  (char32_t)0xA7BF,  (char32_t)0xA7C1,  (char32_t)0xA7C3,  (char32_t)0xA794,
    (char32_t)0x0282,  (char32_t)0x1D8E,  (char32_t)0xA7C8,  (char32_t)0xA7CA,  (char32_t)0xA7D1,
    (char32_t)0xA7D7,  (char32_t)0xA7D9,  (char32_t)0xA7F6,  (char32_t)0x13A0,  (char32_t)0x13A1,
    (char32_t)0x13A2,  (char32_t)0x13A3,  (char32_t)0x13A4,  (char32_t)0x13A5,  (char32_t)0x13A6,
    (char32_t)0x13A7,  (char32_t)0x13A8,  (char32_t)0x13A9,  (char32_t)0x13AA,  (char32_t)0x13AB,
    (char32_t)0x13AC,  (char32_t)0x13AD,  (char32_t)0x13AE,  (char32_t)0x13AF,  (char32_t)0x13B0,
    (char32_t)0x13B1,  (char32_t)0x13B2,  (char32_t)0x13B3,  (char32_t)0x13B4,  (char32_t)0x13B5,
    (char32_t)0x13B6,  (char32_t)0x13B7,  (char32_t)0x13B8,  (char32_t)0x13B9,  (char32_t)0x13BA,
    (char32_t)0x13BB,  (char32_t)0x13BC,  (char32_t)0x13BD,  (char32_t)0x13BE,  (char32_t)0x13BF,
    (char32_t)0x13C0,  (char32_t)0x13C1,  (char32_t)0x13C2,  (char32_t)0x13C3,  (char32_t)0x13C4,
    (char32_t)0x13C5,  (char32_t)0x13C6,  (char32_t)0x13C7,  (char32_t)0x13C8,  (char32_t)0x13C9,
    (char32_t)0x13CA,  (char32_t)0x13CB,  (char32_t)0x13CC,  (char32_t)0x13CD,  (char32_t)0x13CE,
    (char32_t)0x13CF,  (char32_t)0x13D0,  (char32_t)0x13D1,  (char32_t)0x13D2,  (char32_t)0x13D3,
    (char32_t)0x13D4,  (char32_t)0x13D5,  (char32_t)0x13D6,  (char32_t)0x13D7,  (char32_t)0x13D8,
    (char32_t)0x13D9,  (char32_t)0x13DA,  (char32_t)0x13DB,  (char32_t)0x13DC,  (char32_t)0x13DD,
    (char32_t)0x13DE,  (char32_t)0x13DF,  (char32_t)0x13E0,  (char32_t)0x13E1,  (char32_t)0x13E2,
    (char32_t)0x13E3,  (char32_t)0x13E4,  (char32_t)0x13E5,  (char32_t)0x13E6,  (char32_t)0x13E7,
    (char32_t)0x13E8,  (char32_t)0x13E9,  (char32_t)0x13EA,  (char32_t)0x13EB,  (char32_t)0x13EC,
    (char32_t)0x13ED,  (char32_t)0x13EE,  (char32_t)0x13EF,  (char32_t)0xFF41,  (char32_t)0xFF42,
    (char32_t)0xFF43,  (char32_t)0xFF44,  (char32_t)0xFF45,  (char32_t)0xFF46,  (char32_t)0xFF47,
    (char32_t)0xFF48,  (char32_t)0xFF49,  (char32_t)0xFF4A,  (char32_t)0xFF4B,  (char32_t)0xFF4C,
    (char32_t)0xFF4D,  (char32_t)0xFF4E,  (char32_t)0xFF4F,  (char32_t)0xFF50,  (char32_t)0xFF51,
    (char32_t)0xFF52,  (char32_t)0xFF53,  (char32_t)0xFF54,  (char32_t)0xFF55,  (char32_t)0xFF56,
    (char32_t)0xFF57,  (char32_t)0xFF58,  (char32_t)0xFF59,  (char32_t)0xFF5A,  (char32_t)0x10428,
    (char32_t)0x10429, (char32_t)0x1042A, (char32_t)0x1042B, (char32_t)0x1042C, (char32_t)0x1042D,
    (char32_t)0x1042E, (char32_t)0x1042F, (char32_t)0x10430, (char32_t)0x10431, (char32_t)0x10432,
    (char32_t)0x10433, (char32_t)0x10434, (char32_t)0x10435, (char32_t)0x10436, (char32_t)0x10437,
    (char32_t)0x10438, (char32_t)0x10439, (char32_t)0x1043A, (char32_t)0x1043B, (char32_t)0x1043C,
    (char32_t)0x1043D, (char32_t)0x1043E, (char32_t)0x1043F, (char32_t)0x10440, (char32_t)0x10441,
    (char32_t)0x10442, (char32_t)0x10443, (char32_t)0x10444, (char32_t)0x10445, (char32_t)0x10446,
    (char32_t)0x10447, (char32_t)0x10448, (char32_t)0x10449, (char32_t)0x1044A, (char32_t)0x1044B,
    (char32_t)0x1044C, (char32_t)0x1044D, (char32_t)0x1044E, (char32_t)0x1044F, (char32_t)0x104D8,
    (char32_t)0x104D9, (char32_t)0x104DA, (char32_t)0x104DB, (char32_t)0x104DC, (char32_t)0x104DD,
    (char32_t)0x104DE, (char32_t)0x104DF, (char32_t)0x104E0, (char32_t)0x104E1, (char32_t)0x104E2,
    (char32_t)0x104E3, (char32_t)0x104E4, (char32_t)0x104E5, (char32_t)0x104E6, (char32_t)0x104E7,
    (char32_t)0x104E8, (char32_t)0x104E9, (char32_t)0x104EA, (char32_t)0x104EB, (char32_t)0x104EC,
    (char32_t)0x104ED, (char32_t)0x104EE, (char32_t)0x104EF, (char32_t)0x104F0, (char32_t)0x104F1,
    (char32_t)0x104F2, (char32_t)0x104F3, (char32_t)0x104F4, (char32_t)0x104F5, (char32_t)0x104F6,
    (char32_t)0x104F7, (char32_t)0x104F8, (char32_t)0x104F9, (char32_t)0x104FA, (char32_t)0x104FB,
    (char32_t)0x10597, (char32_t)0x10598, (char32_t)0x10599, (char32_t)0x1059A, (char32_t)0x1059B,
    (char32_t)0x1059C, (char32_t)0x1059D, (char32_t)0x1059E, (char32_t)0x1059F, (char32_t)0x105A0,
    (char32_t)0x105A1, (char32_t)0x105A3, (char32_t)0x105A4, (char32_t)0x105A5, (char32_t)0x105A6,
    (char32_t)0x105A7, (char32_t)0x105A8, (char32_t)0x105A9, (char32_t)0x105AA, (char32_t)0x105AB,
    (char32_t)0x105AC, (char32_t)0x105AD, (char32_t)0x105AE, (char32_t)0x105AF, (char32_t)0x105B0,
    (char32_t)0x105B1, (char32_t)0x105B3, (char32_t)0x105B4, (char32_t)0x105B5, (char32_t)0x105B6,
    (char32_t)0x105B7, (char32_t)0x105B8, (char32_t)0x105B9, (char32_t)0x105BB, (char32_t)0x105BC,
    (char32_t)0x10CC0, (char32_t)0x10CC1, (char32_t)0x10CC2, (char32_t)0x10CC3, (char32_t)0x10CC4,
    (char32_t)0x10CC5, (char32_t)0x10CC6, (char32_t)0x10CC7, (char32_t)0x10CC8, (char32_t)0x10CC9,
    (char32_t)0x10CCA, (char32_t)0x10CCB, (char32_t)0x10CCC, (char32_t)0x10CCD, (char32_t)0x10CCE,
    (char32_t)0x10CCF, (char32_t)0x10CD0, (char32_t)0x10CD1, (char32_t)0x10CD2, (char32_t)0x10CD3,
    (char32_t)0x10CD4, (char32_t)0x10CD5, (char32_t)0x10CD6, (char32_t)0x10CD7, (char32_t)0x10CD8,
    (char32_t)0x10CD9, (char32_t)0x10CDA, (char32_t)0x10CDB, (char32_t)0x10CDC, (char32_t)0x10CDD,
    (char32_t)0x10CDE, (char32_t)0x10CDF, (char32_t)0x10CE0, (char32_t)0x10CE1, (char32_t)0x10CE2,
    (char32_t)0x10CE3, (char32_t)0x10CE4, (char32_t)0x10CE5, (char32_t)0x10CE6, (char32_t)0x10CE7,
    (char32_t)0x10CE8, (char32_t)0x10CE9, (char32_t)0x10CEA, (char32_t)0x10CEB, (char32_t)0x10CEC,
    (char32_t)0x10CED, (char32_t)0x10CEE, (char32_t)0x10CEF, (char32_t)0x10CF0, (char32_t)0x10CF1,
    (char32_t)0x10CF2, (char32_t)0x118C0, (char32_t)0x118C1, (char32_t)0x118C2, (char32_t)0x118C3,
    (char32_t)0x118C4, (char32_t)0x118C5, (char32_t)0x118C6, (char32_t)0x118C7, (char32_t)0x118C8,
    (char32_t)0x118C9, (char32_t)0x118CA, (char32_t)0x118CB, (char32_t)0x118CC, (char32_t)0x118CD,
    (char32_t)0x118CE, (char32_t)0x118CF, (char32_t)0x118D0, (char32_t)0x118D1, (char32_t)0x118D2,
    (char32_t)0x118D3, (char32_t)0x118D4, (char32_t)0x118D5, (char32_t)0x118D6, (char32_t)0x118D7,
    (char32_t)0x118D8, (char32_t)0x118D9, (char32_t)0x118DA, (char32_t)0x118DB, (char32_t)0x118DC,
    (char32_t)0x118DD, (char32_t)0x118DE, (char32_t)0x118DF, (char32_t)0x16E60, (char32_t)0x16E61,
    (char32_t)0x16E62, (char32_t)0x16E63, (char32_t)0x16E64, (char32_t)0x16E65, (char32_t)0x16E66,
    (char32_t)0x16E67, (char32_t)0x16E68, (char32_t)0x16E69, (char32_t)0x16E6A, (char32_t)0x16E6B,
    (char32_t)0x16E6C, (char32_t)0x16E6D, (char32_t)0x16E6E, (char32_t)0x16E6F, (char32_t)0x16E70,
    (char32_t)0x16E71, (char32_t)0x16E72, (char32_t)0x16E73, (char32_t)0x16E74, (char32_t)0x16E75,
    (char32_t)0x16E76, (char32_t)0x16E77, (char32_t)0x16E78, (char32_t)0x16E79, (char32_t)0x16E7A,
    (char32_t)0x16E7B, (char32_t)0x16E7C, (char32_t)0x16E7D, (char32_t)0x16E7E, (char32_t)0x16E7F,
    (char32_t)0x1E922, (char32_t)0x1E923, (char32_t)0x1E924, (char32_t)0x1E925, (char32_t)0x1E926,
    (char32_t)0x1E927, (char32_t)0x1E928, (char32_t)0x1E929, (char32_t)0x1E92A, (char32_t)0x1E92B,
    (char32_t)0x1E92C, (char32_t)0x1E92D, (char32_t)0x1E92E, (char32_t)0x1E92F, (char32_t)0x1E930,
    (char32_t)0x1E931, (char32_t)0x1E932, (char32_t)0x1E933, (char32_t)0x1E934, (char32_t)0x1E935,
    (char32_t)0x1E936, (char32_t)0x1E937, (char32_t)0x1E938, (char32_t)0x1E939, (char32_t)0x1E93A,
    (char32_t)0x1E93B, (char32_t)0x1E93C, (char32_t)0x1E93D, (char32_t)0x1E93E, (char32_t)0x1E93F,
    (char32_t)0x1E940, (char32_t)0x1E941, (char32_t)0x1E942, (char32_t)0x1E943};

std::string StringUtils::FormatV(const char *fmt, va_list args)
{
  if (!fmt || !fmt[0])
    return "";

  int size = FORMAT_BLOCK_SIZE;
  va_list argCopy;

  while (true)
  {
    char *cstr = reinterpret_cast<char*>(malloc(sizeof(char) * size));
    if (!cstr)
      return "";

    va_copy(argCopy, args);
    int nActual = vsnprintf(cstr, size, fmt, argCopy);
    va_end(argCopy);

    if (nActual > -1 && nActual < size) // We got a valid result
    {
      std::string str(cstr, nActual);
      free(cstr);
      return str;
    }
    free(cstr);
#ifndef TARGET_WINDOWS
    if (nActual > -1)                   // Exactly what we will need (glibc 2.1)
      size = nActual + 1;
    else                                // Let's try to double the size (glibc 2.0)
      size *= 2;
#else  // TARGET_WINDOWS
    va_copy(argCopy, args);
    size = _vscprintf(fmt, argCopy);
    va_end(argCopy);
    if (size < 0)
      return "";
    else
      size++; // increment for null-termination
#endif // TARGET_WINDOWS
  }

  return ""; // unreachable
}

std::wstring StringUtils::FormatV(const wchar_t *fmt, va_list args)
{
  if (!fmt || !fmt[0])
    return L"";

  int size = FORMAT_BLOCK_SIZE;
  va_list argCopy;

  while (true)
  {
    wchar_t *cstr = reinterpret_cast<wchar_t*>(malloc(sizeof(wchar_t) * size));
    if (!cstr)
      return L"";

    va_copy(argCopy, args);
    int nActual = vswprintf(cstr, size, fmt, argCopy);
    va_end(argCopy);

    if (nActual > -1 && nActual < size) // We got a valid result
    {
      std::wstring str(cstr, nActual);
      free(cstr);
      return str;
    }
    free(cstr);

#ifndef TARGET_WINDOWS
    if (nActual > -1)                   // Exactly what we will need (glibc 2.1)
      size = nActual + 1;
    else                                // Let's try to double the size (glibc 2.0)
      size *= 2;
#else  // TARGET_WINDOWS
    va_copy(argCopy, args);
    size = _vscwprintf(fmt, argCopy);
    va_end(argCopy);
    if (size < 0)
      return L"";
    else
      size++; // increment for null-termination
#endif // TARGET_WINDOWS
  }

  return L"";
}

std::wstring StringUtils::UTF8ToWString(std::string_view str)
{
  std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
  std::wstring result = conv.from_bytes(str.data(), str.data() + str.length());
  return result;
}

std::string StringUtils::WStringToUTF8(std::wstring_view str)
{
  std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
  std::string result = conv.to_bytes(str.data(), str.data() + str.length());
  return result;
}

int compareWchar (const void* a, const void* b)
{
  if (*(const wchar_t*)a <  *(const wchar_t*)b)
    return -1;
  else if (*(const wchar_t*)a >  *(const wchar_t*)b)
    return 1;
  return 0;
}

int compareChar32_t(const void* a, const void* b)
{
  if (*(const char32_t*)a < *(const char32_t*)b)
    return -1;
  else if (*(const char32_t*)a > *(const char32_t*)b)
    return 1;
  return 0;
}

/*
 * TODO: suggest removal of tolowerUnicode and toupperUnicode and simply 
 * use std:tolowercase/touppercase for ToLower & ToUpper.
 *
 * Performs a conversion to lowercase using UTF16 tables that are language/locale
 * agnostic. This means that the tables only contains conversions where
 * there is no conflict (such as when some characters follow different casing rules
 * depending upon language or context). Locale agnostic characters
 * can safely be converted to lower case without breaking a particular
 * language's rule.
 *
 * It is questionable if this should be used for tolower, since ignoring
 * locale is non-standard behavior. Further, this creates a problem where ToLower(wstring)
 * produces different results than ToLower(string), but that is how
 * StringUtils has behaved for some time. Finally, the table may be a bit out of date,
 * being 10 years old.
 */

wchar_t tolowerUnicode(const wchar_t& c)
{
  wchar_t* p = (wchar_t*) bsearch (&c, unicode_uppers, sizeof(unicode_uppers) / sizeof(wchar_t), sizeof(wchar_t), compareWchar);
  if (p)
    return *(unicode_lowers + (p - unicode_uppers));

  return c;
}

/*
 * Comments similar to those for tolowerUnicode, above, apply here.
 */
wchar_t toupperUnicode(const wchar_t& c)
{
  wchar_t* p = (wchar_t*) bsearch (&c, unicode_lowers, sizeof(unicode_lowers) / sizeof(wchar_t), sizeof(wchar_t), compareWchar);
  if (p)
    return *(unicode_uppers + (p - unicode_lowers));

  return c;
}

/*!
 * \brief Folds the case of the given Unicode (32-bit) character
 *
 * Performs "simple" case folding using data from Unicode Inc.'s ICUC4 (License near top of file).
 * Read the description preceding the tables (unicode_fold_upper, unicode_fold_lower) or
 * from the API documentation for FoldCase.
 */
char32_t foldCaseUnicode(const char32_t& c)
{
  char32_t* p =
      (char32_t*)bsearch(&c, unicode_fold_upper, sizeof(unicode_fold_upper) / sizeof(char32_t),
                         sizeof(char32_t), compareChar32_t);
  if (p)
    return *(unicode_fold_lower + (p - unicode_fold_upper));

  return c;
}

char32_t foldCaseUpperUnicode(const char32_t& c)
{
  char32_t* p =
      (char32_t*)bsearch(&c, unicode_fold_lower, sizeof(unicode_fold_lower) / sizeof(char32_t),
                         sizeof(char32_t), compareChar32_t);
  if (p)
    return *(unicode_fold_upper + (p - unicode_fold_lower));

  return c;
}

/*
 * This method to be removed in this PR once the codebase no longer depends upon it. After that,
 * there will be just one signature: std::string ToUpper(std:string_view, locale).
 * (Can this be done without marking it "deprecated" for a while? Even when
 * there are no current users?)
 */
void StringUtils::ToUpper(std::string& str)
{
  std::string result = StringUtils::ToUpper(std::string_view(str));
  result.swap(str);
}

/*
  * This method to be removed in this PR once the codebase no longer depends upon it. After that,
  * there will be just one signature: std::string ToUpper(std:wstring_view, locale).
  * (Can this be done without marking it "deprecated" for a while? Even when
  * there are no current users?)
  */
void StringUtils::ToUpper(std::wstring& str)
{
  std::wstring result = StringUtils::ToUpper(std::wstring_view(str));
  result.swap(str);
}

/*
  * These methods kept until all usages have been switched to string_view version.
  *
  * Disabled these signatures because it can conflict with the new
  * string_view signature, which is compatible with this signature.
  *
 std::string StringUtils::ToLower(const std::string& str)
 {
   std::string result = StringUtils::ToLower(std::string_view(str));
   return result;
 }

 std::wstring StringUtils::ToLower(const std::wstring& str)
 {
   std::wstring result = StringUtils::ToLower(std::wstring_view(str));
   return result;
 }
 */

/*
  * This method to be removed in this PR once the codebase no longer depends upon it. After that,
  * there will be just one signature: std::string ToUpper(std:wstring_view, locale).
  * (Can this be done without marking it "deprecated" for a while? Even when
  * there are no current users?)
  */
void StringUtils::ToLower(std::string& str)
{
  std::string result = StringUtils::ToLower(std::string_view(str));
  result.swap(str);
}

/*
  * This method to be removed in this PR once the codebase no longer depends upon it. After that,
  * there will be just one signature: std::string ToUpper(std:wstring_view, locale).
  * (Can this be done without marking it "deprecated" for a while? Even when
  * there are no current users?)
  */
void StringUtils::ToLower(std::wstring& str)
{
  std::wstring result = StringUtils::ToLower(std::wstring_view(str));
  result.swap(str);
}

/*
  * New method signature. Complies with Kodi rules: return by value, and
  * string_view is more efficient and flexible.
  */
std::string StringUtils::ToLower(std::string_view str,
                                 std::locale locale /* = g_langInfo.GetSystemLocale()) */)
{
  /*
    * Since C++'s tolower(char, locale) operates on only a byte at a time, it's
    * UTF8 accuracy is inferior to tolower(wchar_t, locale). Further, the results
    * of ToLower(string_view) can be different from ToLower(wstring_view). To
    * address these problems, convert to wstring and use ToLower(wstring_view).
    */

  if (str.length() == 0)
    return std::string(str);

    // TODO: Remove TEST_LOWER before completing PR
#undef TEST_LOWER
// #define TEST_LOWER 1
#ifdef TEST_LOWER
  // Compare result of ToLower (utf8) vs ToLower(UTF32)

  std::string resultUTF8; // Assumes lengths don't change.
  for (char c : str)
  {
    resultUTF8.push_back(tolower(c, locale));
  }
#endif

  std::wstring wstr = UTF8ToWString(str);
  std::wstring wresult;

  for (wchar_t c : wstr)
  {
    wresult.push_back(tolower(c, locale));
  }
  std::string result = WStringToUTF8(wresult);
#ifdef TEST_LOWER

  if (result.compare(resultUTF8) != 0)
  {
    std::cout << "ToLower different result for utf8 and wstring src: " << str
              << " utf8 lower: " << resultUTF8 << " vs: " << result << std::endl;
  }
  // else
  // {
  //   std::cout << "ToLower SAME result for utf8 and wstring src: " << str << " utf8 lower: " << resultUTF8 << " vs: " << result << std::endl;
  //}
#endif

  return result;
}

/*
  * This method to be removed in this PR once the codebase no longer depends upon it. After that,
  * there will be just one signature: std::string ToUpper(std:wstring_view, locale).
  * (Can this be done without marking it "deprecated" for a while? Even when
  * there are no current users?)
  */
std::wstring ToLower(const wchar_t* str, std::locale locale)
{
  if (!str)
  {
    return std::wstring();
  }
  std::wstring data(str);
  StringUtils::ToLower(data, locale);
  return data;
  // std::use_facet<std::ctype<wchar_t>>(locale).tolower(&data[0], &data[0] + data.size());
  // return data;
}

std::wstring StringUtils::ToLower(std::wstring_view str,
                                  std::locale locale /* = g_langInfo.GetSystemLocale()) */)
{
  // TODO: Consider eliminating use of "tolowerUnicode".
  //
  // Preserves non-standard behavior of previous versions of Kodi by using a locale
  // agnostic case conversion table:
  //
  //  - Produces different results from ToLower(string)
  //  - The tables used for char conversion OMITs a number of characters
  //    that should be changed.

  if (str.length() == 0)
    return std::wstring(str);

  std::wstring result;
  for (wchar_t c : str)
  {
    // result.push_back(tolower(c, locale));
    result.push_back(tolowerUnicode(c));
  }
  return result;
}

std::string StringUtils::ToUpper(std::string_view str,
                                 std::locale locale /* = g_langInfo.GetSystemLocale()) */)
{
  /*
   * Since C++'s toupper(char, locale) operates on only a byte at a time, it's
   * UTF8 accuracy is not that great, this method acts as a wrapper around
   * ToUpper(wstring_view). This gives better accuracy and consistent results
   * with the wstring version, but at a higher cpu cost. This decision is easily reversed.
   */

  std::wstring wstr = UTF8ToWString(str);
  std::wstring result;
  for (wchar_t c : wstr)
  {
    result.push_back(toupper(c, locale));
  }
  return WStringToUTF8(result);
}

std::wstring StringUtils::ToUpper(std::wstring_view str,
                                  std::locale locale /* = g_langInfo.GetSystemLocale()) */)
{
  if (str.length() == 0)
    return std::wstring(str);

  std::wstring result; // Assumes lengths don't change.
  for (wchar_t c : str)
  {
    // TODO: Consider eliminating use of "toupperUnicode/tolowerUnicode".
    //
    // Preserves non-standard behavior of previous versions of Kodi by using a locale
    // agnostic case conversion table:
    //
    //  - Produces different results from ToUpper(string)
    //  - The tables used for char conversion OMITs a number of characters
    //    that should be changed.
    // Will address in issue 22003

    // result.push_back(toupper(c, locale));
    result.push_back(toupperUnicode(c));
  }
  return result;
}

std::u32string StringUtils::FoldCase(std::u32string_view str)
{
  // Common code to do actual case folding

  if (str.length() == 0)
    return std::u32string(str);

  // C++ FoldCase doesn't change string length; more sophisticated libs, such as ICU
  // can.

  std::u32string result;
  for (auto i = str.begin(); i != str.end(); i++)
  {
    char32_t fold_c = foldCaseUnicode(*i);
    result.push_back(fold_c);
  }
  return result;
}

// TODO: Remove VERIFY_ASCII prior to completing PR
#undef VERIFY_ASCII
// #define VERIFY_ASCII 1
std::wstring StringUtils::FoldCase(std::wstring_view str)
{
  // In the multi-lingual world, FoldCase is used instead of ToLower to 'normalize'
  // unique-ids, such as property ids, map keys, etc. In the good-'ol days of ASCII
  // ToLower was fine, but when you have ToLower changing behavior depending upon
  // the current locale, you have a disaster. For the curious, look up "Turkish-I"
  // problem.
  //
  // FoldCase is designed to transform strings so that unimportant differences
  // (letter case, some accents, etc.) are neutralized by monocasing, etc..
  // Full case folding goes further and converts strings into a canonical
  // form (ex: German sharfes-S (looks like a script B) is converted to
  // to "ss").
  //
  // The FoldCase here does not support full case folding, but as long as key ids
  // remain essentially ASCII, or at least not too exotic, then this FoldCase
  // should be fine. (A library, such as ICUC4 is required for more advanced
  // Folding).
  //
  // Even though Kodi appears to have fairly well behaved unique-ids, it is
  // very easy to create bad ones and they can be hard to detect.
  //  - The "Turkish-I" problem caught everyone by surprise. No one knew
  //    that in a few Turkic locales, changing the case of "i" caused a non-Latin
  //    "i" to appear (I think a "dotless 'i', but maybe a dotted 'I'). The
  //    problem prevented Kodi from starting at all.
  //  - As I write this, there are at least five instances where a translated
  //    value for a device name ("keyboard," etc.) is used as the unique-id.
  //    This was only found by detecting non-ASCII ids AND setting locale to
  //    Russian. The problem might be benign, but a lot of testing or
  //    research is required to be sure.
  //  - Python addons face the same problem. This code does NOT address
  //    that issue.
  //
  // The FoldCase here is based on character data from ICU4C. (The library
  // and data are constantly changing. The current version is from 2021.)
  // The data is in UTF32 format. Since C++ string functions, such as
  // tolower(char) only examines one byte at a time (for UTF8),
  // it is unable to properly process multi-byte characters. Similarly
  // for UTF16, multi-UTF16 code-unit characters are not properly
  // processed (fortunately there are far fewer characters longer than
  // 16-bits than for 8-bit ones). For this reason, both versions of
  // FoldCase (UTF8 and wstring) converts its argument to utf32 to
  // perform the fold and then back to the original UTF8, wstring,
  // etc. after the fold.
  //

  if (str.length() == 0)
    return std::wstring(str);

  // C++ tolower/toupper (which Foldcase uses) doesn't change string length;
  // more sophisticated libs, such as ICU can.

  // TODO: Consider performance impact and improvements:
  //       1- Creating string_view signatures for CharacterConverter
  //       2- If desperate, use char32_t[] so malloc is not used

  std::u32string utf32Str;
  const std::wstring strTmp{str}; // Need to change CharsetConvert to use string-view.
  g_charsetConverter.wToUtf32(strTmp, utf32Str);
  std::u32string foldedStr = StringUtils::FoldCase(utf32Str);
  std::wstring result;
  g_charsetConverter.utf32ToW(foldedStr, result);

#ifdef VERIFY_ASCII
  bool nonAscii = false;
  for (wchar_t c : str)
  {
    if (not isascii(c))
    {
      nonAscii = true;
      break;
    }
  }
  if (nonAscii)
  {
    CLog::Log(LOGWARNING, "StringUtils::FoldCase: Non-ASCII input: {}\n", WStringToUTF8(str));
  }
#endif

  return result;
}

std::string StringUtils::FoldCase(std::string_view str)
{
  // To get same behavior and better accuracy as the wstring version, convert to utf32string.

  const std::string strTmp(str);
  std::u32string utf32Str(U"");
  g_charsetConverter.utf8ToUtf32(strTmp, utf32Str);
  std::u32string foldedStr = StringUtils::FoldCase(utf32Str);
  std::string result;
  g_charsetConverter.utf32ToUtf8(foldedStr, result);

  // TODO: Remove VERIFY_ASCII before PR complete
#ifdef VERIFY_ASCII
  bool nonAscii = false;
  for (wchar_t c : str)
  {
    if (not isascii(c))
    {
      nonAscii = true;
      break;
    }
    result.push_back(tolowerUnicode(c));
  }
  if (nonAscii)
  {
    CLog::Log(LOGWARNING, "StringUtils::FoldCase: Non-ASCII input: {}\n", str);
  }
  return result;
#endif
  return result;
}

std::u32string StringUtils::FoldCaseUpper(std::u32string_view str)
{
  // Common code to do actual case folding

  if (str.length() == 0)
    return std::u32string(str);

  // C++ FoldCase doesn't change string length; more sophisticated libs, such as ICU
  // can.

  std::u32string result;
  for (auto i = str.begin(); i != str.end(); i++)
  {
    char32_t fold_c = foldCaseUpperUnicode(*i);
    result.push_back(fold_c);
  }
  return result;
}

std::wstring StringUtils::FoldCaseUpper(std::wstring_view str)
{
  // Similar to FoldCase.

  if (str.length() == 0)
    return std::wstring(str);

  // C++ FoldCase doesn't change string length, more sophisticated libs, such as ICU
  // can.

  // Perform the case-fold using UTF32 for maximum accuracy.

  std::u32string utf32Str;
  std::wstring strTmp{str}; // Need to change CharsetConvert to use string-view.
  g_charsetConverter.wToUtf32(strTmp, utf32Str);
  const std::u32string foldedStr = StringUtils::FoldCaseUpper(utf32Str);
  std::wstring result;
  g_charsetConverter.utf32ToW(foldedStr, result);
  return result;
}

std::string StringUtils::FoldCaseUpper(std::string_view str)
{
  // To get same behavior and better accuracy as the wstring version, convert to utf32string.

  std::string strTmp(str);
  std::u32string utf32Str;
  g_charsetConverter.utf8ToUtf32(strTmp, utf32Str);
  std::u32string foldedStr = StringUtils::FoldCaseUpper(utf32Str);
  std::string result;
  g_charsetConverter.utf32ToUtf8(foldedStr, result);
  return result;
}

void StringUtils::ToCapitalize(std::string &str)
{
  std::wstring wstr;
  g_charsetConverter.utf8ToW(str, wstr);
  ToCapitalize(wstr);
  g_charsetConverter.wToUTF8(wstr, str);
}

void StringUtils::ToCapitalize(std::wstring &str)
{
  const std::locale& loc = g_langInfo.GetSystemLocale();
  bool isFirstLetter = true;
  for (std::wstring::iterator it = str.begin(); it < str.end(); ++it)
  {
    // capitalize after spaces and punctuation characters (except apostrophes)
    if (std::isspace(*it, loc) || (std::ispunct(*it, loc) && *it != '\''))
      isFirstLetter = true;
    else if (isFirstLetter)
    {
      *it = std::toupper(*it, loc);
      isFirstLetter = false;
    }
  }
}

// TODO: Change all caseless comparisons to use same FoldCase algorithm.
// Will address in followup commit to this PR. Current code is no worse than what it was.

bool StringUtils::EqualsNoCase(const std::string &str1, const std::string &str2)
{
  // It would be more accurate to FoldCase using wstring, since tolower(byte) does not
  // work so well with multi-byte characters. However, converting to wstring and back does add cost.
  // This will be addressed in latter commit to this PR.

  // before we do the char-by-char comparison, first compare sizes of both strings.
  // This led to a 33% improvement in benchmarking on average. (size() just returns a member of std::string)
  if (str1.size() != str2.size())
    return false;
  return EqualsNoCase(str1.c_str(), str2.c_str());
}

bool StringUtils::EqualsNoCase(const std::string &str1, const char *s2)
{
  return EqualsNoCase(str1.c_str(), s2);
}

bool StringUtils::EqualsNoCase(const char *s1, const char *s2)
{
  char c2; // we need only one char outside the loop
  do
  {
    const char c1 = *s1++; // const local variable should help compiler to optimize
    c2 = *s2++;
    if (c1 != c2 &&
        tolower(c1, getFOLD_LOCALE()) !=
            tolower(
                c2,
                getFOLD_LOCALE())) // This includes the possibility that one of the characters is the null-terminator, which implies a string mismatch.
      return false;
  } while (c2 != '\0'); // At this point, we know c1 == c2, so there's no need to test them both.
  return true;
}

int StringUtils::CompareNoCase(const std::string& str1, const std::string& str2, size_t n /* = 0 */)
{
  return CompareNoCase(str1.c_str(), str2.c_str(), n);
}

int StringUtils::CompareNoCase(const char* s1, const char* s2, size_t n /* = 0 */)
{
  char c2; // we need only one char outside the loop
  size_t index = 0;
  do
  {
    const char c1 = *s1++; // const local variable should help compiler to optimize
    c2 = *s2++;
    index++;
    if (c1 != c2 &&
        tolower(c1, getFOLD_LOCALE()) !=
            tolower(
                c2,
                getFOLD_LOCALE())) // This includes the possibility that one of the characters is the null-terminator, which implies a string mismatch.
      return tolower(c1, getFOLD_LOCALE()) - tolower(c2, getFOLD_LOCALE());
  } while (c2 != '\0' &&
           index != n); // At this point, we know c1 == c2, so there's no need to test them both.
  return 0;
}

std::string StringUtils::Left(const std::string &str, size_t count)
{
  count = std::max((size_t)0, std::min(count, str.size()));
  return str.substr(0, count);
}

std::string StringUtils::Mid(const std::string &str, size_t first, size_t count /* = string::npos */)
{
  if (first + count > str.size())
    count = str.size() - first;

  if (first > str.size())
    return std::string();

  assert(first + count <= str.size());

  return str.substr(first, count);
}

std::string StringUtils::Right(const std::string &str, size_t count)
{
  count = std::max((size_t)0, std::min(count, str.size()));
  return str.substr(str.size() - count);
}

std::string& StringUtils::Trim(std::string &str)
{
  TrimLeft(str);
  return TrimRight(str);
}

std::string& StringUtils::Trim(std::string &str, const char* const chars)
{
  TrimLeft(str, chars);
  return TrimRight(str, chars);
}

// hack to check only first byte of UTF-8 character
// without this hack "TrimX" functions failed on Win32 and OS X with UTF-8 strings
static int isspace_c(char c)
{
  return (c & 0x80) == 0 && ::isspace(c);
}

std::string& StringUtils::TrimLeft(std::string &str)
{
  str.erase(str.begin(),
            std::find_if(str.begin(), str.end(), [](char s) { return isspace_c(s) == 0; }));
  return str;
}

std::string& StringUtils::TrimLeft(std::string &str, const char* const chars)
{
  size_t nidx = str.find_first_not_of(chars);
  str.erase(0, nidx);
  return str;
}

std::string& StringUtils::TrimRight(std::string &str)
{
  str.erase(std::find_if(str.rbegin(), str.rend(), [](char s) { return isspace_c(s) == 0; }).base(),
            str.end());
  return str;
}

std::string& StringUtils::TrimRight(std::string &str, const char* const chars)
{
  size_t nidx = str.find_last_not_of(chars);
  str.erase(str.npos == nidx ? 0 : ++nidx);
  return str;
}

int StringUtils::ReturnDigits(const std::string& str)
{
  std::stringstream ss;
  for (const auto& character : str)
  {
    if (isdigit(character))
      ss << character;
  }
  return atoi(ss.str().c_str());
}

std::string& StringUtils::RemoveDuplicatedSpacesAndTabs(std::string& str)
{
  std::string::iterator it = str.begin();
  bool onSpace = false;
  while(it != str.end())
  {
    if (*it == '\t')
      *it = ' ';

    if (*it == ' ')
    {
      if (onSpace)
      {
        it = str.erase(it);
        continue;
      }
      else
        onSpace = true;
    }
    else
      onSpace = false;

    ++it;
  }
  return str;
}

int StringUtils::Replace(std::string &str, char oldChar, char newChar)
{
  int replacedChars = 0;
  for (std::string::iterator it = str.begin(); it != str.end(); ++it)
  {
    if (*it == oldChar)
    {
      *it = newChar;
      replacedChars++;
    }
  }

  return replacedChars;
}

int StringUtils::Replace(std::string &str, const std::string &oldStr, const std::string &newStr)
{
  if (oldStr.empty())
    return 0;

  int replacedChars = 0;
  size_t index = 0;

  while (index < str.size() && (index = str.find(oldStr, index)) != std::string::npos)
  {
    str.replace(index, oldStr.size(), newStr);
    index += newStr.size();
    replacedChars++;
  }

  return replacedChars;
}

int StringUtils::Replace(std::wstring &str, const std::wstring &oldStr, const std::wstring &newStr)
{
  if (oldStr.empty())
    return 0;

  int replacedChars = 0;
  size_t index = 0;

  while (index < str.size() && (index = str.find(oldStr, index)) != std::string::npos)
  {
    str.replace(index, oldStr.size(), newStr);
    index += newStr.size();
    replacedChars++;
  }

  return replacedChars;
}

bool StringUtils::StartsWith(const std::string &str1, const std::string &str2)
{
  return str1.compare(0, str2.size(), str2) == 0;
}

bool StringUtils::StartsWith(const std::string &str1, const char *s2)
{
  return StartsWith(str1.c_str(), s2);
}

bool StringUtils::StartsWith(const char *s1, const char *s2)
{
  while (*s2 != '\0')
  {
    if (*s1 != *s2)
      return false;
    s1++;
    s2++;
  }
  return true;
}

// TODO: Change to use FoldCase algorithm

bool StringUtils::StartsWithNoCase(const std::string &str1, const std::string &str2)
{
  return StartsWithNoCase(str1.c_str(), str2.c_str());
}

bool StringUtils::StartsWithNoCase(const std::string &str1, const char *s2)
{
  return StartsWithNoCase(str1.c_str(), s2);
}

bool StringUtils::StartsWithNoCase(const char *s1, const char *s2)
{
  while (*s2 != '\0')
  {
    if (tolower(*s1, getFOLD_LOCALE()) != tolower(*s2, getFOLD_LOCALE()))
      return false;
    s1++;
    s2++;
  }
  return true;
}

bool StringUtils::EndsWith(const std::string &str1, const std::string &str2)
{
  if (str1.size() < str2.size())
    return false;
  return str1.compare(str1.size() - str2.size(), str2.size(), str2) == 0;
}

bool StringUtils::EndsWith(const std::string &str1, const char *s2)
{
  size_t len2 = strlen(s2);
  if (str1.size() < len2)
    return false;
  return str1.compare(str1.size() - len2, len2, s2) == 0;
}

bool StringUtils::EndsWithNoCase(const std::string &str1, const std::string &str2)
{
  if (str1.size() < str2.size())
    return false;

  const char *s1 = str1.c_str() + str1.size() - str2.size();
  const char *s2 = str2.c_str();
  while (*s2 != '\0')
  {
    if (tolower(*s1, getFOLD_LOCALE()) != tolower(*s2, getFOLD_LOCALE()))
      return false;
    s1++;
    s2++;
  }
  return true;
}

bool StringUtils::EndsWithNoCase(const std::string &str1, const char *s2)
{
  size_t len2 = strlen(s2);
  if (str1.size() < len2)
    return false;

  const char *s1 = str1.c_str() + str1.size() - len2;
  while (*s2 != '\0')
  {
    if (tolower(*s1, getFOLD_LOCALE()) != tolower(*s2, getFOLD_LOCALE()))
      return false;
    s1++;
    s2++;
  }
  return true;
}

std::vector<std::string> StringUtils::Split(const std::string& input, const std::string& delimiter, unsigned int iMaxStrings)
{
  std::vector<std::string> result;
  SplitTo(std::back_inserter(result), input, delimiter, iMaxStrings);
  return result;
}

std::vector<std::string> StringUtils::Split(const std::string& input, const char delimiter, size_t iMaxStrings)
{
  std::vector<std::string> result;
  SplitTo(std::back_inserter(result), input, delimiter, iMaxStrings);
  return result;
}

std::vector<std::string> StringUtils::Split(const std::string& input, const std::vector<std::string>& delimiters)
{
  std::vector<std::string> result;
  SplitTo(std::back_inserter(result), input, delimiters);
  return result;
}

std::vector<std::string> StringUtils::SplitMulti(const std::vector<std::string>& input,
                                                 const std::vector<std::string>& delimiters,
                                                 size_t iMaxStrings /* = 0 */)
{
  if (input.empty())
    return std::vector<std::string>();

  std::vector<std::string> results(input);

  if (delimiters.empty() || (iMaxStrings > 0 && iMaxStrings <= input.size()))
    return results;

  std::vector<std::string> strings1;
  if (iMaxStrings == 0)
  {
    for (size_t di = 0; di < delimiters.size(); di++)
    {
      for (size_t i = 0; i < results.size(); i++)
      {
        std::vector<std::string> substrings = StringUtils::Split(results[i], delimiters[di]);
        for (size_t j = 0; j < substrings.size(); j++)
          strings1.push_back(substrings[j]);
      }
      results = strings1;
      strings1.clear();
    }
    return results;
  }

  // Control the number of strings input is split into, keeping the original strings.
  // Note iMaxStrings > input.size()
  int64_t iNew = iMaxStrings - results.size();
  for (size_t di = 0; di < delimiters.size(); di++)
  {
    for (size_t i = 0; i < results.size(); i++)
    {
      if (iNew > 0)
      {
        std::vector<std::string> substrings = StringUtils::Split(results[i], delimiters[di], iNew + 1);
        iNew = iNew - substrings.size() + 1;
        for (size_t j = 0; j < substrings.size(); j++)
          strings1.push_back(substrings[j]);
      }
      else
        strings1.push_back(results[i]);
    }
    results = strings1;
    iNew = iMaxStrings - results.size();
    strings1.clear();
    if ((iNew <= 0))
      break;  //Stop trying any more delimiters
  }
  return results;
}

// returns the number of occurrences of strFind in strInput.
int StringUtils::FindNumber(const std::string& strInput, const std::string &strFind)
{
  size_t pos = strInput.find(strFind, 0);
  int numfound = 0;
  while (pos != std::string::npos)
  {
    numfound++;
    pos = strInput.find(strFind, pos + 1);
  }
  return numfound;
}

// Plane maps for MySQL utf8_general_ci (now known as utf8mb3_general_ci) collation
// Derived from https://github.com/MariaDB/server/blob/10.5/strings/ctype-utf8.c

// clang-format off
static const uint16_t plane00[] = {
  0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007, 0x0008, 0x0009, 0x000A, 0x000B, 0x000C, 0x000D, 0x000E, 0x000F,
  0x0010, 0x0011, 0x0012, 0x0013, 0x0014, 0x0015, 0x0016, 0x0017, 0x0018, 0x0019, 0x001A, 0x001B, 0x001C, 0x001D, 0x001E, 0x001F,
  0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027, 0x0028, 0x0029, 0x002A, 0x002B, 0x002C, 0x002D, 0x002E, 0x002F,
  0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0x003A, 0x003B, 0x003C, 0x003D, 0x003E, 0x003F,
  0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 0x0048, 0x0049, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F,
  0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 0x0058, 0x0059, 0x005A, 0x005B, 0x005C, 0x005D, 0x005E, 0x005F,
  0x0060, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 0x0048, 0x0049, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F,
  0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 0x0058, 0x0059, 0x005A, 0x007B, 0x007C, 0x007D, 0x007E, 0x007F,
  0x0080, 0x0081, 0x0082, 0x0083, 0x0084, 0x0085, 0x0086, 0x0087, 0x0088, 0x0089, 0x008A, 0x008B, 0x008C, 0x008D, 0x008E, 0x008F,
  0x0090, 0x0091, 0x0092, 0x0093, 0x0094, 0x0095, 0x0096, 0x0097, 0x0098, 0x0099, 0x009A, 0x009B, 0x009C, 0x009D, 0x009E, 0x009F,
  0x00A0, 0x00A1, 0x00A2, 0x00A3, 0x00A4, 0x00A5, 0x00A6, 0x00A7, 0x00A8, 0x00A9, 0x00AA, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x00AF,
  0x00B0, 0x00B1, 0x00B2, 0x00B3, 0x00B4, 0x039C, 0x00B6, 0x00B7, 0x00B8, 0x00B9, 0x00BA, 0x00BB, 0x00BC, 0x00BD, 0x00BE, 0x00BF,
  0x0041, 0x0041, 0x0041, 0x0041, 0x0041, 0x0041, 0x00C6, 0x0043, 0x0045, 0x0045, 0x0045, 0x0045, 0x0049, 0x0049, 0x0049, 0x0049,
  0x00D0, 0x004E, 0x004F, 0x004F, 0x004F, 0x004F, 0x004F, 0x00D7, 0x00D8, 0x0055, 0x0055, 0x0055, 0x0055, 0x0059, 0x00DE, 0x0053,
  0x0041, 0x0041, 0x0041, 0x0041, 0x0041, 0x0041, 0x00C6, 0x0043, 0x0045, 0x0045, 0x0045, 0x0045, 0x0049, 0x0049, 0x0049, 0x0049,
  0x00D0, 0x004E, 0x004F, 0x004F, 0x004F, 0x004F, 0x004F, 0x00F7, 0x00D8, 0x0055, 0x0055, 0x0055, 0x0055, 0x0059, 0x00DE, 0x0059
};

static const uint16_t plane01[] = {
  0x0041, 0x0041, 0x0041, 0x0041, 0x0041, 0x0041, 0x0043, 0x0043, 0x0043, 0x0043, 0x0043, 0x0043, 0x0043, 0x0043, 0x0044, 0x0044,
  0x0110, 0x0110, 0x0045, 0x0045, 0x0045, 0x0045, 0x0045, 0x0045, 0x0045, 0x0045, 0x0045, 0x0045, 0x0047, 0x0047, 0x0047, 0x0047,
  0x0047, 0x0047, 0x0047, 0x0047, 0x0048, 0x0048, 0x0126, 0x0126, 0x0049, 0x0049, 0x0049, 0x0049, 0x0049, 0x0049, 0x0049, 0x0049,
  0x0049, 0x0049, 0x0132, 0x0132, 0x004A, 0x004A, 0x004B, 0x004B, 0x0138, 0x004C, 0x004C, 0x004C, 0x004C, 0x004C, 0x004C, 0x013F,
  0x013F, 0x0141, 0x0141, 0x004E, 0x004E, 0x004E, 0x004E, 0x004E, 0x004E, 0x0149, 0x014A, 0x014A, 0x004F, 0x004F, 0x004F, 0x004F,
  0x004F, 0x004F, 0x0152, 0x0152, 0x0052, 0x0052, 0x0052, 0x0052, 0x0052, 0x0052, 0x0053, 0x0053, 0x0053, 0x0053, 0x0053, 0x0053,
  0x0053, 0x0053, 0x0054, 0x0054, 0x0054, 0x0054, 0x0166, 0x0166, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055,
  0x0055, 0x0055, 0x0055, 0x0055, 0x0057, 0x0057, 0x0059, 0x0059, 0x0059, 0x005A, 0x005A, 0x005A, 0x005A, 0x005A, 0x005A, 0x0053,
  0x0180, 0x0181, 0x0182, 0x0182, 0x0184, 0x0184, 0x0186, 0x0187, 0x0187, 0x0189, 0x018A, 0x018B, 0x018B, 0x018D, 0x018E, 0x018F,
  0x0190, 0x0191, 0x0191, 0x0193, 0x0194, 0x01F6, 0x0196, 0x0197, 0x0198, 0x0198, 0x019A, 0x019B, 0x019C, 0x019D, 0x019E, 0x019F,
  0x004F, 0x004F, 0x01A2, 0x01A2, 0x01A4, 0x01A4, 0x01A6, 0x01A7, 0x01A7, 0x01A9, 0x01AA, 0x01AB, 0x01AC, 0x01AC, 0x01AE, 0x0055,
  0x0055, 0x01B1, 0x01B2, 0x01B3, 0x01B3, 0x01B5, 0x01B5, 0x01B7, 0x01B8, 0x01B8, 0x01BA, 0x01BB, 0x01BC, 0x01BC, 0x01BE, 0x01F7,
  0x01C0, 0x01C1, 0x01C2, 0x01C3, 0x01C4, 0x01C4, 0x01C4, 0x01C7, 0x01C7, 0x01C7, 0x01CA, 0x01CA, 0x01CA, 0x0041, 0x0041, 0x0049,
  0x0049, 0x004F, 0x004F, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055, 0x018E, 0x0041, 0x0041,
  0x0041, 0x0041, 0x00C6, 0x00C6, 0x01E4, 0x01E4, 0x0047, 0x0047, 0x004B, 0x004B, 0x004F, 0x004F, 0x004F, 0x004F, 0x01B7, 0x01B7,
  0x004A, 0x01F1, 0x01F1, 0x01F1, 0x0047, 0x0047, 0x01F6, 0x01F7, 0x004E, 0x004E, 0x0041, 0x0041, 0x00C6, 0x00C6, 0x00D8, 0x00D8
};

static const uint16_t plane02[] = {
  0x0041, 0x0041, 0x0041, 0x0041, 0x0045, 0x0045, 0x0045, 0x0045, 0x0049, 0x0049, 0x0049, 0x0049, 0x004F, 0x004F, 0x004F, 0x004F,
  0x0052, 0x0052, 0x0052, 0x0052, 0x0055, 0x0055, 0x0055, 0x0055, 0x0053, 0x0053, 0x0054, 0x0054, 0x021C, 0x021C, 0x0048, 0x0048,
  0x0220, 0x0221, 0x0222, 0x0222, 0x0224, 0x0224, 0x0041, 0x0041, 0x0045, 0x0045, 0x004F, 0x004F, 0x004F, 0x004F, 0x004F, 0x004F,
  0x004F, 0x004F, 0x0059, 0x0059, 0x0234, 0x0235, 0x0236, 0x0237, 0x0238, 0x0239, 0x023A, 0x023B, 0x023C, 0x023D, 0x023E, 0x023F,
  0x0240, 0x0241, 0x0242, 0x0243, 0x0244, 0x0245, 0x0246, 0x0247, 0x0248, 0x0249, 0x024A, 0x024B, 0x024C, 0x024D, 0x024E, 0x024F,
  0x0250, 0x0251, 0x0252, 0x0181, 0x0186, 0x0255, 0x0189, 0x018A, 0x0258, 0x018F, 0x025A, 0x0190, 0x025C, 0x025D, 0x025E, 0x025F,
  0x0193, 0x0261, 0x0262, 0x0194, 0x0264, 0x0265, 0x0266, 0x0267, 0x0197, 0x0196, 0x026A, 0x026B, 0x026C, 0x026D, 0x026E, 0x019C,
  0x0270, 0x0271, 0x019D, 0x0273, 0x0274, 0x019F, 0x0276, 0x0277, 0x0278, 0x0279, 0x027A, 0x027B, 0x027C, 0x027D, 0x027E, 0x027F,
  0x01A6, 0x0281, 0x0282, 0x01A9, 0x0284, 0x0285, 0x0286, 0x0287, 0x01AE, 0x0289, 0x01B1, 0x01B2, 0x028C, 0x028D, 0x028E, 0x028F,
  0x0290, 0x0291, 0x01B7, 0x0293, 0x0294, 0x0295, 0x0296, 0x0297, 0x0298, 0x0299, 0x029A, 0x029B, 0x029C, 0x029D, 0x029E, 0x029F,
  0x02A0, 0x02A1, 0x02A2, 0x02A3, 0x02A4, 0x02A5, 0x02A6, 0x02A7, 0x02A8, 0x02A9, 0x02AA, 0x02AB, 0x02AC, 0x02AD, 0x02AE, 0x02AF,
  0x02B0, 0x02B1, 0x02B2, 0x02B3, 0x02B4, 0x02B5, 0x02B6, 0x02B7, 0x02B8, 0x02B9, 0x02BA, 0x02BB, 0x02BC, 0x02BD, 0x02BE, 0x02BF,
  0x02C0, 0x02C1, 0x02C2, 0x02C3, 0x02C4, 0x02C5, 0x02C6, 0x02C7, 0x02C8, 0x02C9, 0x02CA, 0x02CB, 0x02CC, 0x02CD, 0x02CE, 0x02CF,
  0x02D0, 0x02D1, 0x02D2, 0x02D3, 0x02D4, 0x02D5, 0x02D6, 0x02D7, 0x02D8, 0x02D9, 0x02DA, 0x02DB, 0x02DC, 0x02DD, 0x02DE, 0x02DF,
  0x02E0, 0x02E1, 0x02E2, 0x02E3, 0x02E4, 0x02E5, 0x02E6, 0x02E7, 0x02E8, 0x02E9, 0x02EA, 0x02EB, 0x02EC, 0x02ED, 0x02EE, 0x02EF,
  0x02F0, 0x02F1, 0x02F2, 0x02F3, 0x02F4, 0x02F5, 0x02F6, 0x02F7, 0x02F8, 0x02F9, 0x02FA, 0x02FB, 0x02FC, 0x02FD, 0x02FE, 0x02FF
};

static const uint16_t plane03[] = {
  0x0300, 0x0301, 0x0302, 0x0303, 0x0304, 0x0305, 0x0306, 0x0307, 0x0308, 0x0309, 0x030A, 0x030B, 0x030C, 0x030D, 0x030E, 0x030F,
  0x0310, 0x0311, 0x0312, 0x0313, 0x0314, 0x0315, 0x0316, 0x0317, 0x0318, 0x0319, 0x031A, 0x031B, 0x031C, 0x031D, 0x031E, 0x031F,
  0x0320, 0x0321, 0x0322, 0x0323, 0x0324, 0x0325, 0x0326, 0x0327, 0x0328, 0x0329, 0x032A, 0x032B, 0x032C, 0x032D, 0x032E, 0x032F,
  0x0330, 0x0331, 0x0332, 0x0333, 0x0334, 0x0335, 0x0336, 0x0337, 0x0338, 0x0339, 0x033A, 0x033B, 0x033C, 0x033D, 0x033E, 0x033F,
  0x0340, 0x0341, 0x0342, 0x0343, 0x0344, 0x0399, 0x0346, 0x0347, 0x0348, 0x0349, 0x034A, 0x034B, 0x034C, 0x034D, 0x034E, 0x034F,
  0x0350, 0x0351, 0x0352, 0x0353, 0x0354, 0x0355, 0x0356, 0x0357, 0x0358, 0x0359, 0x035A, 0x035B, 0x035C, 0x035D, 0x035E, 0x035F,
  0x0360, 0x0361, 0x0362, 0x0363, 0x0364, 0x0365, 0x0366, 0x0367, 0x0368, 0x0369, 0x036A, 0x036B, 0x036C, 0x036D, 0x036E, 0x036F,
  0x0370, 0x0371, 0x0372, 0x0373, 0x0374, 0x0375, 0x0376, 0x0377, 0x0378, 0x0379, 0x037A, 0x037B, 0x037C, 0x037D, 0x037E, 0x037F,
  0x0380, 0x0381, 0x0382, 0x0383, 0x0384, 0x0385, 0x0391, 0x0387, 0x0395, 0x0397, 0x0399, 0x038B, 0x039F, 0x038D, 0x03A5, 0x03A9,
  0x0399, 0x0391, 0x0392, 0x0393, 0x0394, 0x0395, 0x0396, 0x0397, 0x0398, 0x0399, 0x039A, 0x039B, 0x039C, 0x039D, 0x039E, 0x039F,
  0x03A0, 0x03A1, 0x03A2, 0x03A3, 0x03A4, 0x03A5, 0x03A6, 0x03A7, 0x03A8, 0x03A9, 0x0399, 0x03A5, 0x0391, 0x0395, 0x0397, 0x0399,
  0x03A5, 0x0391, 0x0392, 0x0393, 0x0394, 0x0395, 0x0396, 0x0397, 0x0398, 0x0399, 0x039A, 0x039B, 0x039C, 0x039D, 0x039E, 0x039F,
  0x03A0, 0x03A1, 0x03A3, 0x03A3, 0x03A4, 0x03A5, 0x03A6, 0x03A7, 0x03A8, 0x03A9, 0x0399, 0x03A5, 0x039F, 0x03A5, 0x03A9, 0x03CF,
  0x0392, 0x0398, 0x03D2, 0x03D2, 0x03D2, 0x03A6, 0x03A0, 0x03D7, 0x03D8, 0x03D9, 0x03DA, 0x03DA, 0x03DC, 0x03DC, 0x03DE, 0x03DE,
  0x03E0, 0x03E0, 0x03E2, 0x03E2, 0x03E4, 0x03E4, 0x03E6, 0x03E6, 0x03E8, 0x03E8, 0x03EA, 0x03EA, 0x03EC, 0x03EC, 0x03EE, 0x03EE,
  0x039A, 0x03A1, 0x03A3, 0x03F3, 0x03F4, 0x03F5, 0x03F6, 0x03F7, 0x03F8, 0x03F9, 0x03FA, 0x03FB, 0x03FC, 0x03FD, 0x03FE, 0x03FF
};

static const uint16_t plane04[] = {
  0x0415, 0x0415, 0x0402, 0x0413, 0x0404, 0x0405, 0x0406, 0x0406, 0x0408, 0x0409, 0x040A, 0x040B, 0x041A, 0x0418, 0x0423, 0x040F,
  0x0410, 0x0411, 0x0412, 0x0413, 0x0414, 0x0415, 0x0416, 0x0417, 0x0418, 0x0419, 0x041A, 0x041B, 0x041C, 0x041D, 0x041E, 0x041F,
  0x0420, 0x0421, 0x0422, 0x0423, 0x0424, 0x0425, 0x0426, 0x0427, 0x0428, 0x0429, 0x042A, 0x042B, 0x042C, 0x042D, 0x042E, 0x042F,
  0x0410, 0x0411, 0x0412, 0x0413, 0x0414, 0x0415, 0x0416, 0x0417, 0x0418, 0x0419, 0x041A, 0x041B, 0x041C, 0x041D, 0x041E, 0x041F,
  0x0420, 0x0421, 0x0422, 0x0423, 0x0424, 0x0425, 0x0426, 0x0427, 0x0428, 0x0429, 0x042A, 0x042B, 0x042C, 0x042D, 0x042E, 0x042F,
  0x0415, 0x0415, 0x0402, 0x0413, 0x0404, 0x0405, 0x0406, 0x0406, 0x0408, 0x0409, 0x040A, 0x040B, 0x041A, 0x0418, 0x0423, 0x040F,
  0x0460, 0x0460, 0x0462, 0x0462, 0x0464, 0x0464, 0x0466, 0x0466, 0x0468, 0x0468, 0x046A, 0x046A, 0x046C, 0x046C, 0x046E, 0x046E,
  0x0470, 0x0470, 0x0472, 0x0472, 0x0474, 0x0474, 0x0474, 0x0474, 0x0478, 0x0478, 0x047A, 0x047A, 0x047C, 0x047C, 0x047E, 0x047E,
  0x0480, 0x0480, 0x0482, 0x0483, 0x0484, 0x0485, 0x0486, 0x0487, 0x0488, 0x0489, 0x048A, 0x048B, 0x048C, 0x048C, 0x048E, 0x048E,
  0x0490, 0x0490, 0x0492, 0x0492, 0x0494, 0x0494, 0x0496, 0x0496, 0x0498, 0x0498, 0x049A, 0x049A, 0x049C, 0x049C, 0x049E, 0x049E,
  0x04A0, 0x04A0, 0x04A2, 0x04A2, 0x04A4, 0x04A4, 0x04A6, 0x04A6, 0x04A8, 0x04A8, 0x04AA, 0x04AA, 0x04AC, 0x04AC, 0x04AE, 0x04AE,
  0x04B0, 0x04B0, 0x04B2, 0x04B2, 0x04B4, 0x04B4, 0x04B6, 0x04B6, 0x04B8, 0x04B8, 0x04BA, 0x04BA, 0x04BC, 0x04BC, 0x04BE, 0x04BE,
  0x04C0, 0x0416, 0x0416, 0x04C3, 0x04C3, 0x04C5, 0x04C6, 0x04C7, 0x04C7, 0x04C9, 0x04CA, 0x04CB, 0x04CB, 0x04CD, 0x04CE, 0x04CF,
  0x0410, 0x0410, 0x0410, 0x0410, 0x04D4, 0x04D4, 0x0415, 0x0415, 0x04D8, 0x04D8, 0x04D8, 0x04D8, 0x0416, 0x0416, 0x0417, 0x0417,
  0x04E0, 0x04E0, 0x0418, 0x0418, 0x0418, 0x0418, 0x041E, 0x041E, 0x04E8, 0x04E8, 0x04E8, 0x04E8, 0x042D, 0x042D, 0x0423, 0x0423,
  0x0423, 0x0423, 0x0423, 0x0423, 0x0427, 0x0427, 0x04F6, 0x04F7, 0x042B, 0x042B, 0x04FA, 0x04FB, 0x04FC, 0x04FD, 0x04FE, 0x04FF
};

static const uint16_t plane05[] = {
  0x0500, 0x0501, 0x0502, 0x0503, 0x0504, 0x0505, 0x0506, 0x0507, 0x0508, 0x0509, 0x050A, 0x050B, 0x050C, 0x050D, 0x050E, 0x050F,
  0x0510, 0x0511, 0x0512, 0x0513, 0x0514, 0x0515, 0x0516, 0x0517, 0x0518, 0x0519, 0x051A, 0x051B, 0x051C, 0x051D, 0x051E, 0x051F,
  0x0520, 0x0521, 0x0522, 0x0523, 0x0524, 0x0525, 0x0526, 0x0527, 0x0528, 0x0529, 0x052A, 0x052B, 0x052C, 0x052D, 0x052E, 0x052F,
  0x0530, 0x0531, 0x0532, 0x0533, 0x0534, 0x0535, 0x0536, 0x0537, 0x0538, 0x0539, 0x053A, 0x053B, 0x053C, 0x053D, 0x053E, 0x053F,
  0x0540, 0x0541, 0x0542, 0x0543, 0x0544, 0x0545, 0x0546, 0x0547, 0x0548, 0x0549, 0x054A, 0x054B, 0x054C, 0x054D, 0x054E, 0x054F,
  0x0550, 0x0551, 0x0552, 0x0553, 0x0554, 0x0555, 0x0556, 0x0557, 0x0558, 0x0559, 0x055A, 0x055B, 0x055C, 0x055D, 0x055E, 0x055F,
  0x0560, 0x0531, 0x0532, 0x0533, 0x0534, 0x0535, 0x0536, 0x0537, 0x0538, 0x0539, 0x053A, 0x053B, 0x053C, 0x053D, 0x053E, 0x053F,
  0x0540, 0x0541, 0x0542, 0x0543, 0x0544, 0x0545, 0x0546, 0x0547, 0x0548, 0x0549, 0x054A, 0x054B, 0x054C, 0x054D, 0x054E, 0x054F,
  0x0550, 0x0551, 0x0552, 0x0553, 0x0554, 0x0555, 0x0556, 0x0587, 0x0588, 0x0589, 0x058A, 0x058B, 0x058C, 0x058D, 0x058E, 0x058F,
  0x0590, 0x0591, 0x0592, 0x0593, 0x0594, 0x0595, 0x0596, 0x0597, 0x0598, 0x0599, 0x059A, 0x059B, 0x059C, 0x059D, 0x059E, 0x059F,
  0x05A0, 0x05A1, 0x05A2, 0x05A3, 0x05A4, 0x05A5, 0x05A6, 0x05A7, 0x05A8, 0x05A9, 0x05AA, 0x05AB, 0x05AC, 0x05AD, 0x05AE, 0x05AF,
  0x05B0, 0x05B1, 0x05B2, 0x05B3, 0x05B4, 0x05B5, 0x05B6, 0x05B7, 0x05B8, 0x05B9, 0x05BA, 0x05BB, 0x05BC, 0x05BD, 0x05BE, 0x05BF,
  0x05C0, 0x05C1, 0x05C2, 0x05C3, 0x05C4, 0x05C5, 0x05C6, 0x05C7, 0x05C8, 0x05C9, 0x05CA, 0x05CB, 0x05CC, 0x05CD, 0x05CE, 0x05CF,
  0x05D0, 0x05D1, 0x05D2, 0x05D3, 0x05D4, 0x05D5, 0x05D6, 0x05D7, 0x05D8, 0x05D9, 0x05DA, 0x05DB, 0x05DC, 0x05DD, 0x05DE, 0x05DF,
  0x05E0, 0x05E1, 0x05E2, 0x05E3, 0x05E4, 0x05E5, 0x05E6, 0x05E7, 0x05E8, 0x05E9, 0x05EA, 0x05EB, 0x05EC, 0x05ED, 0x05EE, 0x05EF,
  0x05F0, 0x05F1, 0x05F2, 0x05F3, 0x05F4, 0x05F5, 0x05F6, 0x05F7, 0x05F8, 0x05F9, 0x05FA, 0x05FB, 0x05FC, 0x05FD, 0x05FE, 0x05FF
};

static const uint16_t plane1E[] = {
  0x0041, 0x0041, 0x0042, 0x0042, 0x0042, 0x0042, 0x0042, 0x0042, 0x0043, 0x0043, 0x0044, 0x0044, 0x0044, 0x0044, 0x0044, 0x0044,
  0x0044, 0x0044, 0x0044, 0x0044, 0x0045, 0x0045, 0x0045, 0x0045, 0x0045, 0x0045, 0x0045, 0x0045, 0x0045, 0x0045, 0x0046, 0x0046,
  0x0047, 0x0047, 0x0048, 0x0048, 0x0048, 0x0048, 0x0048, 0x0048, 0x0048, 0x0048, 0x0048, 0x0048, 0x0049, 0x0049, 0x0049, 0x0049,
  0x004B, 0x004B, 0x004B, 0x004B, 0x004B, 0x004B, 0x004C, 0x004C, 0x004C, 0x004C, 0x004C, 0x004C, 0x004C, 0x004C, 0x004D, 0x004D,
  0x004D, 0x004D, 0x004D, 0x004D, 0x004E, 0x004E, 0x004E, 0x004E, 0x004E, 0x004E, 0x004E, 0x004E, 0x004F, 0x004F, 0x004F, 0x004F,
  0x004F, 0x004F, 0x004F, 0x004F, 0x0050, 0x0050, 0x0050, 0x0050, 0x0052, 0x0052, 0x0052, 0x0052, 0x0052, 0x0052, 0x0052, 0x0052,
  0x0053, 0x0053, 0x0053, 0x0053, 0x0053, 0x0053, 0x0053, 0x0053, 0x0053, 0x0053, 0x0054, 0x0054, 0x0054, 0x0054, 0x0054, 0x0054,
  0x0054, 0x0054, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055, 0x0056, 0x0056, 0x0056, 0x0056,
  0x0057, 0x0057, 0x0057, 0x0057, 0x0057, 0x0057, 0x0057, 0x0057, 0x0057, 0x0057, 0x0058, 0x0058, 0x0058, 0x0058, 0x0059, 0x0059,
  0x005A, 0x005A, 0x005A, 0x005A, 0x005A, 0x005A, 0x0048, 0x0054, 0x0057, 0x0059, 0x1E9A, 0x0053, 0x1E9C, 0x1E9D, 0x1E9E, 0x1E9F,
  0x0041, 0x0041, 0x0041, 0x0041, 0x0041, 0x0041, 0x0041, 0x0041, 0x0041, 0x0041, 0x0041, 0x0041, 0x0041, 0x0041, 0x0041, 0x0041,
  0x0041, 0x0041, 0x0041, 0x0041, 0x0041, 0x0041, 0x0041, 0x0041, 0x0045, 0x0045, 0x0045, 0x0045, 0x0045, 0x0045, 0x0045, 0x0045,
  0x0045, 0x0045, 0x0045, 0x0045, 0x0045, 0x0045, 0x0045, 0x0045, 0x0049, 0x0049, 0x0049, 0x0049, 0x004F, 0x004F, 0x004F, 0x004F,
  0x004F, 0x004F, 0x004F, 0x004F, 0x004F, 0x004F, 0x004F, 0x004F, 0x004F, 0x004F, 0x004F, 0x004F, 0x004F, 0x004F, 0x004F, 0x004F,
  0x004F, 0x004F, 0x004F, 0x004F, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055,
  0x0055, 0x0055, 0x0059, 0x0059, 0x0059, 0x0059, 0x0059, 0x0059, 0x0059, 0x0059, 0x1EFA, 0x1EFB, 0x1EFC, 0x1EFD, 0x1EFE, 0x1EFF
};

static const uint16_t plane1F[] = {
  0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x0391,
  0x0395, 0x0395, 0x0395, 0x0395, 0x0395, 0x0395, 0x1F16, 0x1F17, 0x0395, 0x0395, 0x0395, 0x0395, 0x0395, 0x0395, 0x1F1E, 0x1F1F,
  0x0397, 0x0397, 0x0397, 0x0397, 0x0397, 0x0397, 0x0397, 0x0397, 0x0397, 0x0397, 0x0397, 0x0397, 0x0397, 0x0397, 0x0397, 0x0397,
  0x0399, 0x0399, 0x0399, 0x0399, 0x0399, 0x0399, 0x0399, 0x0399, 0x0399, 0x0399, 0x0399, 0x0399, 0x0399, 0x0399, 0x0399, 0x0399,
  0x039F, 0x039F, 0x039F, 0x039F, 0x039F, 0x039F, 0x1F46, 0x1F47, 0x039F, 0x039F, 0x039F, 0x039F, 0x039F, 0x039F, 0x1F4E, 0x1F4F,
  0x03A5, 0x03A5, 0x03A5, 0x03A5, 0x03A5, 0x03A5, 0x03A5, 0x03A5, 0x1F58, 0x03A5, 0x1F5A, 0x03A5, 0x1F5C, 0x03A5, 0x1F5E, 0x03A5,
  0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9,
  0x0391, 0x1FBB, 0x0395, 0x1FC9, 0x0397, 0x1FCB, 0x0399, 0x1FDB, 0x039F, 0x1FF9, 0x03A5, 0x1FEB, 0x03A9, 0x1FFB, 0x1F7E, 0x1F7F,
  0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x0391,
  0x0397, 0x0397, 0x0397, 0x0397, 0x0397, 0x0397, 0x0397, 0x0397, 0x0397, 0x0397, 0x0397, 0x0397, 0x0397, 0x0397, 0x0397, 0x0397,
  0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9,
  0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x1FB5, 0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x1FBB, 0x0391, 0x1FBD, 0x0399, 0x1FBF,
  0x1FC0, 0x1FC1, 0x0397, 0x0397, 0x0397, 0x1FC5, 0x0397, 0x0397, 0x0395, 0x1FC9, 0x0397, 0x1FCB, 0x0397, 0x1FCD, 0x1FCE, 0x1FCF,
  0x0399, 0x0399, 0x0399, 0x1FD3, 0x1FD4, 0x1FD5, 0x0399, 0x0399, 0x0399, 0x0399, 0x0399, 0x1FDB, 0x1FDC, 0x1FDD, 0x1FDE, 0x1FDF,
  0x03A5, 0x03A5, 0x03A5, 0x1FE3, 0x03A1, 0x03A1, 0x03A5, 0x03A5, 0x03A5, 0x03A5, 0x03A5, 0x1FEB, 0x03A1, 0x1FED, 0x1FEE, 0x1FEF,
  0x1FF0, 0x1FF1, 0x03A9, 0x03A9, 0x03A9, 0x1FF5, 0x03A9, 0x03A9, 0x039F, 0x1FF9, 0x03A9, 0x1FFB, 0x03A9, 0x1FFD, 0x1FFE, 0x1FFF
};

static const uint16_t plane21[] = {
  0x2100, 0x2101, 0x2102, 0x2103, 0x2104, 0x2105, 0x2106, 0x2107, 0x2108, 0x2109, 0x210A, 0x210B, 0x210C, 0x210D, 0x210E, 0x210F,
  0x2110, 0x2111, 0x2112, 0x2113, 0x2114, 0x2115, 0x2116, 0x2117, 0x2118, 0x2119, 0x211A, 0x211B, 0x211C, 0x211D, 0x211E, 0x211F,
  0x2120, 0x2121, 0x2122, 0x2123, 0x2124, 0x2125, 0x2126, 0x2127, 0x2128, 0x2129, 0x212A, 0x212B, 0x212C, 0x212D, 0x212E, 0x212F,
  0x2130, 0x2131, 0x2132, 0x2133, 0x2134, 0x2135, 0x2136, 0x2137, 0x2138, 0x2139, 0x213A, 0x213B, 0x213C, 0x213D, 0x213E, 0x213F,
  0x2140, 0x2141, 0x2142, 0x2143, 0x2144, 0x2145, 0x2146, 0x2147, 0x2148, 0x2149, 0x214A, 0x214B, 0x214C, 0x214D, 0x214E, 0x214F,
  0x2150, 0x2151, 0x2152, 0x2153, 0x2154, 0x2155, 0x2156, 0x2157, 0x2158, 0x2159, 0x215A, 0x215B, 0x215C, 0x215D, 0x215E, 0x215F,
  0x2160, 0x2161, 0x2162, 0x2163, 0x2164, 0x2165, 0x2166, 0x2167, 0x2168, 0x2169, 0x216A, 0x216B, 0x216C, 0x216D, 0x216E, 0x216F,
  0x2160, 0x2161, 0x2162, 0x2163, 0x2164, 0x2165, 0x2166, 0x2167, 0x2168, 0x2169, 0x216A, 0x216B, 0x216C, 0x216D, 0x216E, 0x216F,
  0x2180, 0x2181, 0x2182, 0x2183, 0x2184, 0x2185, 0x2186, 0x2187, 0x2188, 0x2189, 0x218A, 0x218B, 0x218C, 0x218D, 0x218E, 0x218F,
  0x2190, 0x2191, 0x2192, 0x2193, 0x2194, 0x2195, 0x2196, 0x2197, 0x2198, 0x2199, 0x219A, 0x219B, 0x219C, 0x219D, 0x219E, 0x219F,
  0x21A0, 0x21A1, 0x21A2, 0x21A3, 0x21A4, 0x21A5, 0x21A6, 0x21A7, 0x21A8, 0x21A9, 0x21AA, 0x21AB, 0x21AC, 0x21AD, 0x21AE, 0x21AF,
  0x21B0, 0x21B1, 0x21B2, 0x21B3, 0x21B4, 0x21B5, 0x21B6, 0x21B7, 0x21B8, 0x21B9, 0x21BA, 0x21BB, 0x21BC, 0x21BD, 0x21BE, 0x21BF,
  0x21C0, 0x21C1, 0x21C2, 0x21C3, 0x21C4, 0x21C5, 0x21C6, 0x21C7, 0x21C8, 0x21C9, 0x21CA, 0x21CB, 0x21CC, 0x21CD, 0x21CE, 0x21CF,
  0x21D0, 0x21D1, 0x21D2, 0x21D3, 0x21D4, 0x21D5, 0x21D6, 0x21D7, 0x21D8, 0x21D9, 0x21DA, 0x21DB, 0x21DC, 0x21DD, 0x21DE, 0x21DF,
  0x21E0, 0x21E1, 0x21E2, 0x21E3, 0x21E4, 0x21E5, 0x21E6, 0x21E7, 0x21E8, 0x21E9, 0x21EA, 0x21EB, 0x21EC, 0x21ED, 0x21EE, 0x21EF,
  0x21F0, 0x21F1, 0x21F2, 0x21F3, 0x21F4, 0x21F5, 0x21F6, 0x21F7, 0x21F8, 0x21F9, 0x21FA, 0x21FB, 0x21FC, 0x21FD, 0x21FE, 0x21FF
};

static const uint16_t plane24[] = {
  0x2400, 0x2401, 0x2402, 0x2403, 0x2404, 0x2405, 0x2406, 0x2407, 0x2408, 0x2409, 0x240A, 0x240B, 0x240C, 0x240D, 0x240E, 0x240F,
  0x2410, 0x2411, 0x2412, 0x2413, 0x2414, 0x2415, 0x2416, 0x2417, 0x2418, 0x2419, 0x241A, 0x241B, 0x241C, 0x241D, 0x241E, 0x241F,
  0x2420, 0x2421, 0x2422, 0x2423, 0x2424, 0x2425, 0x2426, 0x2427, 0x2428, 0x2429, 0x242A, 0x242B, 0x242C, 0x242D, 0x242E, 0x242F,
  0x2430, 0x2431, 0x2432, 0x2433, 0x2434, 0x2435, 0x2436, 0x2437, 0x2438, 0x2439, 0x243A, 0x243B, 0x243C, 0x243D, 0x243E, 0x243F,
  0x2440, 0x2441, 0x2442, 0x2443, 0x2444, 0x2445, 0x2446, 0x2447, 0x2448, 0x2449, 0x244A, 0x244B, 0x244C, 0x244D, 0x244E, 0x244F,
  0x2450, 0x2451, 0x2452, 0x2453, 0x2454, 0x2455, 0x2456, 0x2457, 0x2458, 0x2459, 0x245A, 0x245B, 0x245C, 0x245D, 0x245E, 0x245F,
  0x2460, 0x2461, 0x2462, 0x2463, 0x2464, 0x2465, 0x2466, 0x2467, 0x2468, 0x2469, 0x246A, 0x246B, 0x246C, 0x246D, 0x246E, 0x246F,
  0x2470, 0x2471, 0x2472, 0x2473, 0x2474, 0x2475, 0x2476, 0x2477, 0x2478, 0x2479, 0x247A, 0x247B, 0x247C, 0x247D, 0x247E, 0x247F,
  0x2480, 0x2481, 0x2482, 0x2483, 0x2484, 0x2485, 0x2486, 0x2487, 0x2488, 0x2489, 0x248A, 0x248B, 0x248C, 0x248D, 0x248E, 0x248F,
  0x2490, 0x2491, 0x2492, 0x2493, 0x2494, 0x2495, 0x2496, 0x2497, 0x2498, 0x2499, 0x249A, 0x249B, 0x249C, 0x249D, 0x249E, 0x249F,
  0x24A0, 0x24A1, 0x24A2, 0x24A3, 0x24A4, 0x24A5, 0x24A6, 0x24A7, 0x24A8, 0x24A9, 0x24AA, 0x24AB, 0x24AC, 0x24AD, 0x24AE, 0x24AF,
  0x24B0, 0x24B1, 0x24B2, 0x24B3, 0x24B4, 0x24B5, 0x24B6, 0x24B7, 0x24B8, 0x24B9, 0x24BA, 0x24BB, 0x24BC, 0x24BD, 0x24BE, 0x24BF,
  0x24C0, 0x24C1, 0x24C2, 0x24C3, 0x24C4, 0x24C5, 0x24C6, 0x24C7, 0x24C8, 0x24C9, 0x24CA, 0x24CB, 0x24CC, 0x24CD, 0x24CE, 0x24CF,
  0x24B6, 0x24B7, 0x24B8, 0x24B9, 0x24BA, 0x24BB, 0x24BC, 0x24BD, 0x24BE, 0x24BF, 0x24C0, 0x24C1, 0x24C2, 0x24C3, 0x24C4, 0x24C5,
  0x24C6, 0x24C7, 0x24C8, 0x24C9, 0x24CA, 0x24CB, 0x24CC, 0x24CD, 0x24CE, 0x24CF, 0x24EA, 0x24EB, 0x24EC, 0x24ED, 0x24EE, 0x24EF,
  0x24F0, 0x24F1, 0x24F2, 0x24F3, 0x24F4, 0x24F5, 0x24F6, 0x24F7, 0x24F8, 0x24F9, 0x24FA, 0x24FB, 0x24FC, 0x24FD, 0x24FE, 0x24FF
};

static const uint16_t planeFF[] = {
  0xFF00, 0xFF01, 0xFF02, 0xFF03, 0xFF04, 0xFF05, 0xFF06, 0xFF07, 0xFF08, 0xFF09, 0xFF0A, 0xFF0B, 0xFF0C, 0xFF0D, 0xFF0E, 0xFF0F,
  0xFF10, 0xFF11, 0xFF12, 0xFF13, 0xFF14, 0xFF15, 0xFF16, 0xFF17, 0xFF18, 0xFF19, 0xFF1A, 0xFF1B, 0xFF1C, 0xFF1D, 0xFF1E, 0xFF1F,
  0xFF20, 0xFF21, 0xFF22, 0xFF23, 0xFF24, 0xFF25, 0xFF26, 0xFF27, 0xFF28, 0xFF29, 0xFF2A, 0xFF2B, 0xFF2C, 0xFF2D, 0xFF2E, 0xFF2F,
  0xFF30, 0xFF31, 0xFF32, 0xFF33, 0xFF34, 0xFF35, 0xFF36, 0xFF37, 0xFF38, 0xFF39, 0xFF3A, 0xFF3B, 0xFF3C, 0xFF3D, 0xFF3E, 0xFF3F,
  0xFF40, 0xFF21, 0xFF22, 0xFF23, 0xFF24, 0xFF25, 0xFF26, 0xFF27, 0xFF28, 0xFF29, 0xFF2A, 0xFF2B, 0xFF2C, 0xFF2D, 0xFF2E, 0xFF2F,
  0xFF30, 0xFF31, 0xFF32, 0xFF33, 0xFF34, 0xFF35, 0xFF36, 0xFF37, 0xFF38, 0xFF39, 0xFF3A, 0xFF5B, 0xFF5C, 0xFF5D, 0xFF5E, 0xFF5F,
  0xFF60, 0xFF61, 0xFF62, 0xFF63, 0xFF64, 0xFF65, 0xFF66, 0xFF67, 0xFF68, 0xFF69, 0xFF6A, 0xFF6B, 0xFF6C, 0xFF6D, 0xFF6E, 0xFF6F,
  0xFF70, 0xFF71, 0xFF72, 0xFF73, 0xFF74, 0xFF75, 0xFF76, 0xFF77, 0xFF78, 0xFF79, 0xFF7A, 0xFF7B, 0xFF7C, 0xFF7D, 0xFF7E, 0xFF7F,
  0xFF80, 0xFF81, 0xFF82, 0xFF83, 0xFF84, 0xFF85, 0xFF86, 0xFF87, 0xFF88, 0xFF89, 0xFF8A, 0xFF8B, 0xFF8C, 0xFF8D, 0xFF8E, 0xFF8F,
  0xFF90, 0xFF91, 0xFF92, 0xFF93, 0xFF94, 0xFF95, 0xFF96, 0xFF97, 0xFF98, 0xFF99, 0xFF9A, 0xFF9B, 0xFF9C, 0xFF9D, 0xFF9E, 0xFF9F,
  0xFFA0, 0xFFA1, 0xFFA2, 0xFFA3, 0xFFA4, 0xFFA5, 0xFFA6, 0xFFA7, 0xFFA8, 0xFFA9, 0xFFAA, 0xFFAB, 0xFFAC, 0xFFAD, 0xFFAE, 0xFFAF,
  0xFFB0, 0xFFB1, 0xFFB2, 0xFFB3, 0xFFB4, 0xFFB5, 0xFFB6, 0xFFB7, 0xFFB8, 0xFFB9, 0xFFBA, 0xFFBB, 0xFFBC, 0xFFBD, 0xFFBE, 0xFFBF,
  0xFFC0, 0xFFC1, 0xFFC2, 0xFFC3, 0xFFC4, 0xFFC5, 0xFFC6, 0xFFC7, 0xFFC8, 0xFFC9, 0xFFCA, 0xFFCB, 0xFFCC, 0xFFCD, 0xFFCE, 0xFFCF,
  0xFFD0, 0xFFD1, 0xFFD2, 0xFFD3, 0xFFD4, 0xFFD5, 0xFFD6, 0xFFD7, 0xFFD8, 0xFFD9, 0xFFDA, 0xFFDB, 0xFFDC, 0xFFDD, 0xFFDE, 0xFFDF,
  0xFFE0, 0xFFE1, 0xFFE2, 0xFFE3, 0xFFE4, 0xFFE5, 0xFFE6, 0xFFE7, 0xFFE8, 0xFFE9, 0xFFEA, 0xFFEB, 0xFFEC, 0xFFED, 0xFFEE, 0xFFEF,
  0xFFF0, 0xFFF1, 0xFFF2, 0xFFF3, 0xFFF4, 0xFFF5, 0xFFF6, 0xFFF7, 0xFFF8, 0xFFF9, 0xFFFA, 0xFFFB, 0xFFFC, 0xFFFD, 0xFFFE, 0xFFFF
};

static const uint16_t* const planemap[256] = {
    plane00, plane01, plane02, plane03, plane04, plane05, NULL, NULL, NULL,    NULL,    NULL,
    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, NULL, NULL,    NULL,    NULL,
    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, NULL, plane1E, plane1F, NULL,
    plane21, NULL,    NULL,    plane24, NULL,    NULL,    NULL, NULL, NULL,    NULL,    NULL,
    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, NULL, NULL,    NULL,    NULL,
    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, NULL, NULL,    NULL,    NULL,
    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, NULL, NULL,    NULL,    NULL,
    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, NULL, NULL,    NULL,    NULL,
    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, NULL, NULL,    NULL,    NULL,
    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, NULL, NULL,    NULL,    NULL,
    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, NULL, NULL,    NULL,    NULL,
    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, NULL, NULL,    NULL,    NULL,
    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, NULL, NULL,    NULL,    NULL,
    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, NULL, NULL,    NULL,    NULL,
    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, NULL, NULL,    NULL,    NULL,
    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, NULL, NULL,    NULL,    NULL,
    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, NULL, NULL,    NULL,    NULL,
    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, NULL, NULL,    NULL,    NULL,
    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, NULL, NULL,    NULL,    NULL,
    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, NULL, NULL,    NULL,    NULL,
    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, NULL, NULL,    NULL,    NULL,
    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, NULL, NULL,    NULL,    NULL,
    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, NULL, NULL,    NULL,    NULL,
    NULL,    NULL,    planeFF
};
// clang-format on

static wchar_t GetCollationWeight(const wchar_t& r)
{
  // Lookup the "weight" of a UTF8 char, equivalent lowercase ascii letter, in the plane map,
  // the character comparison value used by using "accent folding" collation utf8_general_ci
  // in MySQL (AKA utf8mb3_general_ci in MariaDB 10)
  auto index = r >> 8;
  if (index > 255)
    return 0xFFFD;
  auto plane = planemap[index];
  if (plane == nullptr)
    return r;
  return static_cast<wchar_t>(plane[r & 0xFF]);
}

// Compares separately the numeric and alphabetic parts of a wide string.
// returns negative if left < right, positive if left > right
// and 0 if they are identical.
// See also the equivalent StringUtils::AlphaNumericCollation() for UFT8 data
int64_t StringUtils::AlphaNumericCompare(const wchar_t* left, const wchar_t* right)
{
  const wchar_t *l = left;
  const wchar_t *r = right;
  const wchar_t *ld, *rd;
  wchar_t lc, rc;
  int64_t lnum, rnum;
  bool lsym, rsym;
  while (*l != 0 && *r != 0)
  {
    // check if we have a numerical value
    if (*l >= L'0' && *l <= L'9' && *r >= L'0' && *r <= L'9')
    {
      ld = l;
      lnum = *ld++ - L'0';
      while (*ld >= L'0' && *ld <= L'9' && ld < l + 15)
      { // compare only up to 15 digits
        lnum *= 10;
        lnum += *ld++ - L'0';
      }
      rd = r;
      rnum = *rd++ - L'0';
      while (*rd >= L'0' && *rd <= L'9' && rd < r + 15)
      { // compare only up to 15 digits
        rnum *= 10;
        rnum += *rd++ - L'0';
      }
      // do we have numbers?
      if (lnum != rnum)
      { // yes - and they're different!
        return lnum - rnum;
      }
      l = ld;
      r = rd;
      continue;
    }

    lc = *l;
    rc = *r;
    // Put ascii punctuation and symbols e.g. !#$&()*+,-./:;<=>?@[\]^_ `{|}~ above the other
    // alphanumeric ascii, rather than some being mixed between the numbers and letters, and
    // above all other unicode letters, symbols and punctuation.
    // (Locale collation of these chars varies across platforms)
    lsym = (lc >= 32 && lc < L'0') || (lc > L'9' && lc < L'A') || (lc > L'Z' && lc < L'a') ||
           (lc > L'z' && lc < 128);
    rsym = (rc >= 32 && rc < L'0') || (rc > L'9' && rc < L'A') || (rc > L'Z' && rc < L'a') ||
           (rc > L'z' && rc < 128);
    if (lsym && !rsym)
      return -1;
    if (!lsym && rsym)
      return 1;
    if (lsym && rsym)
    {
      if (lc != rc)
        return lc - rc;
      else
      { // Same symbol advance to next wchar
        l++;
        r++;
        continue;
      }
    }
    if (!g_langInfo.UseLocaleCollation())
    {
      // Apply case sensitive accent folding collation to non-ascii chars.
      // This mimics utf8_general_ci collation, and provides simple collation of LATIN-1 chars
      // for any platformthat doesn't have a language specific collate facet implemented
      if (lc > 128)
        lc = GetCollationWeight(lc);
      if (rc > 128)
        rc = GetCollationWeight(rc);
    }
    // Do case less comparison, convert ascii upper case to lower case
    if (lc >= L'A' && lc <= L'Z')
      lc += L'a' - L'A';
    if (rc >= L'A' && rc <= L'Z')
      rc += L'a' - L'A';

    if (lc != rc)
    {
      if (!g_langInfo.UseLocaleCollation())
      {
        // Compare unicode (having applied accent folding collation to non-ascii chars).
        int i = wcsncmp(&lc, &rc, 1);
        return i;
      }
      else
      {
        // Fetch collation facet from locale to do comparison of wide char although on some
        // platforms this is not language specific but just compares unicode
        const std::collate<wchar_t>& coll =
            std::use_facet<std::collate<wchar_t>>(g_langInfo.GetSystemLocale());
        int cmp_res = coll.compare(&lc, &lc + 1, &rc, &rc + 1);
        if (cmp_res != 0)
          return cmp_res;
      }
    }
    l++; r++;
  }
  if (*r)
  { // r is longer
    return -1;
  }
  else if (*l)
  { // l is longer
    return 1;
  }
  return 0; // files are the same
}

/*
  Convert the UTF8 character to which z points into a 31-bit Unicode point.
  Return how many bytes (0 to 3) of UTF8 data encode the character.
  This only works right if z points to a well-formed UTF8 string.
  Byte-0    Byte-1    Byte-2    Byte-3     Value
  0xxxxxxx                                 00000000 00000000 0xxxxxxx
  110yyyyy  10xxxxxx                       00000000 00000yyy yyxxxxxx
  1110zzzz  10yyyyyy  10xxxxxx             00000000 zzzzyyyy yyxxxxxx
  11110uuu  10uuzzzz  10yyyyyy  10xxxxxx   000uuuuu zzzzyyyy yyxxxxxx
*/
static uint32_t UTF8ToUnicode(const unsigned char* z, int nKey, unsigned char& bytes)
{
  // Lookup table used decode the first byte of a multi-byte UTF8 character
  // clang-format off
  static const unsigned char utf8Trans1[] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x00, 0x01, 0x02, 0x03, 0x00, 0x01, 0x00, 0x00,
  };
  // clang-format on

  uint32_t c;
  bytes = 0;
  c = z[0];
  if (c >= 0xc0)
  {
    c = utf8Trans1[c - 0xc0];
    int index = 1;
    while (index < nKey && (z[index] & 0xc0) == 0x80)
    {
      c = (c << 6) + (0x3f & z[index]);
      index++;
    }
    if (c < 0x80 || (c & 0xFFFFF800) == 0xD800 || (c & 0xFFFFFFFE) == 0xFFFE)
      c = 0xFFFD;
    bytes = static_cast<unsigned char>(index - 1);
  }
  return c;
}

/*
  SQLite collating function, see sqlite3_create_collation
  The equivalent of AlphaNumericCompare() but for comparing UTF8 encoded data

  This only processes enough data to find a difference, and avoids expensive data conversions.
  When sorting in memory item data is converted once to wstring in advance prior to sorting, the
  SQLite callback function can not do that kind of preparation. Instead, in order to use
  AlphaNumericCompare(), it would have to repeatedly convert the full input data to wstring for
  every pair comparison made. That approach was found to be 10 times slower than using this
  separate routine.
*/
int StringUtils::AlphaNumericCollation(int nKey1, const void* pKey1, int nKey2, const void* pKey2)
{
  // Get exact matches of shorter text to start of larger test fast
  int n = std::min(nKey1, nKey2);
  int r = memcmp(pKey1, pKey2, n);
  if (r == 0)
    return nKey1 - nKey2;

  //Not a binary match, so process character at a time
  const unsigned char* zA = static_cast<const unsigned char*>(pKey1);
  const unsigned char* zB = static_cast<const unsigned char*>(pKey2);
  wchar_t lc;
  wchar_t rc;
  unsigned char bytes;
  int64_t lnum;
  int64_t rnum;
  bool lsym;
  bool rsym;
  int ld;
  int rd;
  int i = 0;
  int j = 0;
  // Looping Unicode point at a time through potentially 1 to 4 multi-byte encoded UTF8 data
  while (i < nKey1 && j < nKey2)
  {
    // Check if we have numerical values, compare only up to 15 digits
    if (isdigit(zA[i]) && isdigit(zB[j]))
    {
      lnum = zA[i] - '0';
      ld = i + 1;
      while (ld < nKey1 && isdigit(zA[ld]) && ld < i + 15)
      {
        lnum *= 10;
        lnum += zA[ld] - '0';
        ld++;
      }
      rnum = zB[j] - '0';
      rd = j + 1;
      while (rd < nKey2 && isdigit(zB[rd]) && rd < j + 15)
      {
        rnum *= 10;
        rnum += zB[rd] - '0';
        rd++;
      }
      // do we have numbers?
      if (lnum != rnum)
      { // yes - and they're different!
        return static_cast<int>(lnum - rnum);
      }
      // Advance to after digits
      i = ld;
      j = rd;
      continue;
    }
    // Put ascii punctuation and symbols e.g. !#$&()*+,-./:;<=>?@[\]^_ `{|}~ before the other
    // alphanumeric ascii, rather than some being mixed between the numbers and letters, and
    // above all other unicode letters, symbols and punctuation.
    // (Locale collation of these chars varies across platforms)
    lsym = (zA[i] >= 32 && zA[i] < '0') || (zA[i] > '9' && zA[i] < 'A') ||
           (zA[i] > 'Z' && zA[i] < 'a') || (zA[i] > 'z' && zA[i] < 128);
    rsym = (zB[j] >= 32 && zB[j] < '0') || (zB[j] > '9' && zB[j] < 'A') ||
           (zB[j] > 'Z' && zB[j] < 'a') || (zB[j] > 'z' && zB[j] < 128);
    if (lsym && !rsym)
      return -1;
    if (!lsym && rsym)
      return 1;
    if (lsym && rsym)
    {
      if (zA[i] != zB[j])
        return zA[i] - zB[j];
      else
      { // Same symbol advance to next
        i++;
        j++;
        continue;
      }
    }
    //Decode single (1 to 4 bytes) UTF8 character to Unicode
    lc = UTF8ToUnicode(&zA[i], nKey1 - i, bytes);
    i += bytes;
    rc = UTF8ToUnicode(&zB[j], nKey2 - j, bytes);
    j += bytes;
    if (!g_langInfo.UseLocaleCollation())
    {
      // Apply case sensitive accent folding collation to non-ascii chars.
      // This mimics utf8_general_ci collation, and provides simple collation of LATIN-1 chars
      // for any platform that doesn't have a language specific collate facet implemented
      if (lc > 128)
        lc = GetCollationWeight(lc);
      if (rc > 128)
        rc = GetCollationWeight(rc);
    }
    // Caseless comparison so convert ascii upper case to lower case
    if (lc >= 'A' && lc <= 'Z')
      lc += 'a' - 'A';
    if (rc >= 'A' && rc <= 'Z')
      rc += 'a' - 'A';

    if (lc != rc)
    {
      if (!g_langInfo.UseLocaleCollation() || (lc <= 128 && rc <= 128))
        // Compare unicode (having applied accent folding collation to non-ascii chars).
        return lc - rc;
      else
      {
        // Fetch collation facet from locale to do comparison of wide char although on some
        // platforms this is not language specific but just compares unicode
        const std::collate<wchar_t>& coll =
            std::use_facet<std::collate<wchar_t>>(g_langInfo.GetSystemLocale());
        int cmp_res = coll.compare(&lc, &lc + 1, &rc, &rc + 1);
        if (cmp_res != 0)
          return cmp_res;
      }
    }
    i++;
    j++;
  }
  // Compared characters of shortest are the same as longest, length determines order
  return (nKey1 - nKey2);
}

int StringUtils::DateStringToYYYYMMDD(const std::string &dateString)
{
  std::vector<std::string> days = StringUtils::Split(dateString, '-');
  if (days.size() == 1)
    return atoi(days[0].c_str());
  else if (days.size() == 2)
    return atoi(days[0].c_str())*100+atoi(days[1].c_str());
  else if (days.size() == 3)
    return atoi(days[0].c_str())*10000+atoi(days[1].c_str())*100+atoi(days[2].c_str());
  else
    return -1;
}

std::string StringUtils::ISODateToLocalizedDate(const std::string& strIsoDate)
{
  // Convert ISO8601 date strings YYYY, YYYY-MM, or YYYY-MM-DD to (partial) localized date strings
  CDateTime date;
  std::string formattedDate = strIsoDate;
  if (formattedDate.size() == 10)
  {
    date.SetFromDBDate(strIsoDate);
    formattedDate = date.GetAsLocalizedDate();
  }
  else if (formattedDate.size() == 7)
  {
    std::string strFormat = date.GetAsLocalizedDate(false);
    std::string tempdate;
    // find which date separator we are using.  Can be -./
    size_t pos = strFormat.find_first_of("-./");
    if (pos != std::string::npos)
    {
      bool yearFirst = strFormat.find("1601") == 0; // true if year comes first
      std::string sep = strFormat.substr(pos, 1);
      if (yearFirst)
      { // build formatted date with year first, then separator and month
        tempdate = formattedDate.substr(0, 4);
        tempdate += sep;
        tempdate += formattedDate.substr(5, 2);
      }
      else
      {
        tempdate = formattedDate.substr(5, 2);
        tempdate += sep;
        tempdate += formattedDate.substr(0, 4);
      }
      formattedDate = tempdate;
    }
  // return either just the year or the locally formatted version of the ISO date
  }
  return formattedDate;
}

long StringUtils::TimeStringToSeconds(const std::string &timeString)
{
  std::string strCopy(timeString);
  StringUtils::Trim(strCopy);
  if(StringUtils::EndsWithNoCase(strCopy, " min"))
  {
    // this is imdb format of "XXX min"
    return 60 * atoi(strCopy.c_str());
  }
  else
  {
    std::vector<std::string> secs = StringUtils::Split(strCopy, ':');
    int timeInSecs = 0;
    for (unsigned int i = 0; i < 3 && i < secs.size(); i++)
    {
      timeInSecs *= 60;
      timeInSecs += atoi(secs[i].c_str());
    }
    return timeInSecs;
  }
}

std::string StringUtils::SecondsToTimeString(long lSeconds, TIME_FORMAT format)
{
  bool isNegative = lSeconds < 0;
  lSeconds = std::abs(lSeconds);

  std::string strHMS;
  if (format == TIME_FORMAT_SECS)
    strHMS = std::to_string(lSeconds);
  else if (format == TIME_FORMAT_MINS)
    strHMS = std::to_string(lrintf(static_cast<float>(lSeconds) / 60.0f));
  else if (format == TIME_FORMAT_HOURS)
    strHMS = std::to_string(lrintf(static_cast<float>(lSeconds) / 3600.0f));
  else if (format & TIME_FORMAT_M)
    strHMS += std::to_string(lSeconds % 3600 / 60);
  else
  {
    int hh = lSeconds / 3600;
    lSeconds = lSeconds % 3600;
    int mm = lSeconds / 60;
    int ss = lSeconds % 60;

    if (format == TIME_FORMAT_GUESS)
      format = (hh >= 1) ? TIME_FORMAT_HH_MM_SS : TIME_FORMAT_MM_SS;
    if (format & TIME_FORMAT_HH)
      strHMS += StringUtils::Format("{:02}", hh);
    else if (format & TIME_FORMAT_H)
      strHMS += std::to_string(hh);
    if (format & TIME_FORMAT_MM)
      strHMS += StringUtils::Format(strHMS.empty() ? "{:02}" : ":{:02}", mm);
    if (format & TIME_FORMAT_SS)
      strHMS += StringUtils::Format(strHMS.empty() ? "{:02}" : ":{:02}", ss);
  }

  if (isNegative)
    strHMS = "-" + strHMS;

  return strHMS;
}

bool StringUtils::IsNaturalNumber(const std::string& str)
{
  size_t i = 0;
  size_t n = 0;
  // allow whitespace,digits,whitespace
  while (i < str.size() && isspace((unsigned char) str[i]))
    i++;
  while (i < str.size() && isdigit((unsigned char) str[i]))
  {
    i++; n++;
  }
  while (i < str.size() && isspace((unsigned char) str[i]))
    i++;
  return i == str.size() && n > 0;
}

bool StringUtils::IsInteger(const std::string& str)
{
  size_t i = 0;
  size_t n = 0;
  // allow whitespace,-,digits,whitespace
  while (i < str.size() && isspace((unsigned char) str[i]))
    i++;
  if (i < str.size() && str[i] == '-')
    i++;
  while (i < str.size() && isdigit((unsigned char) str[i]))
  {
    i++; n++;
  }
  while (i < str.size() && isspace((unsigned char) str[i]))
    i++;
  return i == str.size() && n > 0;
}

int StringUtils::asciidigitvalue(char chr)
{
  if (!isasciidigit(chr))
    return -1;

  return chr - '0';
}

int StringUtils::asciixdigitvalue(char chr)
{
  int v = asciidigitvalue(chr);
  if (v >= 0)
    return v;
  if (chr >= 'a' && chr <= 'f')
    return chr - 'a' + 10;
  if (chr >= 'A' && chr <= 'F')
    return chr - 'A' + 10;

  return -1;
}


void StringUtils::RemoveCRLF(std::string& strLine)
{
  StringUtils::TrimRight(strLine, "\n\r");
}

std::string StringUtils::SizeToString(int64_t size)
{
  std::string strLabel;
  constexpr std::array<char, 9> prefixes = {' ', 'k', 'M', 'G', 'T', 'P', 'E', 'Z', 'Y'};
  unsigned int i = 0;
  double s = (double)size;
  while (i < prefixes.size() && s >= 1000.0)
  {
    s /= 1024.0;
    i++;
  }

  if (!i)
    strLabel = StringUtils::Format("{:.2f} B", s);
  else if (i == prefixes.size())
  {
    if (s >= 1000.0)
      strLabel = StringUtils::Format(">999.99 {}B", prefixes[i - 1]);
    else
      strLabel = StringUtils::Format("{:.2f} {}B", s, prefixes[i - 1]);
  }
  else if (s >= 100.0)
    strLabel = StringUtils::Format("{:.1f} {}B", s, prefixes[i]);
  else
    strLabel = StringUtils::Format("{:.2f} {}B", s, prefixes[i]);

  return strLabel;
}

std::string StringUtils::BinaryStringToString(const std::string& in)
{
  std::string out;
  out.reserve(in.size() / 2);
  for (const char *cur = in.c_str(), *end = cur + in.size(); cur != end; ++cur) {
    if (*cur == '\\') {
      ++cur;
      if (cur == end) {
        break;
      }
      if (isdigit(*cur)) {
        char* end;
        unsigned long num = strtol(cur, &end, 10);
        cur = end - 1;
        out.push_back(num);
        continue;
      }
    }
    out.push_back(*cur);
  }
  return out;
}

std::string StringUtils::ToHexadecimal(const std::string& in)
{
  std::ostringstream ss;
  ss << std::hex;
  for (unsigned char ch : in) {
    ss << std::setw(2) << std::setfill('0') << static_cast<unsigned long> (ch);
  }
  return ss.str();
}

// return -1 if not, else return the utf8 char length.
int IsUTF8Letter(const unsigned char *str)
{
  // reference:
  // unicode -> utf8 table: http://www.utf8-chartable.de/
  // latin characters in unicode: http://en.wikipedia.org/wiki/Latin_characters_in_Unicode
  unsigned char ch = str[0];
  if (!ch)
    return -1;
  if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z'))
    return 1;
  if (!(ch & 0x80))
    return -1;
  unsigned char ch2 = str[1];
  if (!ch2)
    return -1;
  // check latin 1 letter table: http://en.wikipedia.org/wiki/C1_Controls_and_Latin-1_Supplement
  if (ch == 0xC3 && ch2 >= 0x80 && ch2 <= 0xBF && ch2 != 0x97 && ch2 != 0xB7)
    return 2;
  // check latin extended A table: http://en.wikipedia.org/wiki/Latin_Extended-A
  if (ch >= 0xC4 && ch <= 0xC7 && ch2 >= 0x80 && ch2 <= 0xBF)
    return 2;
  // check latin extended B table: http://en.wikipedia.org/wiki/Latin_Extended-B
  // and International Phonetic Alphabet: http://en.wikipedia.org/wiki/IPA_Extensions_(Unicode_block)
  if (((ch == 0xC8 || ch == 0xC9) && ch2 >= 0x80 && ch2 <= 0xBF)
      || (ch == 0xCA && ch2 >= 0x80 && ch2 <= 0xAF))
    return 2;
  return -1;
}

size_t StringUtils::FindWords(const char *str, const char *wordLowerCase)
{
  // NOTE: This assumes word is lowercase!
  const unsigned char *s = (const unsigned char *)str;
  do
  {
    // start with a compare
    const unsigned char *c = s;
    const unsigned char *w = (const unsigned char *)wordLowerCase;
    bool same = true;
    while (same && *c && *w)
    {
      unsigned char lc = *c++;
      if (lc >= 'A' && lc <= 'Z')
        lc += 'a'-'A';

      if (lc != *w++) // different
        same = false;
    }
    if (same && *w == 0)  // only the same if word has been exhausted
      return (const char *)s - str;

    // otherwise, skip current word (composed by latin letters) or number
    int l;
    if (*s >= '0' && *s <= '9')
    {
      ++s;
      while (*s >= '0' && *s <= '9') ++s;
    }
    else if ((l = IsUTF8Letter(s)) > 0)
    {
      s += l;
      while ((l = IsUTF8Letter(s)) > 0) s += l;
    }
    else
      ++s;
    while (*s && *s == ' ') s++;

    // and repeat until we're done
  } while (*s);

  return std::string::npos;
}

// assumes it is called from after the first open bracket is found
int StringUtils::FindEndBracket(const std::string &str, char opener, char closer, int startPos)
{
  int blocks = 1;
  for (unsigned int i = startPos; i < str.size(); i++)
  {
    if (str[i] == opener)
      blocks++;
    else if (str[i] == closer)
    {
      blocks--;
      if (!blocks)
        return i;
    }
  }

  return (int)std::string::npos;
}

void StringUtils::WordToDigits(std::string &word)
{

  // Works ONLY with ASCII!

  static const char word_to_letter[] = "22233344455566677778889999";
  std::string digits = StringUtils::FoldCase(word);
  for (unsigned int i = 0; i < digits.size(); ++i)
  { // NB: This assumes ascii, which probably needs extending at some  point.
    char letter = digits[i];
    if (letter > 0x7f)
    {
      CLog::Log(LOGWARNING, "StringUtils.WordToDigits: Non-ASCII input: {}\n", word);
    }

    if ((letter >= 'a' && letter <= 'z')) // assume contiguous letter range
    {
      digits[i] = word_to_letter[letter - 'a'];
    }
    else if (letter < '0' || letter > '9') // We want to keep 0-9!
    {
      digits[i] = ' '; // replace everything else with a space
    }
  }
  digits.swap(word);
}

std::string StringUtils::CreateUUID()
{
#ifdef HAVE_NEW_CROSSGUID
#ifdef TARGET_ANDROID
  JNIEnv* env = xbmc_jnienv();
  return xg::newGuid(env).str();
#else
  return xg::newGuid().str();
#endif /* TARGET_ANDROID */
#else
  static GuidGenerator guidGenerator;
  auto guid = guidGenerator.newGuid();

  std::stringstream strGuid; strGuid << guid;
  return strGuid.str();
#endif
}

bool StringUtils::ValidateUUID(const std::string &uuid)
{
  CRegExp guidRE;
  guidRE.RegComp(ADDON_GUID_RE);
  return (guidRE.RegFind(uuid.c_str()) == 0);
}

double StringUtils::CompareFuzzy(const std::string &left, const std::string &right)
{
  return (0.5 + fstrcmp(left.c_str(), right.c_str()) * (left.length() + right.length())) / 2.0;
}

int StringUtils::FindBestMatch(const std::string &str, const std::vector<std::string> &strings, double &matchscore)
{
  int best = -1;
  matchscore = 0;

  int i = 0;
  for (std::vector<std::string>::const_iterator it = strings.begin(); it != strings.end(); ++it, i++)
  {
    int maxlength = std::max(str.length(), it->length());
    double score = StringUtils::CompareFuzzy(str, *it) / maxlength;
    if (score > matchscore)
    {
      matchscore = score;
      best = i;
    }
  }
  return best;
}

bool StringUtils::ContainsKeyword(const std::string &str, const std::vector<std::string> &keywords)
{
  for (std::vector<std::string>::const_iterator it = keywords.begin(); it != keywords.end(); ++it)
  {
    if (str.find(*it) != str.npos)
      return true;
  }
  return false;
}

size_t StringUtils::utf8_strlen(const char *s)
{
  size_t length = 0;
  while (*s)
  {
    if ((*s++ & 0xC0) != 0x80)
      length++;
  }
  return length;
}

std::string StringUtils::Paramify(const std::string &param)
{
  std::string result = param;
  // escape backspaces
  StringUtils::Replace(result, "\\", "\\\\");
  // escape double quotes
  StringUtils::Replace(result, "\"", "\\\"");

  // add double quotes around the whole string
  return "\"" + result + "\"";
}

std::vector<std::string> StringUtils::Tokenize(const std::string &input, const std::string &delimiters)
{
  std::vector<std::string> tokens;
  Tokenize(input, tokens, delimiters);
  return tokens;
}

void StringUtils::Tokenize(const std::string& input, std::vector<std::string>& tokens, const std::string& delimiters)
{
  tokens.clear();
  // Skip delimiters at beginning.
  std::string::size_type dataPos = input.find_first_not_of(delimiters);
  while (dataPos != std::string::npos)
  {
    // Find next delimiter
    const std::string::size_type nextDelimPos = input.find_first_of(delimiters, dataPos);
    // Found a token, add it to the vector.
    tokens.push_back(input.substr(dataPos, nextDelimPos - dataPos));
    // Skip delimiters.  Note the "not_of"
    dataPos = input.find_first_not_of(delimiters, nextDelimPos);
  }
}

std::vector<std::string> StringUtils::Tokenize(const std::string &input, const char delimiter)
{
  std::vector<std::string> tokens;
  Tokenize(input, tokens, delimiter);
  return tokens;
}

void StringUtils::Tokenize(const std::string& input, std::vector<std::string>& tokens, const char delimiter)
{
  tokens.clear();
  // Skip delimiters at beginning.
  std::string::size_type dataPos = input.find_first_not_of(delimiter);
  while (dataPos != std::string::npos)
  {
    // Find next delimiter
    const std::string::size_type nextDelimPos = input.find(delimiter, dataPos);
    // Found a token, add it to the vector.
    tokens.push_back(input.substr(dataPos, nextDelimPos - dataPos));
    // Skip delimiters.  Note the "not_of"
    dataPos = input.find_first_not_of(delimiter, nextDelimPos);
  }
}

uint32_t StringUtils::ToUint32(std::string_view str, uint32_t fallback /* = 0 */) noexcept
{
  return NumberFromSS(str, fallback);
}

uint64_t StringUtils::ToUint64(std::string_view str, uint64_t fallback /* = 0 */) noexcept
{
  return NumberFromSS(str, fallback);
}

float StringUtils::ToFloat(std::string_view str, float fallback /* = 0.0f */) noexcept
{
  return NumberFromSS(str, fallback);
}

std::string StringUtils::FormatFileSize(uint64_t bytes)
{
  const std::array<std::string, 6> units{{"B", "kB", "MB", "GB", "TB", "PB"}};
  if (bytes < 1000)
    return Format("{}B", bytes);

  size_t i = 0;
  double value = static_cast<double>(bytes);
  while (i + 1 < units.size() && value >= 999.5)
  {
    ++i;
    value /= 1024.0;
  }
  unsigned int decimals = value < 9.995 ? 2 : (value < 99.95 ? 1 : 0);
  return Format("{:.{}f}{}", value, decimals, units[i]);
}

const std::locale& StringUtils::GetOriginalLocale() noexcept
{
  return g_langInfo.GetOriginalLocale();
}

std::string StringUtils::CreateFromCString(const char* cstr)
{
  return cstr != nullptr ? std::string(cstr) : std::string();
}
