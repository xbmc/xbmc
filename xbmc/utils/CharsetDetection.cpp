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

#include "CharsetDetection.h"


std::string CCharsetDetection::GetBomEncoding(const char* const content, const size_t contentLength)
{
  if (contentLength < 2)
    return "";
  if (content[0] == (char)0xFE && content[1] == (char)0xFF)
    return "UTF-16BE";
  if (contentLength >= 4 && content[0] == (char)0xFF && content[1] == (char)0xFE && content[2] == (char)0x00 && content[3] == (char)0x00)
    return "UTF-32LE";  /* first two bytes are same for UTF-16LE and UTF-32LE, so first check for full UTF-32LE mark */
  if (content[0] == (char)0xFF && content[1] == (char)0xFE)
   return "UTF-16LE";
  if (contentLength < 3)
    return "";
  if (content[0] == (char)0xEF && content[1] == (char)0xBB && content[2] == (char)0xBF)
    return "UTF-8";
  if (contentLength < 4)
    return "";
  if (content[0] == (char)0x00 && content[1] == (char)0x00 && content[2] == (char)0xFE && content[3] == (char)0xFF)
    return "UTF-32BE";
  if (contentLength >= 5 && content[0] == (char)0x2B && content[1] == (char)0x2F && content[2] == (char)0x76 &&
            (content[4] == (char)0x32 || content[4] == (char)0x39 || content[4] == (char)0x2B || content[4] == (char)0x2F))
    return "UTF-7";
  if (content[0] == (char)0x84 && content[1] == (char)0x31 && content[2] == (char)0x95 && content[3] == (char)0x33)
    return "GB18030";

  return "";
}
