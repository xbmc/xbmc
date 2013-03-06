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

#ifndef ARCHIVE_MANAGER_H_
#define ARCHIVE_MANAGER_H_

#if (defined HAVE_CONFIG_H) && (!defined WIN32)
  #include "config.h"
#endif

#ifdef HAVE_LIBARCHIVE

#include "utils/StdString.h"
#include "DllLibArchive.h"

class CArchiveManager
{
public:
  ~CArchiveManager();

  /*
   * Function which creates an archive and does the setup to start reading
   * from the specified filePaths.
   */
  struct archive *CreateArchive(int format, int filter,
                                std::vector<CStdString> filePaths);

  /*
   * Extracts an archive
   */
  bool ExtractArchive(CStdString const& strArchive, CStdString const& strPath,
    CStdString const& strPathInArchive = "", int format = -1, int filter = -1);

  /*
   * Functions to load/unload libarchive library.
   */
  bool Load();
  bool Unload();

  /*
   * Function returning instance of libarchive library object.
   */
  DllLibArchive const& getDllLibArchive() const;
private:
  CCriticalSection m_critSection;
  DllLibArchive m_dllLibArchive;
};

extern CArchiveManager g_archiveManager;
#endif // HAVE_LIBARCHIVE

#endif
