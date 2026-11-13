/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIWindowFullScreenWeb.h"

#include "ServiceBroker.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPlayer.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIDialog.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/WindowIDs.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "windowing/GraphicContext.h"
#include "windowing/WinSystem.h"

using namespace KODI::WEB;

CGUIWindowFullScreenWeb::CGUIWindowFullScreenWeb()
  : CGUIWindow(WINDOW_FULLSCREEN_WEB, "WebFullScreen.xml")
{
  m_controlStats = new GUICONTROLSTATS;
  m_loadType = KEEP_IN_MEMORY;
}

CGUIWindowFullScreenWeb::~CGUIWindowFullScreenWeb()
{
  delete m_controlStats;
}

void CGUIWindowFullScreenWeb::Process(unsigned int currentTime, CDirtyRegionList& dirtyregion)
{
  const auto& components = CServiceBroker::GetAppComponents();
  const auto appPlayer = components.GetComponent<CApplicationPlayer>();
  // the page is drawn by the render pass unless it has a plane of its own
  if (!appPlayer->IsRenderingVideoLayer())
    MarkDirtyRegion();

  m_controlStats->Reset();

  CGUIWindow::Process(currentTime, dirtyregion);

  CGraphicContext& gfx = CServiceBroker::GetWinSystem()->GetGfxContext();
  m_renderRegion.SetRect(0, 0, static_cast<float>(gfx.GetWidth()),
                         static_cast<float>(gfx.GetHeight()));
}

void CGUIWindowFullScreenWeb::Render()
{
  CGraphicContext& gfx = CServiceBroker::GetWinSystem()->GetGfxContext();
  if (gfx.GetRenderOrder() != RENDER_ORDER_FRONT_TO_BACK)
  {
    gfx.SetRenderingResolution(gfx.GetVideoResolution(), false);
    auto& components = CServiceBroker::GetAppComponents();
    const auto appPlayer = components.GetComponent<CApplicationPlayer>();
    const bool clear =
        CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_guiGeometryClear;
    appPlayer->Render(clear, 255);
    gfx.SetRenderingResolution(m_coordsRes, m_needsScaling);
  }
  CGUIWindow::Render();
}

void CGUIWindowFullScreenWeb::RenderEx()
{
  CGUIWindow::RenderEx();
  CGraphicContext& gfx = CServiceBroker::GetWinSystem()->GetGfxContext();
  gfx.SetRenderingResolution(gfx.GetVideoResolution(), false);
  auto& components = CServiceBroker::GetAppComponents();
  const auto appPlayer = components.GetComponent<CApplicationPlayer>();
  appPlayer->Render(false, 255, false);
  gfx.SetRenderingResolution(m_coordsRes, m_needsScaling);
}

bool CGUIWindowFullScreenWeb::OnAction(const CAction& action)
{
  switch (action.GetID())
  {
    case ACTION_SHOW_OSD:
    case ACTION_TRIGGER_OSD:
      ToggleOSD();
      return true;
    case ACTION_SHOW_GUI:
      CServiceBroker::GetGUI()->GetWindowManager().PreviousWindow();
      return true;
    default:
      break;
  }

  // the pointer belongs to the page while this window is up
  if (action.IsMouse())
    return true;

  return CGUIWindow::OnAction(action);
}

bool CGUIWindowFullScreenWeb::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
    case GUI_MSG_WINDOW_INIT:
    {
      const auto& components = CServiceBroker::GetAppComponents();
      const auto appPlayer = components.GetComponent<CApplicationPlayer>();
      if (!appPlayer->IsWebPlaying())
      {
        CServiceBroker::GetGUI()->GetWindowManager().PreviousWindow();
        return true;
      }
      break;
    }
    case GUI_MSG_SETFOCUS:
    case GUI_MSG_LOSTFOCUS:
    {
      if (message.GetSenderId() != WINDOW_FULLSCREEN_WEB)
        return true;
      break;
    }
    default:
      break;
  }

  return CGUIWindow::OnMessage(message);
}

void CGUIWindowFullScreenWeb::ClearBackground()
{
  const auto& components = CServiceBroker::GetAppComponents();
  const auto appPlayer = components.GetComponent<CApplicationPlayer>();
  CGraphicContext& gfx = CServiceBroker::GetWinSystem()->GetGfxContext();
  if (appPlayer->IsRenderingVideoLayer())
    gfx.Clear(0);
  else if (!CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_guiGeometryClear)
    gfx.Clear(0xff000000);
  else
    gfx.Clear();
}

bool CGUIWindowFullScreenWeb::HasVisibleControls()
{
  return m_controlStats->nCountVisible > 0;
}

void CGUIWindowFullScreenWeb::OnWindowLoaded()
{
  CGUIWindow::OnWindowLoaded();
  // the page covers the screen, nothing behind it needs clearing
  m_clearBackground = 0;
}

void CGUIWindowFullScreenWeb::OnInitWindow()
{
  CServiceBroker::GetWinSystem()->GetGfxContext().SetFullScreenVideo(true);
  CGUIWindow::OnInitWindow();
}

void CGUIWindowFullScreenWeb::OnDeinitWindow(int nextWindowID)
{
  CServiceBroker::GetGUI()->GetWindowManager().CloseInternalModalDialogs(true);
  CGUIWindow::OnDeinitWindow(nextWindowID);
  CServiceBroker::GetWinSystem()->GetGfxContext().SetFullScreenVideo(false);
}

void CGUIWindowFullScreenWeb::ToggleOSD()
{
  CGUIDialog* osd = CServiceBroker::GetGUI()->GetWindowManager().GetDialog(WINDOW_DIALOG_WEB_OSD);
  if (!osd)
    return;

  if (osd->IsDialogRunning())
    osd->Close();
  else
    osd->Open();

  MarkDirtyRegion();
}
