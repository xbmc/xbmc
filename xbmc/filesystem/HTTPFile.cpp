/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "HTTPFile.h"

using namespace XFILE;

CHTTPFile::CHTTPFile(void)
{
  m_openedforwrite = false;
}


CHTTPFile::~CHTTPFile(void)
{
}

bool CHTTPFile::OpenForWrite(const CURL& url, bool bOverWrite)
{
  // real Open is delayed until we receive the POST data
  m_urlforwrite = url;
  m_openedforwrite = true;
  return true;
}

ssize_t CHTTPFile::Write(const void* lpBuf, size_t uiBufSize)
{
  // Although we can not verify much, try to catch errors where we can
  if (!m_openedforwrite)
    return -1;

  m_postdata = std::string(static_cast<const char*>(lpBuf), uiBufSize);
  m_postdataset = true;
  m_openedforwrite = false;
  if (!Open(m_urlforwrite))
    return -1;

  // Finally (and this is a clumsy hack) return the http response code
  return m_httpresponse;
}

