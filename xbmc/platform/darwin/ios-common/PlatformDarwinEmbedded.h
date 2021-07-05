/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "platform/darwin/PlatformDarwin.h"

class CPlatformDarwinEmbedded : public CPlatformDarwin
{
public:
  CPlatformDarwinEmbedded() = default;

  ~CPlatformDarwinEmbedded() override = default;

  bool InitStageOne() override;
  bool InitStageTwo() override;

  bool SupportsUserInstalledBinaryAddons() override;
};
