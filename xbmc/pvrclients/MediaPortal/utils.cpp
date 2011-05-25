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
 *
 */
#if defined(_WIN32) || defined(_WIN64)
#pragma warning(disable:4244) //wchar to char = loss of data
#endif

#include "utils.h"
#include <string>

using namespace std;

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

std::string WStringToString(const std::wstring& s)
{
  std::string temp(s.length(), ' ');
  std::copy(s.begin(), s.end(), temp.begin());
  return temp;
}

std::wstring StringToWString(const std::string& s)
{
  std::wstring temp(s.length(),L' ');
  std::copy(s.begin(), s.end(), temp.begin());
  return temp;
}

std::string lowercase(const std::string& s)
{
  std::string t;
  for (std::string::const_iterator i = s.begin(); i != s.end(); ++i)
  {
    t += tolower(*i);
  }
  return t;
}

bool stringtobool(const std::string& s)
{
  std::string temp = lowercase(s);

  if(temp.compare("false") == 0)
    return false;
  else if(temp.compare("0") == 0)
    return false;
  else
    return true;
}
