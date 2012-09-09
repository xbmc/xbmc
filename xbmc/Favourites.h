#pragma once

/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "utils/StdString.h"

class CFileItemList;
class CFileItem;

class CFavourites
{
public:
  static bool Load(CFileItemList& items);
  static bool LoadFavourites(CStdString& strPath, CFileItemList& items);
  static bool AddOrRemove(CFileItem *item, int contextWindow);
  static bool Save(const CFileItemList& items);
  static bool IsFavourite(CFileItem *item, int contextWindow);

#ifdef UNIT_TESTING
  static bool TestParamify();
#endif

private:
  static CStdString GetExecutePath(const CFileItem *item, int contextWindow);
};
