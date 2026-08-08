/*
 *  Copyright (C) 2026 Team Kodi
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "platform/posix/PlatformPosix.h"

class CPlatformWasm : public CPlatformPosix
{
public:
  bool InitStageOne() override;
  void DeinitStageOne() override;
};
