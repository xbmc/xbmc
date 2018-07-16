/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogPlayerControls.h"


CGUIDialogPlayerControls::CGUIDialogPlayerControls(void)
    : CGUIDialog(WINDOW_DIALOG_PLAYER_CONTROLS, "PlayerControls.xml")
{
  m_loadType = KEEP_IN_MEMORY;
}

CGUIDialogPlayerControls::~CGUIDialogPlayerControls(void) = default;

