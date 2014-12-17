#ifndef UDF_DIRECTORY_H
#define UDF_DIRECTORY_H
/*
 *      Copyright (C) 2010 Team Boxee
 *      http://www.boxee.tv
 *
 *      Copyright (C) 2010-2013 Team XBMC
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
#include "IFileDirectory.h"

namespace XFILE
{
class CUDFDirectory :
      public IFileDirectory
{
public:
  CUDFDirectory(void);
  virtual ~CUDFDirectory(void);
  virtual bool GetDirectory(const CURL& url, CFileItemList &items);
  virtual bool Exists(const CURL& url);
  virtual bool ContainsFiles(const CURL& url) { return true; }
};
}

#endif
