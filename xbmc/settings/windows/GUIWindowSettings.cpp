/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIWindowSettings.h"

#include "guilib/WindowIDs.h"

CGUIWindowSettings::CGUIWindowSettings(void)
    : CGUIWindow(WINDOW_SETTINGS_MENU, "Settings.xml")
{
  m_loadType = KEEP_IN_MEMORY;
}

CGUIWindowSettings::~CGUIWindowSettings(void) = default;
