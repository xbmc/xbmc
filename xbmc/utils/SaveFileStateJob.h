/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

class CBookmark;
class CFileItem;

class CSaveFileState
{
public:
  static void DoWork(CFileItem& item,
                     CBookmark& bookmark,
                     bool updatePlayCount);

  //! The file an item's resume point and play count are kept against
  static std::string ProgressTrackingFileOf(const CFileItem& item);
};

