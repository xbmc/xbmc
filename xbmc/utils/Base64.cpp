/*
 *  Copyright (C) 2011-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Base64.h"

#define PADDING '='

const std::string Base64::m_characters = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                         "abcdefghijklmnopqrstuvwxyz"
                                         "0123456789+/";

void Base64::Encode(const char* input, unsigned int length, std::string &output)
{
  if (input == NULL || length == 0)
    return;

  long l;
  output.clear();
  output.reserve(((length + 2) / 3) * 4);

  for (unsigned int i = 0; i < length; i += 3)
  {
    l  = ((((unsigned long) input[i]) << 16) & 0xFFFFFF) |
         ((((i + 1) < length) ? (((unsigned long) input[i + 1]) << 8) : 0) & 0xFFFF) |
         ((((i + 2) < length) ? (((unsigned long) input[i + 2]) << 0) : 0) & 0x00FF);

    output.push_back(m_characters[(l >> 18) & 0x3F]);
    output.push_back(m_characters[(l >> 12) & 0x3F]);

    if (i + 1 < length)
      output.push_back(m_characters[(l >> 6) & 0x3F]);
    if (i + 2 < length)
      output.push_back(m_characters[(l >> 0) & 0x3F]);
  }

  int left = 3 - (length % 3);

  if (length % 3)
  {
    for (int i = 0; i < left; i++)
      output.push_back(PADDING);
  }
}

std::string Base64::Encode(const char* input, unsigned int length)
{
  std::string output;
  Encode(input, length, output);

  return output;
}

void Base64::Encode(const std::string &input, std::string &output)
{
  Encode(input.c_str(), input.size(), output);
}

std::string Base64::Encode(const std::string &input)
{
  std::string output;
  Encode(input, output);

  return output;
}

void Base64::Decode(const char* input, unsigned int length, std::string &output)
{
  if (input == NULL || length == 0)
    return;

  long l;
  output.clear();

  for (unsigned int index = 0; index < length; index++)
  {
    if (input[index] == '=')
    {
      length = index;
      break;
    }
  }

  output.reserve(length - ((length + 2) / 4));

  for (unsigned int i = 0; i < length; i += 4)
  {
    l = ((((unsigned long) m_characters.find(input[i])) & 0x3F) << 18);
    l |= (((i + 1) < length) ? ((((unsigned long) m_characters.find(input[i + 1])) & 0x3F) << 12) : 0);
    l |= (((i + 2) < length) ? ((((unsigned long) m_characters.find(input[i + 2])) & 0x3F) <<  6) : 0);
    l |= (((i + 3) < length) ? ((((unsigned long) m_characters.find(input[i + 3])) & 0x3F) <<  0) : 0);

    output.push_back((char)((l >> 16) & 0xFF));
    if (i + 2 < length)
      output.push_back((char)((l >> 8) & 0xFF));
    if (i + 3 < length)
      output.push_back((char)((l >> 0) & 0xFF));
  }
}

std::string Base64::Decode(const char* input, unsigned int length)
{
  std::string output;
  Decode(input, length, output);

  return output;
}

void Base64::Decode(const std::string &input, std::string &output)
{
  size_t length = input.find_first_of(PADDING);
  if (length == std::string::npos)
    length = input.size();

  Decode(input.c_str(), length, output);
}

std::string Base64::Decode(const std::string &input)
{
  std::string output;
  Decode(input, output);

  return output;
}
