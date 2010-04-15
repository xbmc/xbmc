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

#include "utils.h"

using namespace std;

namespace uri {
  const char ENCODE_BEGIN_CHAR = '%';
  const traits SCHEME_TRAITS = {
      0, 0, ':',
      {
          CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV, CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV,
          CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV, CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV,
          CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV, CINV,CINV,CINV,CVA2,CINV,CVA2,CVA2,CINV,
          CVA2,CVA2,CVA2,CVA2,CVA2,CVA2,CVA2,CVA2, CVA2,CVA2,CEND,CINV,CINV,CINV,CINV,CINV,
          CINV,CVAL,CVAL,CVAL,CVAL,CVAL,CVAL,CVAL, CVAL,CVAL,CVAL,CVAL,CVAL,CVAL,CVAL,CVAL,
          CVAL,CVAL,CVAL,CVAL,CVAL,CVAL,CVAL,CVAL, CVAL,CVAL,CVAL,CINV,CINV,CINV,CINV,CINV,
          CINV,CVAL,CVAL,CVAL,CVAL,CVAL,CVAL,CVAL, CVAL,CVAL,CVAL,CVAL,CVAL,CVAL,CVAL,CVAL,
          CVAL,CVAL,CVAL,CVAL,CVAL,CVAL,CVAL,CVAL, CVAL,CVAL,CVAL,CINV,CINV,CINV,CINV,CINV, // 127 7F
          CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV, CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV,
          CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV, CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV,
          CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV, CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV,
          CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV, CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV,
          CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV, CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV,
          CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV, CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV,
          CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV, CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV,
          CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV, CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV,
      }
  };
  const traits AUTHORITY_TRAITS = {
      "//", 0, 0,
      {
          CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV, CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV,
          CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV, CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV,
          CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV, CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV,
          CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV, CINV,CINV,CEND,CINV,CINV,CINV,CINV,CINV,
          CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV, CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV,
          CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV, CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV,
          CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV, CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV,
          CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV, CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV, // 127 7F
          CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV, CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV,
          CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV, CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV,
          CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV, CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV,
          CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV, CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV,
          CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV, CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV,
          CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV, CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV,
          CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV, CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV,
          CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV, CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV,
      }
  };
  const traits PATH_TRAITS = {
      0, 0, 0,
      {   // '/' is invalid
          CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV, CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV,
          CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV, CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV,
          CINV,CVAL,CINV,CINV,CVAL,CVAL,CVAL,CVAL, CVAL,CVAL,CVAL,CVAL,CVAL,CVAL,CVAL,CINV,
          CVAL,CVAL,CVAL,CVAL,CVAL,CVAL,CVAL,CVAL, CVAL,CVAL,CVAL,CVAL,CINV,CVAL,CINV,CINV,
          CVAL,CVAL,CVAL,CVAL,CVAL,CVAL,CVAL,CVAL, CVAL,CVAL,CVAL,CVAL,CVAL,CVAL,CVAL,CVAL,
          CVAL,CVAL,CVAL,CVAL,CVAL,CVAL,CVAL,CVAL, CVAL,CVAL,CVAL,CINV,CINV,CINV,CINV,CVAL,
          CINV,CVAL,CVAL,CVAL,CVAL,CVAL,CVAL,CVAL, CVAL,CVAL,CVAL,CVAL,CVAL,CVAL,CVAL,CVAL,
          CVAL,CVAL,CVAL,CVAL,CVAL,CVAL,CVAL,CVAL, CVAL,CVAL,CVAL,CINV,CINV,CINV,CVAL,CINV, // 127 7F
          CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV, CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV,
          CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV, CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV,
          CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV, CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV,
          CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV, CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV,
          CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV, CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV,
          CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV, CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV,
          CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV, CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV,
          CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV, CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV,
      }
  };
  const traits QUERY_TRAITS = {
      0, '?', 0,
      {   // '=' and '&' are invalid
          CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV, CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV,
          CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV, CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV,
          CINV,CVAL,CINV,CINV,CVAL,CVAL,CINV,CVAL, CVAL,CVAL,CVAL,CVAL,CVAL,CVAL,CVAL,CVAL,
          CVAL,CVAL,CVAL,CVAL,CVAL,CVAL,CVAL,CVAL, CVAL,CVAL,CVAL,CVAL,CINV,CINV,CINV,CVAL,
          CVAL,CVAL,CVAL,CVAL,CVAL,CVAL,CVAL,CVAL, CVAL,CVAL,CVAL,CVAL,CVAL,CVAL,CVAL,CVAL,
          CVAL,CVAL,CVAL,CVAL,CVAL,CVAL,CVAL,CVAL, CVAL,CVAL,CVAL,CINV,CINV,CINV,CINV,CVAL,
          CINV,CVAL,CVAL,CVAL,CVAL,CVAL,CVAL,CVAL, CVAL,CVAL,CVAL,CVAL,CVAL,CVAL,CVAL,CVAL,
          CVAL,CVAL,CVAL,CVAL,CVAL,CVAL,CVAL,CVAL, CVAL,CVAL,CVAL,CINV,CINV,CINV,CVAL,CINV, // 127 7F
          CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV, CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV,
          CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV, CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV,
          CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV, CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV,
          CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV, CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV,
          CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV, CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV,
          CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV, CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV,
          CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV, CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV,
          CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV, CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV,
      }
  };
  const traits FRAGMENT_TRAITS = {
      0, '#', 0,
      {
          CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV, CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV,
          CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV, CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV,
          CINV,CVAL,CINV,CINV,CVAL,CVAL,CVAL,CVAL, CVAL,CVAL,CVAL,CVAL,CVAL,CVAL,CVAL,CVAL,
          CVAL,CVAL,CVAL,CVAL,CVAL,CVAL,CVAL,CVAL, CVAL,CVAL,CVAL,CVAL,CINV,CVAL,CINV,CVAL,
          CVAL,CVAL,CVAL,CVAL,CVAL,CVAL,CVAL,CVAL, CVAL,CVAL,CVAL,CVAL,CVAL,CVAL,CVAL,CVAL,
          CVAL,CVAL,CVAL,CVAL,CVAL,CVAL,CVAL,CVAL, CVAL,CVAL,CVAL,CINV,CINV,CINV,CINV,CVAL,
          CINV,CVAL,CVAL,CVAL,CVAL,CVAL,CVAL,CVAL, CVAL,CVAL,CVAL,CVAL,CVAL,CVAL,CVAL,CVAL,
          CVAL,CVAL,CVAL,CVAL,CVAL,CVAL,CVAL,CVAL, CVAL,CVAL,CVAL,CINV,CINV,CINV,CVAL,CINV, // 127 7F
          CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV, CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV,
          CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV, CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV,
          CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV, CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV,
          CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV, CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV,
          CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV, CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV,
          CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV, CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV,
          CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV, CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV,
          CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV, CINV,CINV,CINV,CINV,CINV,CINV,CINV,CINV,
      }
  };

  bool parse_hex(const std::string& s, size_t pos, char& chr) {
    if (s.size() < pos + 2)
        return false;
    unsigned int v;
    unsigned int c = (unsigned int)s[pos];
    if ('0' <= c && c <= '9')
        v = (c - '0') << 4;
    else if ('A' <= c && c <= 'F')
        v = (10 + (c - 'A')) << 4;
    else if ('a' <= c && c <= 'f')
        v = (10 + (c - 'a')) << 4;
    else
        return false;
    c = (unsigned int)s[pos + 1];
    if ('0' <= c && c <= '9')
        v += c - '0';
    else if ('A' <= c && c <= 'F')
        v += 10 + (c - 'A');
    else if ('a' <= c && c <= 'f')
        v += 10 + (c - 'a');
    else
        return false;
    chr = (char)v; // Set output.
    return true;
  }

  void append_hex(char v, std::string& s) {
    unsigned int c = (unsigned char)v & 0xF0;
    c >>= 4;
    s.insert(s.end(), (char)((9 < c) ? (c - 10) + 'A' : c + '0'));
    c = v & 0x0F;
    s.insert(s.end(), (char)((9 < c) ? (c - 10) + 'A' : c + '0'));
  }

  std::string encode(const traits& ts, const std::string& comp) {
      std::string::const_iterator f = comp.begin();
      std::string::const_iterator anchor = f;
      std::string s;
      for (; f != comp.end();) {
          char c = *f;
          if (ts.char_class[(unsigned char)c] < CVAL || c == ENCODE_BEGIN_CHAR) { // Must encode.
              s.append(anchor, f); // Catch up to this char.
              s.append(1, ENCODE_BEGIN_CHAR);
              append_hex(c, s); // Convert.
              anchor = ++f;
          }
          else
              ++f;
      }
      return (anchor == comp.begin()) ? comp : s.append(f, comp.end());
  }

  bool decode(std::string& s) {
      size_t pos = s.find(ENCODE_BEGIN_CHAR);
      if (pos == std::string::npos) // Handle the "99%" case fast.
          return true;
      std::string v;
      for (size_t i = 0;;) {
          if (pos == std::string::npos) {
              v.append(s, i, s.size() - i); // Append up to end.
              break;
          }
          v.append(s, i, pos - i); // Append up to char.
          i = pos + 3; // Skip all 3 chars.
          char c;
          if (!parse_hex(s, pos + 1, c)) // Convert hex.
              return false;
          v.insert(v.end(), c); // Append converted hex.
          pos = s.find(ENCODE_BEGIN_CHAR, i); // Find next
      }
      s = v;
      return true;
  }
}

void Tokenize(const string& str, vector<string>& tokens, const string& delimiters = " ")
{
  // Skip delimiters at beginning.
  //string::size_type lastPos = str.find_first_not_of(delimiters, 0);
  // Don't skip delimiters at beginning.
  string::size_type start_pos = 0;
  // Find first "non-delimiter".
  string::size_type delim_pos = 0;

  while (string::npos != delim_pos)
  {
    delim_pos = str.find_first_of(delimiters, start_pos);
    // Found a token, add it to the vector.
    tokens.push_back(str.substr(start_pos, delim_pos - start_pos));
    start_pos = delim_pos + 1;

    // Find next "non-delimiter"
  }
}

time_t GetUTCdifftime(void)
{
  // Determine time difference between UTC and localtime
  time_t rawtime;
  struct tm* timeinfo;
  time_t local;
  time_t gm;

  time( &rawtime ); // this is already localtime???
  timeinfo = localtime ( &rawtime );
  local = mktime(timeinfo);
  timeinfo = gmtime ( &rawtime );
  gm = mktime(timeinfo);

  return(local - gm);
}
