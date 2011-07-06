/*
 *      Copyright (C) 2005-2010 Team XBMC
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

#include "StreamUtils.h"
#include "LangInfo.h"

//#include <algorithm>

using namespace std;

int StreamUtils::GetCodecPriority(const CStdString &codec)
{
  if (codec == "flac") // Lossless FLAC
    return 6;
  if (codec == "dtsma") // DTS-HD Master Audio (aka DTS++)
    return 5;
  if (codec == "truehd") // Dolby TrueHD
    return 4;
  if (codec == "eac3") // Dolby Digital Plus
    return 3;
  if (codec == "dca") // DTS
    return 2;
  if (codec == "ac3") // Dolby Digital
    return 1;
  return 0;
}

bool StreamUtils::IsSameLanguage(const CStdString &left, const CStdString &right)
{
  /*
   * If either language code is empty then assume that they are the same. No sensible comparison can
   * take place.
   */
  if(left.IsEmpty() || right.IsEmpty())
    return true;

  /*
   * If the language codes are the same length then do a direct comparison.
   */
  if (left.length() == right.length())
    return left == right;

  /*
   * The ffmpeg stream language is supposed to be an ISO 639-2 code (3 characters long). The language
   * being compared with may only be a 2 character ISO 639-1 code. Convert both to the 2 character
   * language code and go from there. If one of the codes is invalid, then an empty language code
   * will be returned and will ultimately be different to a valid code.
   *
   * http://en.wikipedia.org/wiki/List_of_ISO_639-1_codes
   * http://en.wikipedia.org/wiki/List_of_ISO_639-2_codes
   */
  CStdString convertedLeft(left);
  CStdString convertedRight(right);

  if(left.length() == 3) // IS639-2 code
    convertedLeft = g_langInfo.ConvertIso6392ToIso6391(left); // IS639-1 code

  if(right.length() == 3) // IS639-2 code
    convertedRight = g_langInfo.ConvertIso6392ToIso6391(right); // IS639-1 code

  return convertedLeft == convertedRight;
}
