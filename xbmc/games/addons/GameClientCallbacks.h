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
namespace GAME
{
  class IGameInputCallback
  {
  public:
    virtual ~IGameInputCallback() = default;

    virtual void PollInput() = 0;
  };
}
}
