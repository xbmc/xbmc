/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "KeymapEnvironment.h"

#include "WindowTranslator.h"

int CKeymapEnvironment::GetFallthrough(int windowId) const
{
  return CWindowTranslator::GetFallbackWindow(windowId);
}
