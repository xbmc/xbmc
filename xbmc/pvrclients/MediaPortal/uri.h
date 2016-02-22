#pragma once
/*
 *      Copyright (C) 2005-2011 Team XBMC
 *      http://www.xbmc.org
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <string>

namespace uri
{
  /// Char class.
  enum char_class_e
  {
    CINV = -2, ///< invalid
    CEND = -1, ///< end delimitor
    CVAL = 0,  ///< valid any position
    CVA2 = 1,  ///< valid anywhere but 1st position
  };

  /// Traits used for parsing and encoding components.
  struct traits
  {
    const char* begin_cstring; ///< begin cstring (or 0 if none)
    const char begin_char;     ///< begin char (or 0 if none)
    const char end_char;       ///< end char (or 0 if none)
    char char_class[256];      ///< map of char to class
  };

  /**
   * \brief Encode the URI (sub) component.
   * Note that this should be used on the subcomponents before appending to
   * subdelimiter chars, if any.
   *
   * From the RFC: URI producing applications should percent-encode data octets
   * are specifically allowed by the URI scheme to represent data in that
   * component.  If a reserved character is found in a URI component and
   * no delimiting role is known for that character, then it must be
   * interpreted as representing the data octet corresponding to that
   * character's encoding in US-ASCII.
   * @see http://tools.ietf.org/html/rfc3986
   * @see decode std::string encode(const traits& ts, const std::string& comp);
   */
  std::string encode(const traits& ts, const std::string& comp);
  /**
   * Decode the pct-encoded (hex) sequences, if any, return success.
   * Does not change string on error.
   * @see http://tools.ietf.org/html/rfc3986#section-2.1
   * @see encode
   * \param s A reference to the std::string to decode
   */
  bool decode(std::string& s);

  extern const char ENCODE_BEGIN_CHAR;  ///< encode begin char ('\%')
  extern const traits SCHEME_TRAITS;    ///< scheme traits
  extern const traits AUTHORITY_TRAITS; ///< authority traits
  extern const traits PATH_TRAITS;      ///< path traits
  extern const traits QUERY_TRAITS;     ///< query traits
  extern const traits FRAGMENT_TRAITS;  ///< fragment traits
}
