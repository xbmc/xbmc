/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogFullScreenInfo.h"

#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"

CGUIDialogFullScreenInfo::CGUIDialogFullScreenInfo(void)
    : CGUIDialog(WINDOW_DIALOG_FULLSCREEN_INFO, "DialogFullScreenInfo.xml")
{
  m_loadType = KEEP_IN_MEMORY;
}

CGUIDialogFullScreenInfo::~CGUIDialogFullScreenInfo(void) = default;

bool CGUIDialogFullScreenInfo::OnAction(const CAction &action)
{
  if (action.GetID() == ACTION_SHOW_INFO)
  {
    Close();
    return true;
  }
  return CGUIDialog::OnAction(action);
}

