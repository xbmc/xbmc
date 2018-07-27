/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <cstdint>
#include <string>

class ILocalizer
{
public:
  virtual ~ILocalizer() = default;

  virtual std::string Localize(std::uint32_t code) const = 0;

protected:
  ILocalizer() = default;
};
