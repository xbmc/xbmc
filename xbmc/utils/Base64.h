#pragma once
/*
 *      Copyright (C) 2011 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include <string>

class Base64
{
public:
  static void Encode(const char* input, unsigned int length, std::string &output);
  static std::string Encode(const char* input, unsigned int length);
  static void Encode(const std::string &input, std::string &output);
  static std::string Encode(const std::string &input);
  static void Decode(const char* input, unsigned int length, std::string &output);
  static std::string Decode(const char* input, unsigned int length);
  static void Decode(const std::string &input, std::string &output);
  static std::string Decode(const std::string &input);

private:
  static const std::string m_characters;
};
