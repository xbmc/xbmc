/*
 *  Copyright (C) 2022-2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

namespace KODI
{
namespace NETWORK
{
class ILibtorrent
{
public:
  virtual ~ILibtorrent() = default;

  // Lifecycle functions
  virtual void Start() = 0;
  virtual bool IsOnline() const = 0;
  virtual void Stop(bool bWait) = 0;
};
} // namespace NETWORK
} // namespace KODI
