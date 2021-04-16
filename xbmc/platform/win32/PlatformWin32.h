/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "platform/Platform.h"

class CPlatformWin32 : public CPlatform
{
public:
  CPlatformWin32() = default;

  ~CPlatformWin32() override = default;

  bool Init() override;
};
