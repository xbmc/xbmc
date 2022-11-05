/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GameWindowFullScreen.h"

#include "GUIInfoManager.h" //! @todo Remove me
#include "GameWindowFullScreenText.h"
#include "ServiceBroker.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPlayer.h" //! @todo Remove me
#include "cores/RetroPlayer/guibridge/GUIGameRenderManager.h"
#include "cores/RetroPlayer/guibridge/GUIRenderHandle.h"
#include "games/GameServices.h"
#include "games/GameSettings.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIControl.h"
#include "guilib/GUIDialog.h"
#include "guilib/GUIWindowManager.h" //! @todo Remove me
#include "guilib/WindowIDs.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "windowing/GraphicContext.h" //! @todo Remove me

using namespace KODI;
using namespace KODI::GUILIB;
using namespace RETRO;

CGameWindowFullScreen::CGameWindowFullScreen(void)
  : CGUIWindow(WINDOW_FULLSCREEN_GAME, "VideoFullScreen.xml"),
    m_fullscreenText(new CGameWindowFullScreenText(*this))
{
  // initialize CGUIControl
  m_controlStats = new GUICONTROLSTATS;

  // initialize CGUIWindow
  m_loadType = KEEP_IN_MEMORY;

  RegisterWindow();
}

CGameWindowFullScreen::~CGameWindowFullScreen()
{
  UnregisterWindow();

  delete m_controlStats;
}

void CGameWindowFullScreen::Process(unsigned int currentTime, CDirtyRegionList& dirtyregion)
{
  if (m_renderHandle->IsDirty())
    MarkDirtyRegion();

  m_controlStats->Reset();

  CGUIWindow::Process(currentTime, dirtyregion);

  //! @todo This isn't quite optimal - ideally we'd only be dirtying up the actual video render rect
  //!       which is probably the job of the renderer as it can more easily track resizing etc.
  m_renderRegion.SetRect(
      0, 0, static_cast<float>(CServiceBroker::GetWinSystem()->GetGfxContext().GetWidth()),
      static_cast<float>(CServiceBroker::GetWinSystem()->GetGfxContext().GetHeight()));
}

void CGameWindowFullScreen::Render()
{
  m_renderHandle->Render();

  CGUIWindow::Render();
}

void CGameWindowFullScreen::RenderEx()
{
  CGUIWindow::RenderEx();

  m_renderHandle->RenderEx();
}

bool CGameWindowFullScreen::OnAction(const CAction& action)
{
  switch (action.GetID())
  {
    case ACTION_SHOW_OSD:
    case ACTION_TRIGGER_OSD:
    {
      TriggerOSD();
      return true;
    }
    case ACTION_MOUSE_MOVE:
    {
      if (action.GetAmount(2) || action.GetAmount(3))
      {
        TriggerOSD();
        return true;
      }
      break;
    }
    case ACTION_MOUSE_LEFT_CLICK:
    {
      TriggerOSD();
      return true;
    }
    case ACTION_SHOW_GUI:
    {
      // Switch back to the menu
      CServiceBroker::GetGUI()->GetWindowManager().PreviousWindow();
      return true;
    }
    case ACTION_ASPECT_RATIO:
    {
      // Toggle the aspect ratio mode (only if the info is onscreen)
      // g_application.GetAppPlayer().SetRenderViewMode(CViewModeSettings::GetNextQuickCycleViewMode(CMediaSettings::GetInstance().GetCurrentVideoSettings().m_ViewMode));
      return true;
    }
    default:
      break;
  }

  return CGUIWindow::OnAction(action);
}

bool CGameWindowFullScreen::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
    case GUI_MSG_SETFOCUS:
    case GUI_MSG_LOSTFOCUS:
    {
      if (message.GetSenderId() != WINDOW_FULLSCREEN_GAME)
        return true;
      break;
    }
    default:
      break;
  }

  return CGUIWindow::OnMessage(message);
}

void CGameWindowFullScreen::FrameMove()
{
  m_fullscreenText->FrameMove();

  CGUIWindow::FrameMove();
}

void CGameWindowFullScreen::ClearBackground()
{
  m_renderHandle->ClearBackground();

  CGUIWindow::ClearBackground();
}

bool CGameWindowFullScreen::HasVisibleControls()
{
  return m_controlStats->nCountVisible > 0;
}

void CGameWindowFullScreen::OnWindowLoaded()
{
  CGUIWindow::OnWindowLoaded();

  // Override the clear colour - we must never clear fullscreen
  m_clearBackground = 0;

  m_fullscreenText->OnWindowLoaded();
}

void CGameWindowFullScreen::OnInitWindow()
{
  GUIINFO::CPlayerGUIInfo& guiInfo =
      CServiceBroker::GetGUI()->GetInfoManager().GetInfoProviders().GetPlayerInfoProvider();
  guiInfo.SetShowInfo(false);

  // Switch resolution
  CServiceBroker::GetWinSystem()->GetGfxContext().SetFullScreenVideo(true); //! @todo

  CGUIWindow::OnInitWindow();

  // Show OSD help
  GAME::CGameSettings& gameSettings = CServiceBroker::GetGameServices().GameSettings();
  if (gameSettings.ShowOSDHelp())
    TriggerOSD();
  else
  {
    //! @todo We need to route this check through the GUI bridge. By adding the
    //        dependency to the application player here, we are prevented from
    //        having multiple players.
    const auto& components = CServiceBroker::GetAppComponents();
    const auto appPlayer = components.GetComponent<CApplicationPlayer>();
    if (!appPlayer->HasGameAgent())
    {
      gameSettings.SetShowOSDHelp(true);
      TriggerOSD();
    }
  }
}

void CGameWindowFullScreen::OnDeinitWindow(int nextWindowID)
{
  // Close all active modal dialogs
  CServiceBroker::GetGUI()->GetWindowManager().CloseInternalModalDialogs(true);

  CGUIWindow::OnDeinitWindow(nextWindowID);

  CServiceBroker::GetWinSystem()->GetGfxContext().SetFullScreenVideo(false); //! @todo
}

void CGameWindowFullScreen::TriggerOSD()
{
  CGUIDialog* pOSD = GetOSD();
  if (pOSD != nullptr)
  {
    if (!pOSD->IsDialogRunning())
      pOSD->Open();
  }
}

CGUIDialog* CGameWindowFullScreen::GetOSD()
{
  return CServiceBroker::GetGUI()->GetWindowManager().GetDialog(WINDOW_DIALOG_GAME_OSD);
}

void CGameWindowFullScreen::RegisterWindow()
{
  m_renderHandle = CServiceBroker::GetGameRenderManager().RegisterWindow(*this);
}

void CGameWindowFullScreen::UnregisterWindow()
{
  m_renderHandle.reset();
}
