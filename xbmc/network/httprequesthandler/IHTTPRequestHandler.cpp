/*
 *      Copyright (C) 2011-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "IHTTPRequestHandler.h"

void IHTTPRequestHandler::AddPostField(const std::string &key, const std::string &value)
{
  if (key.empty())
    return;

  std::map<std::string, std::string>::iterator field = m_postFields.find(key);
  if (field == m_postFields.end())
    m_postFields[key] = value;
  else
    m_postFields[key].append(value);
}

#if (MHD_VERSION >= 0x00040001)
bool IHTTPRequestHandler::AddPostData(const char *data, size_t size)
#else
bool IHTTPRequestHandler::AddPostData(const char *data, unsigned int size)
#endif
{
  if (size > 0)
    return appendPostData(data, size);
  
  return true;
}