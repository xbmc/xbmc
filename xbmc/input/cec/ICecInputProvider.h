/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <optional>

class CKey;

namespace KODI::CEC
{
class ICecInputProvider
{
public:
  virtual ~ICecInputProvider() = default;

  /*!
   * \brief Try to get a keypress from a CEC peripheral
   */
  virtual std::optional<CKey> GetCecKey() = 0;
};
} // namespace KODI::CEC
