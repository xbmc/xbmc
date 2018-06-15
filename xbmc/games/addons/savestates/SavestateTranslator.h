/*
 *  Copyright (C) 2016-2017 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "Savestate.h"

#include <string>

namespace KODI
{
namespace GAME
{
  class CSavestateTranslator
  {
  public:
    static SAVETYPE TranslateType(const std::string& type);
    static std::string TranslateType(const SAVETYPE& type);
  };
}
}
