/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/GUIWindow.h"

#define TEST_PATTERNS_COUNT 5
#define TEST_PATTERNS_BOUNCE_SQUARE_SIZE 100
#define TEST_PATTERNS_BLINK_CYCLE 100

class CGUIWindowTestPattern : public CGUIWindow
{
public:
  CGUIWindowTestPattern(void);
  ~CGUIWindowTestPattern(void) override;
  void Render() override;
  void Process(unsigned int currentTime, CDirtyRegionList& dirtyregions) override;
};
