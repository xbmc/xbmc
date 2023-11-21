/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogBusyNoCancel.h"

#include "ServiceBroker.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "threads/Thread.h"

CGUIDialogBusyNoCancel::CGUIDialogBusyNoCancel(void)
  : CGUIDialog(WINDOW_DIALOG_BUSY_NOCANCEL, "DialogBusy.xml", DialogModalityType::MODAL)
{
  m_loadType = LOAD_ON_GUI_INIT;
}

CGUIDialogBusyNoCancel::~CGUIDialogBusyNoCancel(void) = default;

void CGUIDialogBusyNoCancel::Open_Internal(bool bProcessRenderLoop,
                                           const std::string& param /* = "" */)
{
  m_bLastVisible = true;

  CGUIDialog::Open_Internal(false, param);
}

void CGUIDialogBusyNoCancel::DoProcess(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  bool visible = CServiceBroker::GetGUI()->GetWindowManager().IsModalDialogTopmost(WINDOW_DIALOG_BUSY_NOCANCEL);
  if (!visible && m_bLastVisible)
    dirtyregions.emplace_back(m_renderRegion);
  m_bLastVisible = visible;

  CGUIDialog::DoProcess(currentTime, dirtyregions);
}

void CGUIDialogBusyNoCancel::Render()
{
  if(!m_bLastVisible)
    return;
  CGUIDialog::Render();
}
