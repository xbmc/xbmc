/*
 *      Copyright (C) 2005-2014 Team XBMC
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

#include "system.h"
#ifdef HAVE_LIBBLURAY
#include "BlurayFile.h"
#include "URL.h"
#include "utils/StringUtils.h"

namespace XFILE
{

  CBlurayFile::CBlurayFile(void)
  {
  }

  CBlurayFile::~CBlurayFile(void)
  {
    Close();
  }

  CURL CBlurayFile::RemoveProtocol(const CURL& url)
  {
    assert(url.IsProtocol("bluray"));

    std::string host = url.GetHostName();
    std::string filename = url.GetFileName();
    if (host.empty() || filename.empty())
      return CURL();
    return CURL(host.append(filename));
  }

  bool CBlurayFile::Open(const CURL& url)
  {
    return m_file.Open(RemoveProtocol(url));
  }

  bool CBlurayFile::Exists(const CURL& url)
  {
    return m_file.Exists(RemoveProtocol(url));
  }

  int CBlurayFile::Stat(const CURL& url, struct __stat64* buffer)
  {
    return m_file.Stat(RemoveProtocol(url), buffer);
  }

  ssize_t CBlurayFile::Read(void* lpBuf, size_t uiBufSize)
  {
    return m_file.Read(lpBuf, uiBufSize);
  }

  int64_t CBlurayFile::Seek(int64_t iFilePosition, int iWhence /*=SEEK_SET*/)
  {
    return m_file.Seek(iFilePosition, iWhence);
  }

  void CBlurayFile::Close()
  {
    m_file.Close();
  }

  int64_t CBlurayFile::GetPosition()
  {
    return m_file.GetPosition();
  }

  int64_t CBlurayFile::GetLength()
  {
    return m_file.GetLength();
  }
} /* namespace XFILE */
#endif
