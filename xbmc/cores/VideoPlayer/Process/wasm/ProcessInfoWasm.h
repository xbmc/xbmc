/*
 *  Copyright (C) 2026 Team Kodi
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "cores/VideoPlayer/Process/ProcessInfo.h"

class CProcessInfoWasm : public CProcessInfo
{
public:
  static CProcessInfo* Create();
  static void Register();

  EINTERLACEMETHOD GetFallbackDeintMethod() override;
};
