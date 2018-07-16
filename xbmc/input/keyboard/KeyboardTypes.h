/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "input/Key.h"

#include <string>

namespace KODI
{
namespace KEYBOARD
{
  /*!
   * \brief Name of a hardware-indendent symbol representing a key
   *
   * Names are defined in the keyboard's controller profile.
   */
  using KeyName = std::string;

  //! @todo Move CKey enum to this file
  using Modifier = CKey::Modifier;
}
}
