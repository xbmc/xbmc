/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogWebOSD.h"

#include "ServiceBroker.h"
#include "application/ApplicationComponents.h"
#include "guilib/GUIMessage.h"
#include "guilib/WindowIDs.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "messaging/ApplicationMessenger.h"

using namespace KODI::WEB;
using namespace KODI::WAYLANDHOST;

namespace
{

constexpr int CTL_LABEL_TITLE = 2;
constexpr int CTL_LABEL_URL = 3;
constexpr int CTL_BUTTON_BACK = 11;
constexpr int CTL_BUTTON_FORWARD = 12;
constexpr int CTL_BUTTON_RELOAD = 13;
constexpr int CTL_BUTTON_CLOSE = 14;

std::shared_ptr<CWebClientManager> Manager()
{
  return CServiceBroker::GetAppComponents().GetComponent<CWebClientManager>();
}

} // unnamed namespace

CGUIDialogWebOSD::CGUIDialogWebOSD() : CGUIDialog(WINDOW_DIALOG_WEB_OSD, "DialogWebOSD.xml")
{
  m_loadType = KEEP_IN_MEMORY;
}

bool CGUIDialogWebOSD::OnAction(const CAction& action)
{
  if (action.GetID() == ACTION_SHOW_OSD)
  {
    Close();
    return true;
  }

  return CGUIDialog::OnAction(action);
}

bool CGUIDialogWebOSD::OnMessage(CGUIMessage& message)
{
  if (message.GetMessage() != GUI_MSG_CLICKED)
    return CGUIDialog::OnMessage(message);

  const auto manager = Manager();
  switch (message.GetSenderId())
  {
    case CTL_BUTTON_BACK:
      manager->Navigate(m_shown.profile, WebNavigation::BACK);
      break;
    case CTL_BUTTON_FORWARD:
      manager->Navigate(m_shown.profile, WebNavigation::FORWARD);
      break;
    case CTL_BUTTON_RELOAD:
      manager->Navigate(m_shown.profile, WebNavigation::RELOAD);
      break;
    case CTL_BUTTON_CLOSE:
      CServiceBroker::GetAppMessenger()->PostMsg(TMSG_MEDIA_STOP);
      break;
    default:
      return CGUIDialog::OnMessage(message);
  }

  Close();
  return true;
}

void CGUIDialogWebOSD::FrameMove()
{
  UpdateControls(false);
  CGUIDialog::FrameMove();
}

void CGUIDialogWebOSD::OnInitWindow()
{
  UpdateControls(true);
  CGUIDialog::OnInitWindow();
  if (m_shown.canGoBack)
    SET_CONTROL_FOCUS(CTL_BUTTON_BACK, 0);
}

void CGUIDialogWebOSD::UpdateControls(bool force)
{
  CWebClientManager::ViewState shown;
  if (const auto manager = Manager())
    manager->GetShownView(shown);

  if (force || shown.title != m_shown.title)
    SET_CONTROL_LABEL(CTL_LABEL_TITLE, shown.title);
  if (force || shown.url != m_shown.url)
    SET_CONTROL_LABEL(CTL_LABEL_URL, shown.url);
  if (force || shown.canGoBack != m_shown.canGoBack)
    CONTROL_ENABLE_ON_CONDITION(CTL_BUTTON_BACK, shown.canGoBack);
  if (force || shown.canGoForward != m_shown.canGoForward)
    CONTROL_ENABLE_ON_CONDITION(CTL_BUTTON_FORWARD, shown.canGoForward);
  m_shown = std::move(shown);
}
