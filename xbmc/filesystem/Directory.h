#pragma once
/*
 *      Copyright (C) 2005-2008 Team XBMC
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

#include "IDirectory.h"

namespace XFILE
{
/*!
 \ingroup filesystem
 \brief Wrappers for \e IDirectory
 */
class CDirectory
{
public:
  CDirectory(void);
  virtual ~CDirectory(void);

  class CHints
  {
  public:
    CHints() : flags(DIR_FLAG_DEFAULTS), scanning_type(0)
    {
    };
    CStdString mask;
    int flags;
    int scanning_type;  /* content type we are looking for during scanning */
  };

  static bool GetDirectory(const CStdString& strPath
                           , CFileItemList &items
                           , const CStdString &strMask=""
                           , int flags=DIR_FLAG_DEFAULTS
                           , bool allowThreads=false);

  static bool GetDirectory(const CStdString& strPath
                           , CFileItemList &items
                           , const CHints &hints
                           , bool allowThreads=false);

  static bool Create(const CStdString& strPath);
  static bool Exists(const CStdString& strPath);
  static bool Remove(const CStdString& strPath);

  /*! \brief Filter files that act like directories from the list, replacing them with their directory counterparts
   \param items The item list to filter
   \param mask  The mask to apply when filtering files */
  static void FilterFileDirectories(CFileItemList &items, const CStdString &mask);
};
}
