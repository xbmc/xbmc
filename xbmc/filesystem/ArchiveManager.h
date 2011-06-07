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

#ifndef ARCHIVE_MANAGER_H_
#define ARCHIVE_MANAGER_H_

#include  "utils/StdString.h"

#include <deque>
#include <map>

class CArchiveEntry
{
public:
  CArchiveEntry();
  ~CArchiveEntry();
  void get_file(CStdString &file);
  void set_file(const CStdString &file);
  struct stat *get_file_stat();
  void set_file_stat(const struct stat *file_stat);
  void get_cache_path(CStdString &cache_path);
  void set_cache_path(const CStdString &cache_path);
private:
  CStdString m_file;
  struct stat *m_file_stat;
  CStdString m_cache_path;
};

class CArchiveManager
{
public:
  CArchiveManager();
  ~CArchiveManager();

  bool GetArchiveList(const CStdString &strPath,
                      std::deque<CArchiveEntry> &items);
  bool GetArchiveEntry(const CStdString &strPath, CArchiveEntry &item);
  bool ExtractArchive(const CStdString &strArchive, const CStdString &strPath,
                      const CStdString &strPathInArchive);
  bool CacheArchivedPath(const CStdString &strArchive,
                         CArchiveEntry &item);
private:
  bool libarchive_extract(const CStdString &strArchive,
                          const CStdString &strPath,
                          const CStdString &strPathInArchive);
  bool libarchive_list(const CStdString &strPath,
                       std::deque<CArchiveEntry> &items);
  std::map<CStdString,std::deque<CArchiveEntry> > m_archiveMap;
};

extern CArchiveManager g_archiveManager;

#endif
