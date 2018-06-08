/*
 *      Copyright (C) 2013 Team XBMC
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "Utf8Utils.h"


CUtf8Utils::utf8CheckResult CUtf8Utils::checkStrForUtf8(const std::string& str)
{
  const char* const strC = str.c_str();
  const size_t len = str.length();
  size_t pos = 0;
  bool isPlainAscii = true;

  while (pos < len)
  {
    const size_t chrLen = SizeOfUtf8Char(strC + pos);
    if (chrLen == 0)
      return hiAscii; // non valid UTF-8 sequence
    else if (chrLen > 1)
      isPlainAscii = false;

    pos += chrLen;
  }

  if (isPlainAscii)
    return plainAscii; // only single-byte characters (valid for US-ASCII and for UTF-8)

  return utf8string;   // valid UTF-8 with at least one valid UTF-8 multi-byte sequence
}



size_t CUtf8Utils::FindValidUtf8Char(const std::string& str, const size_t startPos /*= 0*/)
{
  const char* strC = str.c_str();
  const size_t len = str.length();

  size_t pos = startPos;
  while (pos < len)
  {
    if (SizeOfUtf8Char(strC + pos))
      return pos;

    pos++;
  }

  return std::string::npos;
}

size_t CUtf8Utils::RFindValidUtf8Char(const std::string& str, const size_t startPos)
{
  const size_t len = str.length();
  if (!len)
    return std::string::npos;

  const char* strC = str.c_str();
  size_t pos = (startPos >= len) ? len - 1 : startPos;
  while (pos < len)  // pos is unsigned, after zero pos becomes large then len
  {
    if (SizeOfUtf8Char(strC + pos))
      return pos;

    pos--;
  }

  return std::string::npos;
}

inline size_t CUtf8Utils::SizeOfUtf8Char(const std::string& str, const size_t charStart /*= 0*/)
{
  if (charStart >= str.length())
    return std::string::npos;

  return SizeOfUtf8Char(str.c_str() + charStart);
}

// must be used only internally in class!
// str must be null-terminated
inline size_t CUtf8Utils::SizeOfUtf8Char(const char* const str)
{
  if (!str)
    return 0;

  const unsigned char* const strU = (const unsigned char*)str;
  const unsigned char chr = strU[0];

  /* this is an implementation of http://www.unicode.org/versions/Unicode6.2.0/ch03.pdf#G27506 */

  /* U+0000 - U+007F in UTF-8 */
  if (chr <= 0x7F)
    return 1;

  /* U+0080 - U+07FF in UTF-8 */                    /* binary representation and range */
  if (chr >= 0xC2 && chr <= 0xDF                    /* C2=1100 0010 - DF=1101 1111 */
      // as str is null terminated,
      && ((strU[1] & 0xC0) == 0x80))  /* C0=1100 0000, 80=1000 0000 - BF=1011 1111 */
    return 2;  // valid UTF-8 2 bytes sequence

  /* U+0800 - U+0FFF in UTF-8 */
  if (chr == 0xE0                                   /* E0=1110 0000 */
      && (strU[1] & 0xE0) == 0xA0     /* E0=1110 0000, A0=1010 0000 - BF=1011 1111 */
      && (strU[2] & 0xC0) == 0x80)    /* C0=1100 0000, 80=1000 0000 - BF=1011 1111 */
    return 3; // valid UTF-8 3 bytes sequence

  /* U+1000 - U+CFFF in UTF-8 */
  /* skip U+D000 - U+DFFF (handled later) */
  /* U+E000 - U+FFFF in UTF-8 */
  if (((chr >= 0xE1 && chr <= 0xEC)                 /* E1=1110 0001 - EC=1110 1100 */
        || chr == 0xEE || chr == 0xEF)              /* EE=1110 1110 - EF=1110 1111 */
        && (strU[1] & 0xC0) == 0x80   /* C0=1100 0000, 80=1000 0000 - BF=1011 1111 */
        && (strU[2] & 0xC0) == 0x80)  /* C0=1100 0000, 80=1000 0000 - BF=1011 1111 */
    return 3; // valid UTF-8 3 bytes sequence

  /* U+D000 - U+D7FF in UTF-8 */
  /* note: range U+D800 - U+DFFF is reserved and invalid */
  if (chr == 0xED                                   /* ED=1110 1101 */
      && (strU[1] & 0xE0) == 0x80     /* E0=1110 0000, 80=1000 0000 - 9F=1001 1111 */
      && (strU[2] & 0xC0) == 0x80)    /* C0=1100 0000, 80=1000 0000 - BF=1011 1111 */
    return 3; // valid UTF-8 3 bytes sequence

  /* U+10000 - U+3FFFF in UTF-8 */
  if (chr == 0xF0                                   /* F0=1111 0000 */
      && (strU[1] & 0xE0) == 0x80     /* E0=1110 0000, 80=1000 0000 - 9F=1001 1111 */
      && strU[2] >= 0x90 && strU[2] <= 0xBF         /* 90=1001 0000 - BF=1011 1111 */
      && (strU[3] & 0xC0) == 0x80)    /* C0=1100 0000, 80=1000 0000 - BF=1011 1111 */
    return 4; // valid UTF-8 4 bytes sequence

  /* U+40000 - U+FFFFF in UTF-8 */
  if (chr >= 0xF1 && chr <= 0xF3                    /* F1=1111 0001 - F3=1111 0011 */
      && (strU[1] & 0xC0) == 0x80     /* C0=1100 0000, 80=1000 0000 - BF=1011 1111 */
      && (strU[2] & 0xC0) == 0x80     /* C0=1100 0000, 80=1000 0000 - BF=1011 1111 */
      && (strU[3] & 0xC0) == 0x80)    /* C0=1100 0000, 80=1000 0000 - BF=1011 1111 */
    return 4; // valid UTF-8 4 bytes sequence

  /* U+100000 - U+10FFFF in UTF-8 */
  if (chr == 0xF4                                   /* F4=1111 0100 */
      && (strU[1] & 0xF0) == 0x80     /* F0=1111 0000, 80=1000 0000 - 8F=1000 1111 */
      && (strU[2] & 0xC0) == 0x80     /* C0=1100 0000, 80=1000 0000 - BF=1011 1111 */
      && (strU[3] & 0xC0) == 0x80)    /* C0=1100 0000, 80=1000 0000 - BF=1011 1111 */
    return 4; // valid UTF-8 4 bytes sequence

  return 0; // invalid UTF-8 char sequence
}
