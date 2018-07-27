/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */
#include "powermanagement/WinIdleTimer.h"
#include "Application.h"

void CWinIdleTimer::StartZero()
{
  if (!g_application.IsDPMSActive())
    SetThreadExecutionState(ES_SYSTEM_REQUIRED|ES_DISPLAY_REQUIRED);
  CStopWatch::StartZero();
}
