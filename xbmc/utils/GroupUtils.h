#pragma once
/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      http://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "utils/SortUtils.h"

class CFileItemList;

// can be used as a flag
typedef enum {
  GroupByNone = 0x0,
  GroupBySet  = 0x1
} GroupBy;

typedef enum {
  GroupAttributeNone              = 0x0,
  GroupAttributeIgnoreSingleItems = 0x1
} GroupAttribute;

class GroupUtils
{
public:
  static bool Group(GroupBy groupBy, const std::string &baseDir, const CFileItemList &items, CFileItemList &groupedItems, GroupAttribute groupAttributes = GroupAttributeNone);
  static bool Group(GroupBy groupBy, const std::string &baseDir, const CFileItemList &items, CFileItemList &groupedItems, CFileItemList &ungroupedItems, GroupAttribute groupAttributes = GroupAttributeNone);
  static bool GroupAndMix(GroupBy groupBy, const std::string &baseDir, const CFileItemList &items, CFileItemList &groupedItemsMixed, GroupAttribute groupAttributes = GroupAttributeNone);
};
