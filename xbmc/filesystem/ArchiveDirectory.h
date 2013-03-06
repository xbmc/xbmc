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

#ifndef ARCHIVEDIRECTORY_H_
#define ARCHIVEDIRECTORY_H_

#if (defined HAVE_CONFIG_H) && (!defined WIN32)
  #include "config.h"
#endif

#ifdef HAVE_LIBARCHIVE

#include "IFileDirectory.h"
#include "DllLibArchive.h"

#include <vector>

namespace XFILE
{
  class CArchiveDirectory : public IFileDirectory
  {
  public:
    CArchiveDirectory();
    CArchiveDirectory(std::vector<CStdString> const& filePaths);
    CArchiveDirectory(int format, int filter);
    CArchiveDirectory(int format, int filter,
                      std::vector<CStdString> const& filePaths);

    virtual bool GetDirectory(const CStdString& strPathOrig,
                              CFileItemList &items);
    virtual bool Exists(const char* strPath);
    virtual DIR_CACHE_TYPE GetCacheType(const CStdString& strPath) const;
    virtual bool ContainsFiles(const CStdString& strPath);
  private:
    int m_format;
    int m_filter;
    std::vector<CStdString> m_filePaths;
    DllLibArchive m_dll;
    bool Load(CStdString const& function);
    struct archive *OpenArchive(CStdString const& strPath);
  };
}

#endif // HAVE_LIBARCHIVE

#endif
