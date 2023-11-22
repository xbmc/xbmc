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

#include <algorithm>
#include <array>
#include <assert.h>
#include <functional>
#include <inttypes.h>
#include <iomanip>
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

//	Copyright (c) Leigh Brasington 2012.  All rights reserved.
//  This code may be used and reproduced without written permission.
//  http://www.leighb.com/tounicupper.htm
//
//	The tables were constructed from
//	http://publib.boulder.ibm.com/infocenter/iseries/v7r1m0/index.jsp?topic=%2Fnls%2Frbagslowtoupmaptable.htm

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

int compareWchar (const void* a, const void* b)
{
  if (*(const wchar_t*)a <  *(const wchar_t*)b)
    return -1;
  else if (*(const wchar_t*)a >  *(const wchar_t*)b)
    return 1;
  return 0;
}

wchar_t tolowerUnicode(const wchar_t& c)
{
  wchar_t* p = (wchar_t*) bsearch (&c, unicode_uppers, sizeof(unicode_uppers) / sizeof(wchar_t), sizeof(wchar_t), compareWchar);
  if (p)
    return *(unicode_lowers + (p - unicode_uppers));

  return c;
}

wchar_t toupperUnicode(const wchar_t& c)
{
  wchar_t* p = (wchar_t*) bsearch (&c, unicode_lowers, sizeof(unicode_lowers) / sizeof(wchar_t), sizeof(wchar_t), compareWchar);
  if (p)
    return *(unicode_uppers + (p - unicode_lowers));

  return c;
}

template<typename Str, typename Fn>
void transformString(const Str& input, Str& output, Fn fn)
{
  std::transform(input.begin(), input.end(), output.begin(), fn);
}

std::string StringUtils::ToUpper(const std::string& str)
{
  std::string result(str.size(), '\0');
  transformString(str, result, ::toupper);
  return result;
}

std::wstring StringUtils::ToUpper(const std::wstring& str)
{
  std::wstring result(str.size(), '\0');
  transformString(str, result, toupperUnicode);
  return result;
}

void StringUtils::ToUpper(std::string &str)
{
  transformString(str, str, ::toupper);
}

void StringUtils::ToUpper(std::wstring &str)
{
  transformString(str, str, toupperUnicode);
}

std::string StringUtils::ToLower(const std::string& str)
{
  std::string result(str.size(), '\0');
  transformString(str, result, ::tolower);
  return result;
}

std::wstring StringUtils::ToLower(const std::wstring& str)
{
  std::wstring result(str.size(), '\0');
  transformString(str, result, tolowerUnicode);
  return result;
}

void StringUtils::ToLower(std::string &str)
{
  transformString(str, str, ::tolower);
}

void StringUtils::ToLower(std::wstring &str)
{
  transformString(str, str, tolowerUnicode);
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

bool StringUtils::EqualsNoCase(const std::string &str1, const std::string &str2)
{
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
    if (c1 != c2 && ::tolower(c1) != ::tolower(c2)) // This includes the possibility that one of the characters is the null-terminator, which implies a string mismatch.
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
    if (c1 != c2 && ::tolower(c1) != ::tolower(c2)) // This includes the possibility that one of the characters is the null-terminator, which implies a string mismatch.
      return ::tolower(c1) - ::tolower(c2);
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

bool StringUtils::IsSpecialCharacter(char c)
{
  static constexpr std::string_view view(" .-_+,!'\"\t/\\*?#$%&@()[]{}");
  if (std::any_of(view.begin(), view.end(), [c](char ch) { return ch == c; }))
    return true;
  else
    return false;
}

std::string StringUtils::ReplaceSpecialCharactersWithSpace(const std::string& str)
{
  std::string result;
  bool prevCharWasSpecial = false;

  for (char c : str)
  {
    if (IsSpecialCharacter(c))
    {
      if (!prevCharWasSpecial)
      {
        result += ' ';
      }
      prevCharWasSpecial = true;
    }
    else
    {
      result += c;
      prevCharWasSpecial = false;
    }
  }
  return result;
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
    if (::tolower(*s1) != ::tolower(*s2))
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
    if (::tolower(*s1) != ::tolower(*s2))
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
    if (::tolower(*s1) != ::tolower(*s2))
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
        return static_cast<int64_t>(lc) - static_cast<int64_t>(rc);
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
  wchar_t lc, rc;
  unsigned char bytes;
  int64_t lnum, rnum;
  bool lsym, rsym;
  int ld, rd;
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
        return static_cast<int>(zA[i]) - static_cast<int>(zB[j]);
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
        return static_cast<int>(lc) - static_cast<int>(rc);
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
  size_t i = 0, n = 0;
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
  size_t i = 0, n = 0;
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
  static const char word_to_letter[] = "22233344455566677778889999";
  StringUtils::ToLower(word);
  for (unsigned int i = 0; i < word.size(); ++i)
  { // NB: This assumes ascii, which probably needs extending at some  point.
    char letter = word[i];
    if ((letter >= 'a' && letter <= 'z')) // assume contiguous letter range
    {
      word[i] = word_to_letter[letter-'a'];
    }
    else if (letter < '0' || letter > '9') // We want to keep 0-9!
    {
      word[i] = ' ';  // replace everything else with a space
    }
  }
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

std::string StringUtils::DeParamify(const std::string& param)
{
  std::string result = param;

  // remove double quotes around the whole string
  if (StringUtils::StartsWith(result, "\"") && StringUtils::EndsWith(result, "\""))
  {
    result.erase(0, 1);
    result.pop_back();

    // unescape double quotes
    StringUtils::Replace(result, "\\\"", "\"");

    // unescape backspaces
    StringUtils::Replace(result, "\\\\", "\\");
  }

  return result;
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

bool StringUtils::Contains(std::string_view str,
                           std::string_view keyword,
                           bool isCaseInsensitive /* = true */)
{
  if (isCaseInsensitive)
  {
    auto itStr = std::search(str.begin(), str.end(), keyword.begin(), keyword.end(),
                             [](unsigned char ch1, unsigned char ch2) {
                               return std::toupper(ch1) == std::toupper(ch2);
                             });
    return (itStr != str.end());
  }

  return str.find(keyword) != std::string_view::npos;
}

const std::locale& StringUtils::GetOriginalLocale() noexcept
{
  return g_langInfo.GetOriginalLocale();
}

std::string StringUtils::CreateFromCString(const char* cstr)
{
  return cstr != nullptr ? std::string(cstr) : std::string();
}
