/*
* XBMC
* 7z Filesystem
* Copyright (c) 2008 topfs2
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#pragma once
#include "IFileDirectory.h"
#include "7zManager.h"

namespace DIRECTORY
{
  class C7zDirectory : public IFileDirectory
  {
  public:
    C7zDirectory(void);
    virtual ~C7zDirectory(void);
    virtual bool GetDirectory(const CStdString& strPathOrig, CFileItemList &items);
    virtual bool ContainsFiles(const CStdString& strPath);
    virtual bool Exists(const char* strPath);
  private:
  };
}
