/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogPVRGuideControls.h"

using namespace PVR;

CGUIDialogPVRGuideControls::CGUIDialogPVRGuideControls()
  : CGUIDialog(WINDOW_DIALOG_PVR_GUIDE_CONTROLS, "DialogPVRGuideControls.xml")
{
  m_loadType = KEEP_IN_MEMORY;
}

CGUIDialogPVRGuideControls::~CGUIDialogPVRGuideControls() = default;
