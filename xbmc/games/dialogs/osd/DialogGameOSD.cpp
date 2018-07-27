/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DialogGameOSD.h"
#include "guilib/WindowIDs.h"

using namespace KODI;
using namespace GAME;

CDialogGameOSD::CDialogGameOSD() :
  CGUIDialog(WINDOW_DIALOG_GAME_OSD, "GameOSD.xml")
{
  // Initialize CGUIWindow
  m_loadType = KEEP_IN_MEMORY;
}
