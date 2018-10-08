/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include<string>

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
