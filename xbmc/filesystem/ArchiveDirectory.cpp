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

#include "ArchiveDirectory.h"
#include "ArchiveManager.h"
#include "utils/URIUtils.h"
#include "utils/RegExp.h"
#include "FileItem.h"
#include "utils/log.h"
#include "URL.h"
#include "Directory.h"

#include <functional>
#include <map>

using namespace XFILE;
using namespace std;

/* Comparator for CStdString used in std::map */
struct CStdStringCompare : public binary_function<CStdString, CStdString, bool>
{
  bool operator() (CStdString const& a, CStdString const& b)
  {
    return (a.compare(b) < 0) ? true : false;
  }
};

CArchiveDirectory::CArchiveDirectory()
{
  m_format = -1;
  m_filter = -1;
}

CArchiveDirectory::CArchiveDirectory(std::vector<CStdString> const& filePaths)
{
  m_format = -1;
  m_filter = -1;
  m_filePaths = filePaths;
}

CArchiveDirectory::CArchiveDirectory(int format, int filter)
{
  m_format = format;
  m_filter = filter;
}

CArchiveDirectory::CArchiveDirectory(int format, int filter,
                                     std::vector<CStdString> const& filePaths)
{
  m_format = format;
  m_filter = filter;
  m_filePaths = filePaths;
}

bool CArchiveDirectory::GetDirectory(const CStdString& strPathOrig,
                                     CFileItemList &items)
{
  struct archive *a;
  struct archive_entry *entry;
  int count;
  CStdString strPath, pattern, archivePath, archivePathDecoded, pathInArchive;
  CStdString dirInArchiveNoSlash;
  CRegExp regex(true), regex2(true);
  map<CStdString, bool, CStdStringCompare> directories;

  /*
   * if this isn't a proper archive path, assume it's the path to a archive
   * file
   */
  if( !strPathOrig.Left(6).Equals("rar://") )
    URIUtils::CreateArchivePath(strPath, "rar", strPathOrig, "");
  else
    strPath = strPathOrig;

  /*
   * Regex matching standard RAR protocol URL
   * Group 1: RAR protocol URL up to last '.rar/' or '.\d\d\d/'
   * Group 2: the encoded path for the RAR file
   * Group 3: contents in RAR
   */
  pattern = "^(rar://([^/]*?)/)(.*)$";
  if (!regex.RegComp(pattern) || regex.RegFind(strPath) < 0)
  {
    CLog::Log(LOGDEBUG, "%s: Couldn't get regex match for directory in path",
              __FUNCTION__);
    return false;
  }

  archivePath = regex.GetMatch(1);
  archivePathDecoded = CURL::Decode(regex.GetMatch(2));
  pathInArchive = regex.GetMatch(3);

  /*
   * Regex matching content in dir
   * Group 1: Contents of dir
   */
  pattern = "^\\Q" + pathInArchive + "\\E([^/]+/?)$";
  if (!regex.RegComp(pattern))
  {
    CLog::Log(LOGDEBUG, "%s: Couldn't compile regex", __FUNCTION__);
    return false;
  }

  /*
   * Regex finding subdir within dir
   * Group 1: subdir
   */
  pattern = "^\\Q" + pathInArchive + "\\E([^/]+?/).*$";
  if (!regex2.RegComp(pattern))
  {
    CLog::Log(LOGDEBUG, "%s: Couldn't compile regex", __FUNCTION__);
    return false;
  }

  if ((a = OpenArchive(strPathOrig)) == NULL)
    return false;

  count = 0;
  while (m_dll.archive_read_next_header(a, &entry) == ARCHIVE_OK)
  {
    CStdString filepath(m_dll.archive_entry_pathname(entry));
    CFileItemPtr pFileItem(new CFileItem);
    if (regex.RegFind(filepath) >= 0)
    {
      filepath = regex.GetMatch(1);
      const struct stat *entry_stat = m_dll.archive_entry_stat(entry);
      if ((entry_stat->st_mode & (S_IFMT)) == (S_IFDIR))
      {
        URIUtils::AddSlashAtEnd(filepath);
        if (directories[filepath])
          continue;
        pFileItem->m_dwSize = 0;
        pFileItem->m_bIsFolder = true;
        directories[filepath] = true;
      }
      else
        pFileItem->m_dwSize = m_dll.archive_entry_size(entry);
      pFileItem->SetPath(strPath + filepath);
      pFileItem->m_dateTime = m_dll.archive_entry_mtime(entry);

      /*
       * FIXME: What is m_idepth? Right now it's assigned the position of the
       * data block in the archive.
       */
      pFileItem->m_idepth = count;
      items.Add(pFileItem);
    }
    else if (regex2.RegFind(filepath) >= 0)
    {
      /*
       * Archive entries for non-empty directories will sometimes not be
       * written to the archive. These directories are found via regex.
       */
      pathInArchive = regex2.GetMatch(1);
      if (!directories[pathInArchive])
      {
        pFileItem->m_dwSize = 0;
        pFileItem->m_bIsFolder = true;
        pFileItem->SetPath(strPath + pathInArchive);
        pFileItem->m_idepth = count;
        items.Add(pFileItem);
        directories[pathInArchive] = true;
      }
    }
    count++;
  }

  m_dll.archive_read_close(a);
  m_dll.archive_read_free(a);
  return true;
}

bool CArchiveDirectory::Exists(const char* strPath)
{
  struct archive *a;
  struct archive_entry *entry;
  CStdString pattern, pathInArchive;
  CRegExp regex(true), regex2(true), regex3(true);
  bool retval = false;

  /*
   * Regex matching standard RAR protocol URL
   * Group 1: RAR protocol URL up to last '.rar/' or '.\d\d\d/'
   * Group 2: the 'rar' or '\d\d\d' part
   * Group 3: contents in RAR
   */
  pattern = "^(rar://.*\\.(rar|\\d{3})/)(.*)$";
  if (!regex.RegComp(pattern) || regex.RegFind(strPath) < 0)
  {
    CLog::Log(LOGDEBUG, "%s: Couldn't get regex match for directory in path",
              __FUNCTION__);
    return false;
  }

  pathInArchive = regex.GetMatch(3);

  /* Regex checking if dir is proper directory path */
  pattern = "^.+/$";
  if (!regex.RegComp(pattern))
  {
    CLog::Log(LOGDEBUG, "%s: Couldn't compile regex", __FUNCTION__);
    return false;
  }
  else if (regex.RegFind(pathInArchive) < 0)
  {
    CLog::Log(LOGDEBUG, "%s: Specified path is not a directory", __FUNCTION__);
    return false;
  }

  /*
   * Regex matching dir with or without content
   * Group 1: Contents of dir
   */
  pattern = "^\\Q" + pathInArchive + "\\E(.*)$";
  if (!regex.RegComp(pattern))
  {
    CLog::Log(LOGDEBUG, "%s: Couldn't compile regex", __FUNCTION__);
    return false;
  }

  if ((a = OpenArchive(strPath)) == NULL)
    return false;

  while (m_dll.archive_read_next_header(a, &entry) == ARCHIVE_OK)
  {
    CStdString filepath(m_dll.archive_entry_pathname(entry));
    if (regex.RegFind(filepath) >= 0)
    {
      retval = true;
      break;
    }
  }

  m_dll.archive_read_close(a);
  m_dll.archive_read_free(a);
  return retval;
}

DIR_CACHE_TYPE CArchiveDirectory::GetCacheType(const CStdString& strPath) const
{
  return DIR_CACHE_ALWAYS;
}

bool CArchiveDirectory::ContainsFiles(const CStdString& strPath)
{
  struct archive *a;
  struct archive_entry *entry;
  CStdString pattern, pathInArchive;
  CRegExp regex(true);
  bool retval = false;

  /*
   * Regex matching standard RAR protocol URL
   * Group 1: RAR protocol URL up to last '.rar/' or '.\d\d\d/'
   * Group 2: the 'rar' or '\d\d\d' part
   * Group 3: contents in RAR
   */
  pattern = "^(rar://.*\\.(rar|\\d{3})/)(.*)$";
  if (!regex.RegComp(pattern) || regex.RegFind(strPath) < 0)
  {
    CLog::Log(LOGDEBUG, "%s: Couldn't get regex match for directory in path",
              __FUNCTION__);
    return false;
  }

  pathInArchive = regex.GetMatch(3);

  /*
   * Regex matching content in dir
   * Group 1: Contents of dir
   */
  pattern = "^\\Q" + pathInArchive + "\\E([^/]+/?)$";
  if (!regex.RegComp(pattern))
  {
    CLog::Log(LOGDEBUG, "%s: Couldn't compile regex", __FUNCTION__);
    return false;
  }

  if ((a = OpenArchive(strPath)) == NULL)
    return false;

  while (m_dll.archive_read_next_header(a, &entry) == ARCHIVE_OK)
  {
    CStdString filepath(m_dll.archive_entry_pathname(entry));
    if (regex.RegFind(filepath) >= 0)
    {
      const struct stat *entry_stat = m_dll.archive_entry_stat(entry);
      if ((entry_stat->st_mode & (S_IFMT)) != (S_IFDIR))
      {
        retval = true;
        break;
      }
    }
  }

  m_dll.archive_read_close(a);
  m_dll.archive_read_free(a);
  return retval;
}

bool CArchiveDirectory::Load(CStdString const& function)
{
  if (!g_archiveManager.Load())
  {
    CLog::Log(LOGERROR, "%s: Unable to load DllLibArchive.", function.c_str());
    return false;
  }
  m_dll = g_archiveManager.getDllLibArchive();
  return true;
}

struct archive *CArchiveDirectory::OpenArchive(CStdString const& strPath)
{
  struct archive *a;
  CStdString pattern, archivePathDecoded, strPathNoExt;
  CRegExp regex(true);
  CFileItemList itemlist;

  if (!Load(__FUNCTION__))
    return NULL;

  if (m_filePaths.empty())
  {
    /*
     * This occurs when reading a directory within an archive. Get
     * the original paths to the archive files.
     */
    pattern = "^(rar://([^/]*?)/)(.*)$";
    if (!regex.RegComp(pattern) || regex.RegFind(strPath) < 0)
      return NULL;

    archivePathDecoded = CURL::Decode(regex.GetMatch(2));

    pattern = "^(.*)(\\.part\\d+\\.rar)$";
    if (!regex.RegComp(pattern))
      return NULL;

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
      m_filePaths.push_back(archivePathDecoded);
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
        m_filePaths.push_back(itemlist[i]->GetPath());
      }
    }
  }

  if ((a = g_archiveManager.CreateArchive(m_format, m_filter, m_filePaths)) ==
    NULL)
  {
    return NULL;
  }

  if (m_dll.archive_read_open1(a) != ARCHIVE_OK)
  {
    CLog::Log(LOGERROR,"%s: %s", __FUNCTION__, m_dll.archive_error_string(a));
    m_dll.archive_read_free(a);
    return NULL;
  }

  return a;
}
#endif // HAVE_LIBARCHIVE
