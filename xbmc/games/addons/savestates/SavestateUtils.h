/*
 *  Copyright (C) 2016-2017 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

namespace KODI
{
namespace GAME
{
  class CSavestate;

  class CSavestateUtils
  {
  public:
    /*!
     * \brief Calculate a path for the specified savestate
     *
     * The savestate path is the game path with the extension replaced by ".sav".
     */
    static std::string MakePath(const CSavestate& save);

    /*!
     * \brief Calculate a metadata path for the specified savestate
     *
     * The savestate metadata path is the game path with the extension replaced
     * by ".xml".
     */
    static std::string MakeMetadataPath(const std::string &gamePath);
  };
}
}
