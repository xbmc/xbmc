/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

namespace KODI
{
namespace RETRO
{
  /*!
   * \brief Type of save action, either:
   *
   *   - automatic (saving was not prompted by the user)
   *   - manual (user manually prompted the save)
   */
  enum class SAVE_TYPE
  {
    UNKNOWN,
    AUTO,
    MANUAL,
  };
}
}
