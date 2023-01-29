/*
 *  Copyright (C) 2005-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "CSSUtils.h"

#include <cstdint>
#include <string>

namespace
{
// https://www.w3.org/TR/css-syntax-3/#hex-digit
bool isHexDigit(char c)
{
  return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
}

// https://www.w3.org/TR/css-syntax-3/#hex-digit
uint32_t convertHexDigit(char c)
{
  if (c >= '0' && c <= '9')
  {
    return c - '0';
  }
  else if (c >= 'A' && c <= 'F')
  {
    return 10 + c - 'A';
  }
  else
  {
    return 10 + c - 'a';
  }
}

// https://infra.spec.whatwg.org/#surrogate
bool isSurrogateCodePoint(uint32_t c)
{
  return c >= 0xD800 && c <= 0xDFFF;
}

// https://www.w3.org/TR/css-syntax-3/#maximum-allowed-code-point
bool isGreaterThanMaximumAllowedCodePoint(uint32_t c)
{
  return c > 0x10FFFF;
}

// https://www.w3.org/TR/css-syntax-3/#consume-escaped-code-point
std::string escapeStringChunk(std::string& str, size_t& pos)
{
  if (str.size() < pos + 1)
    return "";

  uint32_t codePoint = convertHexDigit(str[pos + 1]);

  if (str.size() >= pos + 2)
    pos += 2;
  else
    return "";

  int numDigits = 1;
  while (numDigits < 6 && isHexDigit(str[pos]))
  {
    codePoint = 16 * codePoint + convertHexDigit(str[pos]);
    if (str.size() >= pos + 1)
    {
      pos += 1;
      numDigits += 1;
    }
    else
      break;
  }

  std::string result;

  // Convert code point to UTF-8 bytes
  if (codePoint == 0 || isSurrogateCodePoint(codePoint) ||
      isGreaterThanMaximumAllowedCodePoint(codePoint))
  {
    result += u8"\uFFFD";
  }
  else if (codePoint < 0x80)
  {
    // 1-byte UTF-8: 0xxxxxxx
    result += static_cast<char>(codePoint);
  }
  else if (codePoint < 0x800)
  {
    // 2-byte UTF-8: 110xxxxx 10xxxxxx
    uint32_t x1 = codePoint >> 6; // 6 = num of x's in 2nd byte
    uint32_t x2 = codePoint - (x1 << 6); // 6 = num of x's in 2nd byte
    uint32_t b1 = (6 << 5) + x1; // 6 = 0b110 ; 5 = num of x's in 1st byte
    uint32_t b2 = (2 << 6) + x2; // 2 = 0b10  ; 6 = num of x's in 2nd byte
    result += static_cast<char>(b1);
    result += static_cast<char>(b2);
  }
  else if (codePoint < 0x10000)
  {
    // 3-byte UTF-8: 1110xxxx 10xxxxxx 10xxxxxx
    uint32_t y1 = codePoint >> 6;
    uint32_t x3 = codePoint - (y1 << 6);
    uint32_t x1 = y1 >> 6;
    uint32_t x2 = y1 - (x1 << 6);
    uint32_t b1 = (14 << 4) + x1;
    uint32_t b2 = (2 << 6) + x2;
    uint32_t b3 = (2 << 6) + x3;
    result += static_cast<char>(b1);
    result += static_cast<char>(b2);
    result += static_cast<char>(b3);
  }
  else
  {
    // 4-byte UTF-8: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
    uint32_t y2 = codePoint >> 6;
    uint32_t x4 = codePoint - (y2 << 6);
    uint32_t y1 = y2 >> 6;
    uint32_t x3 = y2 - (y1 << 6);
    uint32_t x1 = y1 >> 6;
    uint32_t x2 = y1 - (x1 << 6);
    uint32_t b1 = (30 << 3) + x1;
    uint32_t b2 = (2 << 6) + x2;
    uint32_t b3 = (2 << 6) + x3;
    uint32_t b4 = (2 << 6) + x4;
    result += static_cast<char>(b1);
    result += static_cast<char>(b2);
    result += static_cast<char>(b3);
    result += static_cast<char>(b4);
  }

  return result;
}

} // unnamed namespace

void UTILS::CSS::Escape(std::string& str)
{
  std::string result;

  for (size_t pos = 0; pos < str.size(); pos++)
  {
    if (str[pos] == '\\')
      result += escapeStringChunk(str, pos);
    else
      result += str[pos];
  }

  str = result;
}
