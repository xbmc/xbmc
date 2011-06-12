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

#include "system.h"
#include "ArchiveManager.h"
#include "filesystem/File.h"
#include "utils/log.h"
#include "URL.h"
#include "utils/URIUtils.h"
#include "SpecialProtocol.h"
#include "Util.h"
#include "XFileUtils.h"
#include "threads/SingleLock.h"

#include <sys/stat.h>

#define LIBARCHIVE_BLOCK_SIZE 10240

using namespace XFILE;
using namespace std;

struct mydata
{
  CFile *file;
  void *buffer;
};

static int  file_close(struct archive *, void *);
static ssize_t  file_read(struct archive *, void *, const void **buff);
static int64_t  file_skip(struct archive *, void *, int64_t request);

static ssize_t file_read(struct archive *a, void *client_data, const void **buff)
{
  struct mydata *data = (struct mydata*)client_data;
  CFile *file = data->file;
  *buff = data->buffer;
  return file->Read(data->buffer, LIBARCHIVE_BLOCK_SIZE);
}

static int64_t
file_skip(struct archive *a, void *client_data, int64_t request)
{
  struct mydata *data = (struct mydata*)client_data;
  CFile *file = data->file;
  int64_t result = file->Seek(request, SEEK_CUR);
  if (result >= 0)
    return result;

  /* Let libarchive recover with read+discard. */
  if (errno == ESPIPE)
    return 0;
  CLog::Log(LOGERROR, "CArchiveManager - %s: Error seeking", __FUNCTION__);
  return -1;
}

static int
file_close(struct archive *a, void *client_data)
{
  struct mydata *data = (struct mydata*)client_data;
  CFile *file = data->file;
  free(data->buffer);
  file->Close();
  return ARCHIVE_OK;
}

CArchiveManager g_archiveManager;

CArchiveEntry::CArchiveEntry()
{
  m_file.clear();
  m_file_stat = NULL;
  m_cache_path.clear();
}
CArchiveEntry::~CArchiveEntry()
{
  CStdString strCachedDir;
  if (m_cache_path.length() > 0)
  {
    CFile::Delete(m_cache_path);
    URIUtils::GetDirectory(m_cache_path, strCachedDir);
    RemoveDirectory(strCachedDir.c_str());
  }
}
void CArchiveEntry::get_file(CStdString &file)
{
  file = m_file;
}
void CArchiveEntry::set_file(const CStdString &file)
{
  m_file = file;
}
struct stat *CArchiveEntry::get_file_stat()
{
  return m_file_stat;
}
void CArchiveEntry::set_file_stat(const struct stat *file_stat)
{
  m_file_stat = (struct stat*)file_stat;
}
void CArchiveEntry::get_cache_path(CStdString &cache_path)
{
  cache_path = m_cache_path;
}
void CArchiveEntry::set_cache_path(const CStdString &cache_path)
{
  m_cache_path = cache_path;
}

CArchiveManager::CArchiveManager()
{
  m_archiveMap.clear();
}

CArchiveManager::~CArchiveManager()
{
  m_archiveMap.clear();
  if (m_dllLibArchive.IsLoaded())
    m_dllLibArchive.Unload();
}

bool CArchiveManager::GetArchiveList(const CStdString &strPath,
                                     deque<CArchiveEntry> &items)
{
  CURL url(strPath);
  CStdString strFile = url.GetHostName();
  return libarchive_list(strFile, items);
}

bool CArchiveManager::GetArchiveEntry(const CStdString &strPath,
                                      CArchiveEntry &item)
{
  CURL url(strPath);

  CStdString strFile = url.GetHostName();

  map<CStdString,deque<CArchiveEntry> >::iterator it = m_archiveMap.find(strFile);
  deque<CArchiveEntry> items;
  if (it == m_archiveMap.end()) // we need to list the archive
  {
    GetArchiveList(strPath, items);
  }
  else
  {
    items = it->second;
  }

  CStdString strFileName = url.GetFileName();
  for (deque<CArchiveEntry>::iterator it2=items.begin();it2 != items.end();++it2)
  {
    CStdString s;
    it2->get_file(s);
    if (s == strFileName)
    {
      item = *it2;
      return true;
    }
  }
  return false;
}

bool CArchiveManager::ExtractArchive(const CStdString &strArchive,
                                     const CStdString &strPath,
                                     const CStdString &strPathInArchive)
{
  return libarchive_extract(strArchive, strPath, strPathInArchive);
}

bool CArchiveManager::CacheArchivedPath(const CStdString &strArchive,
                                        CArchiveEntry &item)
{
  CStdString strCachedPath, strPathInArchive, strDir(_P("special://temp/"));
  item.get_file(strPathInArchive);
  URIUtils::AddFileToFolder(strDir + "archivefolder%04d",
                            URIUtils::GetFileName(strPathInArchive),
                            strCachedPath);
  strCachedPath = CUtil::GetNextPathname(strCachedPath, 9999);
  if (strCachedPath.IsEmpty())
  {
    CLog::Log(LOGWARNING, "%s: Could not cache file %s from archive %s.",
              __FUNCTION__, strPathInArchive.c_str(), strArchive.c_str());
    return false;
  }
  strCachedPath = CUtil::MakeLegalPath(strCachedPath);
  CStdString strCachedDir;
  URIUtils::GetDirectory(strCachedPath, strCachedDir);
  if (!libarchive_extract(strArchive, strCachedDir, strPathInArchive))
    return false;
  item.set_cache_path(strCachedPath.c_str());
  return true;
}

bool CArchiveManager::Load()
{
  if (!m_dllLibArchive.IsLoaded())
    return m_dllLibArchive.Load();
  return true;
}

bool CArchiveManager::libarchive_extract(const CStdString &strArchive,
                                         const CStdString &strPath,
                                         const CStdString &strPathInArchive)
{
  struct archive *a;
  struct archive *ext;
  struct archive_entry *entry;
  int r;
  const void *buff;
  size_t size;
  off_t offset;

  int flags = ARCHIVE_EXTRACT_PERM | ARCHIVE_EXTRACT_TIME;

  if (!Load())
  {
    CLog::Log(LOGERROR, "%s: Unable to load DllLibArchive.", __FUNCTION__);
    return false;
  }
  a = m_dllLibArchive.archive_read_new();
  m_dllLibArchive.archive_read_support_format_all(a);
  m_dllLibArchive.archive_read_support_compression_all(a);
  ext = m_dllLibArchive.archive_write_disk_new();
  m_dllLibArchive.archive_write_disk_set_options(ext, flags);
  m_dllLibArchive.archive_write_disk_set_standard_lookup(ext);

  CFile file;
  if (!file.Open(strArchive))
  {
    CLog::Log(LOGERROR,"%s: Unable to open file '%s'.", __FUNCTION__,
              strPath.c_str());
    return false;
  }
  struct mydata data;
  data.file = &file;
  data.buffer = calloc(1, LIBARCHIVE_BLOCK_SIZE);
  r = m_dllLibArchive.archive_read_open2(a, &data, NULL, file_read, file_skip, file_close);
  if (r != ARCHIVE_OK)
  {
    CLog::Log(LOGERROR,"%s", m_dllLibArchive.archive_error_string(a));
    return false;
  }
  while (1)
  {
    r = m_dllLibArchive.archive_read_next_header(a, &entry);
    if (r == ARCHIVE_EOF)
      break;

    /* Extract single file specified by strPathInArchive if defined */
    if (strPathInArchive.length() > 0 &&
      strPathInArchive.compare(m_dllLibArchive.archive_entry_pathname(entry)) != 0)
      continue;

    /* Extract to path if strPath specified, otherwise extract to current
     * directory.
     */
    if (strPath.length() > 0 && strPath.compare(".") != 0)
    {
      CStdString destpath(strPath);
      URIUtils::AddSlashAtEnd(destpath);
      destpath.append(m_dllLibArchive.archive_entry_pathname(entry));
      m_dllLibArchive.archive_entry_set_pathname(entry, destpath);
    }
    if (r != ARCHIVE_OK)
      CLog::Log(LOGERROR,"%s", m_dllLibArchive.archive_error_string(a));
    if (r < ARCHIVE_WARN)
      return false;
    r = m_dllLibArchive.archive_write_header(ext, entry);
    if (r != ARCHIVE_OK)
      CLog::Log(LOGERROR,"%s", m_dllLibArchive.archive_error_string(ext));
    else if (m_dllLibArchive.archive_entry_size(entry) > 0) {
      while (1) {
        r = m_dllLibArchive.archive_read_data_block(a, &buff, &size, &offset);
        if (r == ARCHIVE_EOF)
          break;
        r = m_dllLibArchive.archive_write_data_block(ext, buff, size, offset);
        if (r != ARCHIVE_OK) {
          CLog::Log(LOGERROR,"%s", m_dllLibArchive.archive_error_string(ext));
          break;
        }
      }
      if (r < ARCHIVE_WARN)
        return false;
    }
    r = m_dllLibArchive.archive_write_finish_entry(ext);
    if (r != ARCHIVE_OK)
      CLog::Log(LOGERROR,"%s", m_dllLibArchive.archive_error_string(ext));
    if (r < ARCHIVE_WARN)
      return false;
    if (strPathInArchive.length() > 0)
      break;
  }

  m_dllLibArchive.archive_read_close(a);
  m_dllLibArchive.archive_write_close(ext);
  return true;
}

bool CArchiveManager::libarchive_list(const CStdString &strPath,
                                      deque<CArchiveEntry> &items)
{
  CSingleLock lock(m_CritSection);
  map<CStdString,deque<CArchiveEntry> >::iterator it = m_archiveMap.find(strPath);
  if (it != m_archiveMap.end())
  {
    items = (*it).second;
    return true;
  }

  struct archive *a;
  struct archive_entry *entry;
  int r;

  if (!Load())
  {
    CLog::Log(LOGERROR, "%s: Unable to load DllLibArchive.", __FUNCTION__);
    return false;
  }
  a = m_dllLibArchive.archive_read_new();
  m_dllLibArchive.archive_read_support_compression_all(a);
  m_dllLibArchive.archive_read_support_format_all(a);

  CFile file;
  if (!file.Open(strPath))
  {
    CLog::Log(LOGERROR,"%s: Unable to open file '%s'.", __FUNCTION__,
              strPath.c_str());
    return false;
  }
  struct mydata data;
  data.file = &file;
  data.buffer = calloc(1, LIBARCHIVE_BLOCK_SIZE);
  r = m_dllLibArchive.archive_read_open2(a, &data, NULL, file_read, file_skip, file_close);
  if (r != ARCHIVE_OK)
  {
    CLog::Log(LOGERROR,"%s", m_dllLibArchive.archive_error_string(a));
    return false;
  }
  while (m_dllLibArchive.archive_read_next_header(a, &entry) == ARCHIVE_OK) {
    CArchiveEntry e;
    CStdString file(m_dllLibArchive.archive_entry_pathname(entry));
    e.set_file(file);
    e.set_file_stat(m_dllLibArchive.archive_entry_stat(entry));
    items.push_back(e);
  }
  r = m_dllLibArchive.archive_read_close(a);
  if (r != ARCHIVE_OK)
  {
    CLog::Log(LOGERROR,"Error closing archive file '%s'",
              strPath.c_str());
    return false;
  }
  m_archiveMap.insert(pair<CStdString,deque<CArchiveEntry> >(strPath, items));
  return true;
}
