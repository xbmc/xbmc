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

#if (defined HAVE_CONFIG_H) && (!defined WIN32)
  #include "config.h"
#endif

#ifdef HAVE_LIBARCHIVE

#include "FileArchive.h"
#include "URL.h"
#include "utils/URIUtils.h"

#include <deque>
#include <sys/stat.h>

using namespace XFILE;
using namespace std;

CFileArchive::CFileArchive()
{
  m_bCached = false;
}

CFileArchive::~CFileArchive()
{
  Close();
}

bool CFileArchive::Open(const CURL&url)
{
  CStdString strOpts = url.GetOptions();
  CURL url2(url);
  url2.SetOptions("");
  CStdString strPath = url2.Get();
  if (!g_archiveManager.GetArchiveEntry(strPath,m_archiveItem))
    return false;

  CStdString cache_path;
  m_archiveItem.get_cache_path(cache_path);

  if (cache_path.length() <= 0)
  {
    CStdString file;
    m_archiveItem.get_file(file);
    if (!g_archiveManager.CacheArchivedPath(url2.GetHostName(),m_archiveItem))
      return false;
    m_archiveItem.get_cache_path(cache_path);
  }

  return mFile.Open(cache_path);
}

int64_t CFileArchive::GetLength()
{
  return m_archiveItem.get_file_stat()->st_size;
}

int64_t CFileArchive::GetPosition()
{
  return mFile.GetPosition();
}

int64_t CFileArchive::Seek(int64_t iFilePosition, int iWhence)
{
  return mFile.Seek(iFilePosition,iWhence);
}

bool CFileArchive::Exists(const CURL& url)
{
  CArchiveEntry item;
  if (g_archiveManager.GetArchiveEntry(url.Get(),item))
    return true;
  return false;
}

int CFileArchive::Stat(struct __stat64 *buffer)
{
  /* libarchive doesn't support stat64 yet */
  const struct stat *s = m_archiveItem.get_file_stat();
  if (!s)
    return -1;
  buffer->st_dev = s->st_dev;
  buffer->st_ino = s->st_ino;
  buffer->st_mode = s->st_mode;
  buffer->st_nlink = s->st_nlink;
  buffer->st_uid = s->st_uid;
  buffer->st_gid = s->st_gid;
  buffer->st_rdev = s->st_rdev;
  buffer->st_size = s->st_size;
  buffer->st_atime = s->st_atime;
  buffer->st_mtime = s->st_mtime;
  buffer->st_ctime = s->st_ctime;
  buffer->st_blksize = s->st_blksize;
  buffer->st_blocks = s->st_blocks;
  return 0;
}

int CFileArchive::Stat(const CURL& url, struct __stat64* buffer)
{
  if (!g_archiveManager.GetArchiveEntry(url.Get(),m_archiveItem))
    return -1;
  return Stat(buffer);
}

unsigned int CFileArchive::Read(void* lpBuf, int64_t uiBufSize)
{
  return mFile.Read(lpBuf,uiBufSize);
}

void CFileArchive::Close()
{
  mFile.Close();
}

#endif // HAVE_LIBARCHIVE
