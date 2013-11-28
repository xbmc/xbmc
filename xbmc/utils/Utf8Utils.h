#pragma once

/*
 *      Copyright (C) 2013 Team XBMC
 *      http://xbmc.org
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


#include <string>

class CUtf8Utils
{
public:
  static size_t FindValidUtf8Char(const std::string& str, const size_t startPos = 0);
  static size_t RFindValidUtf8Char(const std::string& str, const size_t startPos);
  
  static size_t SizeOfUtf8Char(const std::string& str, const size_t charStart = 0);
private:
  static size_t SizeOfUtf8Char(const char* const str);
};
