/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogPlayEject.h"

#include "ServiceBroker.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "storage/MediaManager.h"
#include "utils/Variant.h"

#include <utility>

#define ID_BUTTON_PLAY      11
#define ID_BUTTON_EJECT     10

CGUIDialogPlayEject::CGUIDialogPlayEject()
    : CGUIDialogYesNo(WINDOW_DIALOG_PLAY_EJECT)
{
}

CGUIDialogPlayEject::~CGUIDialogPlayEject() = default;

bool CGUIDialogPlayEject::OnMessage(CGUIMessage& message)
{
  if (message.GetMessage() == GUI_MSG_CLICKED)
  {
    int iControl = message.GetSenderId();
    if (iControl == ID_BUTTON_PLAY)
    {
      if (CServiceBroker::GetMediaManager().IsDiscInDrive())
      {
        m_bConfirmed = true;
        Close();
      }

      return true;
    }
    if (iControl == ID_BUTTON_EJECT)
    {
      CServiceBroker::GetMediaManager().ToggleTray();
      return true;
    }
  }

  return CGUIDialogYesNo::OnMessage(message);
}

void CGUIDialogPlayEject::FrameMove()
{
  CONTROL_ENABLE_ON_CONDITION(ID_BUTTON_PLAY, CServiceBroker::GetMediaManager().IsDiscInDrive());

  CGUIDialogYesNo::FrameMove();
}

void CGUIDialogPlayEject::OnInitWindow()
{
  if (CServiceBroker::GetMediaManager().IsDiscInDrive())
  {
    m_defaultControl = ID_BUTTON_PLAY;
  }
  else
  {
    CONTROL_DISABLE(ID_BUTTON_PLAY);
    m_defaultControl = ID_BUTTON_EJECT;
  }

  CGUIDialogYesNo::OnInitWindow();
}

bool CGUIDialogPlayEject::ShowAndGetInput(const std::string& strLine1,
                                          const std::string& strLine2,
                                          unsigned int uiAutoCloseTime /* = 0 */)
{

  // Create the dialog
  CGUIDialogPlayEject * pDialog = (CGUIDialogPlayEject *)CServiceBroker::GetGUI()->GetWindowManager().
    GetWindow(WINDOW_DIALOG_PLAY_EJECT);
  if (!pDialog)
    return false;

  // Setup dialog parameters
  pDialog->SetHeading(CVariant{219});
  pDialog->SetLine(0, CVariant{429});
  pDialog->SetLine(1, CVariant{strLine1});
  pDialog->SetLine(2, CVariant{strLine2});
  pDialog->SetChoice(ID_BUTTON_PLAY - 10, 208);
  pDialog->SetChoice(ID_BUTTON_EJECT - 10, 13391);
  if (uiAutoCloseTime)
    pDialog->SetAutoClose(uiAutoCloseTime);

  // Display the dialog
  pDialog->Open();

  return pDialog->IsConfirmed();
}
