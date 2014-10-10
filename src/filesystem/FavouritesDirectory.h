#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "IDirectory.h"

class CFileItemList;
class CFileItem;

namespace XFILE
{

  class CFavouritesDirectory : public IDirectory
  {
  public:
    virtual bool GetDirectory(const CURL& url, CFileItemList &items);
    virtual bool Exists(const CURL& url);
    static bool Load(CFileItemList &items);
    static bool LoadFavourites(const std::string& strPath, CFileItemList& items);

    static bool AddOrRemove(CFileItem *item, int contextWindow);
    static bool Save(const CFileItemList& items);
    static bool IsFavourite(CFileItem *item, int contextWindow);

    static std::string GetExecutePath(const CFileItem &item, int contextWindow);
    static std::string GetExecutePath(const CFileItem &item, const std::string &contextWindow);
  private:
  };
  
}
