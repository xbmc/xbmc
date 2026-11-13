/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogWebKeyboard.h"

#include "ServiceBroker.h"
#include "guilib/GUIControl.h"
#include "guilib/WindowIDs.h"
#include "windowing/GraphicContext.h"
#include "windowing/WinSystem.h"

#include <mutex>

using namespace KODI::WEB;

namespace
{

constexpr int CTL_GROUP_KEYS = 9100;

} // unnamed namespace

CGUIDialogWebKeyboard::CGUIDialogWebKeyboard()
  : CGUIDialogKeyboardGeneric(WINDOW_DIALOG_WEB_KEYBOARD, "DialogWebKeyboard.xml")
{
}

int CGUIDialogWebKeyboard::GetCoveredTop()
{
  CGraphicContext& gfx = CServiceBroker::GetWinSystem()->GetGfxContext();
  const CGUIControl* keys = GetControl(CTL_GROUP_KEYS);
  if (!keys)
    return gfx.GetHeight();

  std::unique_lock lock(gfx);
  gfx.SetScalingResolution(m_coordsRes, m_needsScaling);
  return static_cast<int>(gfx.ScaleFinalYCoord(keys->GetXPosition(), keys->GetYPosition()));
}
