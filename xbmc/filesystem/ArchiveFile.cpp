/*
 *      Copyright (C) 2012 Team XBMC
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

#include "ArchiveFile.h"
#include "ArchiveManager.h"
#include "utils/RegExp.h"
#include "utils/URIUtils.h"
#include "Directory.h"
#include "URL.h"
#include "FileItem.h"
#include "File.h"

using namespace XFILE;
using namespace std;

CArchiveFile::CArchiveFile()
{
  m_format = -1;
  m_filter = -1;
  m_archive = NULL;
  m_archive_entry = NULL;
  m_position = -1;
  m_fakeDirStatBuffer = NULL;
}

CArchiveFile::CArchiveFile(int format, int filter)
{
  m_format = format;
  m_filter = filter;
  m_archive = NULL;
  m_archive_entry = NULL;
  m_position = -1;
  m_fakeDirStatBuffer = NULL;
}

CArchiveFile::~CArchiveFile()
{
  Close();
}

bool CArchiveFile::Open(const CURL& url)
{
  int r;
  CStdString pattern, archivePath, archivePathDecoded, pathInArchive;
  CStdString pathNoSlash;
  CRegExp regex(true), regex2(true);
  bool isDirectory = false;

  m_strUrl = url.Get();

  if (!Load(__FUNCTION__))
    return false;

  /*
   * Regex matching standard RAR protocol URL
   * Group 1: RAR protocol URL up to last '.rar/' or '.\d\d\d/'
   * Group 2: the encoded path for the RAR file
   * Group 3: contents in RAR
   */
  pattern = "^(rar://([^/]*?)/)(.*)$";
  if (!regex.RegComp(pattern) || regex.RegFind(m_strUrl) < 0)
  {
    CLog::Log(LOGDEBUG, "%s: Couldn't get regex match for '%s'",
              __FUNCTION__, m_strUrl.c_str());
    return false;
  }

  archivePath = regex.GetMatch(1);
  archivePathDecoded = CURL::Decode(regex.GetMatch(2));
  pathInArchive = regex.GetMatch(3);
  pathNoSlash = pathInArchive;

  /*
   * Regex matching a directory path
   * Group 1: contents in RAR without slash
   */
  pattern = "^(.*)/$";
  if (!regex.RegComp(pattern))
  {
    CLog::Log(LOGDEBUG, "%s: Couldn't compile regex", __FUNCTION__);
    return false;
  }
  if (regex.RegFind(pathInArchive) >= 0)
  {
    /* Strip the trailing slash */
    pathNoSlash = regex.GetMatch(1);

    /* Regex matching dir with or without content */
    pattern = "^\\Q" + pathInArchive + "\\E.*$";
    if (!regex.RegComp(pattern))
    {
      CLog::Log(LOGDEBUG, "%s: Couldn't compile regex", __FUNCTION__);
      return false;
    }
    isDirectory = true;
  }

  if (!CreateArchive())
  {
    CLog::Log(LOGERROR, "%s: Error creating archive", __FUNCTION__);
    return false;
  }

  r = m_dll.archive_read_open1(m_archive);
  if (r != ARCHIVE_OK)
  {
    CLog::Log(LOGERROR,"%s: %s", __FUNCTION__,
              m_dll.archive_error_string(m_archive));
    Close();
    return false;
  }

  /* Now find the file the specified file in the archive */
  while (m_dll.archive_read_next_header(m_archive, &m_archive_entry)
    == (ARCHIVE_OK))
  {
    CStdString filepath(m_dll.archive_entry_pathname(m_archive_entry));
    if (pathNoSlash.Equals(filepath))
    {
      m_position = 0;

      /* Free the fake dir stat buffer if one was created */
      free(m_fakeDirStatBuffer);
      m_fakeDirStatBuffer = NULL;
      return true;
    }
    else if (isDirectory && regex.RegFind(filepath) >= 0)
    {
      /*
       * Archive optimization where non-empty directories will sometimes
       * not contain its own header block. Create a fake stat buffer
       * for these directories, but continue to check all archive headers.
       */
      if (m_fakeDirStatBuffer)
        continue;
      m_fakeDirStatBuffer =
        (struct __stat64*)calloc(1, sizeof(struct __stat64));
      if (!m_fakeDirStatBuffer)
      {
        CLog::Log(LOGERROR, "%s: Out of memory", __FUNCTION__);
        Close();
        return false;
      }

      /* Copy the statistics from the archive */
      if (CFile::Stat(archivePathDecoded, m_fakeDirStatBuffer) < 0)
      {
        CLog::Log(LOGERROR, "%s: Could not stat %s", __FUNCTION__,
          archivePathDecoded.c_str());
        Close();
        return false;
      }

      /* Fixup some statistics for the directory */
      m_fakeDirStatBuffer->st_mode =
        (
          S_IFDIR |
          S_IRUSR |
          S_IWUSR |
          S_IXUSR |
          S_IRGRP |
          S_IXGRP |
          S_IROTH |
          S_IXOTH
        );
      m_fakeDirStatBuffer->st_size = 4;
      m_fakeDirStatBuffer->st_blksize = 4;
      m_fakeDirStatBuffer->st_blocks = 8;
      m_position = 0;
    }
  }

  /*
   * If a fake directory stat struct exists, the directory exists in the
   * archive.
   */
  if (m_fakeDirStatBuffer)
    return true;

  /* Specified path doesn't exist in archive */
  Close();
  return false;
}

bool CArchiveFile::Exists(const CURL& url)
{
  /* File exists if it can be opened */
  if (Open(url))
  {
    Close();
    return true;
  }
  return false;
}

int CArchiveFile::Stat(const CURL& url, struct __stat64* buffer)
{
  /* Can call stat if file can be opened */
  int retval = -1;
  if (Open(url))
  {
    retval = Stat(buffer);
    Close();
  }
  return retval;
}

int CArchiveFile::Stat(struct __stat64* buffer)
{
  if (m_fakeDirStatBuffer)
  {
    /* This was a directory in the archive without a header */
    memcpy(buffer, m_fakeDirStatBuffer, sizeof(struct __stat64));
    return 0;
  }

  if (!m_archive_entry)
    return -1;

  const struct stat *s = m_dll.archive_entry_stat(m_archive_entry);
  if (!s)
    return -1;

  /* Safely copy entries from stat to stat64 */
  buffer->st_dev = s->st_dev;
  buffer->st_ino = s->st_ino;
  buffer->st_mode = s->st_mode;
  buffer->st_nlink = s->st_nlink;
  buffer->st_uid = s->st_uid;
  buffer->st_gid = s->st_gid;
  buffer->st_rdev = s->st_rdev;
  buffer->st_size = s->st_size;
  buffer->st_blksize = s->st_blksize;
  buffer->st_blocks = s->st_blocks;
  buffer->st_atime = s->st_atime;
  buffer->st_mtime = s->st_mtime;
  buffer->st_ctime = s->st_ctime;

  return 0;
}

unsigned int CArchiveFile::Read(void* lpBuf, int64_t uiBufSize)
{
  if (!m_archive)
    return 0;

  ssize_t retval = m_dll.archive_read_data(m_archive, lpBuf, uiBufSize);

  if (retval > 0)
  {
    m_position += retval;
    return retval;
  }
  return 0;
}

bool CArchiveFile::ReadString(char *szLine, int iLineLength)
{
  CRegExp regex;
  unsigned int bytesRead;
  CStdString newStr;
  if ((bytesRead = Read(szLine, iLineLength - 1)) == 0)
    return false;
  szLine[bytesRead] = '\0';

  if (!regex.RegComp("(.*?(\n|\r)).*"))
    return false;

  if (regex.RegFind(szLine) >= 0)
  {
    newStr = regex.GetMatch(1);
    newStr += regex.GetMatch(2);
    strcpy(szLine, newStr.c_str());
    Seek(m_position - (bytesRead - newStr.length()), SEEK_SET);
  }
  return true;
}

int64_t CArchiveFile::Seek(int64_t iFilePosition, int iWhence)
{
  int64_t r, new_position;
  uint8_t buf[128];
  unsigned int buf_read;
  if (!m_archive)
    return -1;

  /* libarchive seek support for RAR archives is currently broken */
  r = m_dll.archive_seek_data(m_archive, iFilePosition, iWhence);
  if (r == (ARCHIVE_FAILED))
  {
    /* Likely a compressed archive which can't be seeked. Do read + discard */
    switch (iWhence)
    {
      case SEEK_CUR:
        new_position = GetPosition() + iFilePosition;
        break;
      case SEEK_END:
        new_position = GetLength() + iFilePosition;
        break;
      case SEEK_SET:
      default:
        new_position = iFilePosition;
    }
    Close();
    CURL url(m_strUrl);
    if (!Open(url))
      return -1;
    r = new_position;

    if (r < 0)
      return -1; /* Can't seek past beginning of file */
    else if (r > GetLength())
    {
      /* Seek to end of file but allow return value past end of file */
      new_position = GetLength();
    }

    while (new_position > 0)
    {
      if (new_position > (int64_t)sizeof(buf))
        buf_read = Read(buf, sizeof(buf));
      else
        buf_read = Read(buf, new_position);
      if (!buf_read)
        return -1;
      new_position -= buf_read;
    }
  }
  m_position = r;
  return r;
}

void CArchiveFile::Close()
{
  if (!Load(__FUNCTION__))
    return;

  /* NOTE: Freeing an archive or archive entry frees both structs */
  if (m_archive)
  {
    m_dll.archive_read_free(m_archive);
    m_archive = NULL;
    m_archive_entry = NULL;
  }
  else if (m_archive_entry)
  {
    m_dll.archive_entry_free(m_archive_entry);
    m_archive = NULL;
    m_archive_entry = NULL;
  }
  m_position = -1;
  free(m_fakeDirStatBuffer);
  m_fakeDirStatBuffer = NULL;
}

int64_t CArchiveFile::GetPosition()
{
  return m_position;
}

int64_t CArchiveFile::GetLength()
{
  if (!m_archive_entry)
    return -1;
  return m_dll.archive_entry_size(m_archive_entry);
}

bool CArchiveFile::CreateArchive()
{
  CStdString pattern, archivePathDecoded, strPathNoExt;
  CRegExp regex(true);
  std::vector<CStdString> filePaths;
  CFileItemList itemlist;

  pattern = "^(rar://([^/]*?)/)(.*)$";
  if (!regex.RegComp(pattern) || regex.RegFind(m_strUrl) < 0)
    return false;

  archivePathDecoded = CURL::Decode(regex.GetMatch(2));

  pattern = "^(.*)(\\.part\\d+\\.rar)$";
  if (!regex.RegComp(pattern))
    return false;

  if (regex.RegFind(archivePathDecoded) >= 0)
  {
    /* This may be a multivolume archive */
    strPathNoExt = regex.GetMatch(1);

    if (!CDirectory::GetDirectory(URIUtils::GetDirectory(archivePathDecoded),
      itemlist, ".rar", DIR_FLAG_NO_FILE_DIRS))
      return NULL;

    /* Remove files that may not be part of the multivolume RAR */
    pattern = "^\\Q" + strPathNoExt + "\\E\\.part\\d+\\.rar$";
    if (!regex.RegComp(pattern))
      return NULL;
    for (int i = 0; i < itemlist.Size(); i++)
    {
      if (regex.RegFind(itemlist[i]->GetPath()) < 0)
      {
        itemlist.Remove(i);
      }
    }
  }

  if (itemlist.IsEmpty())
  {
    /* Not a multivolume archive */
    filePaths.push_back(archivePathDecoded);
  }
  else
  {
    /*
    * Sort the list, add the entries to a vector, and return
    * a CArchiveDirectory object with this vector.
    */
    itemlist.Sort(SORT_METHOD_FULLPATH, SortOrderAscending);
    for (int i = 0; i < itemlist.Size(); i++)
    {
      filePaths.push_back(itemlist[i]->GetPath());
    }
  }

  if ((m_archive =
    g_archiveManager.CreateArchive(m_format, m_filter, filePaths)) == NULL)
  {
    return false;
  }

  if ((m_archive_entry = m_dll.archive_entry_new2(m_archive)) == NULL)
  {
    Close();
    return false;
  }

  return true;
}

bool CArchiveFile::Load(CStdString const& function)
{
  if (!g_archiveManager.Load())
  {
    CLog::Log(LOGERROR, "%s: Unable to load DllLibArchive.", function.c_str());
    return false;
  }
  m_dll = g_archiveManager.getDllLibArchive();
  return true;
}
#endif // HAVE_LIBARCHIVE
