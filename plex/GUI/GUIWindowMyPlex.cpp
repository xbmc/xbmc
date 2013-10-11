//
//  GUIDialogMyPlexPin.cpp
//  Plex
//
//  Created by Tobias Hieta <tobias@plexapp.com> on 2012-12-14.
//  Copyright 2012 Plex Inc. All rights reserved.
//


#define ID_SPINNER        2
#define ID_DESC_TEXT      10081
#define ID_PIN_NR         10082
#define ID_USERNAME       10083
#define ID_PASSWORD       10084
#define ID_BUTTON_CANCEL  10085
#define ID_BUTTON_MANUAL  10086
#define ID_BUTTON_SUBMIT  10087
#define ID_ICON_SUCCESS   10088
#define ID_ICON_ERROR     10089

#include "GUIWindowMyPlex.h"
#include "guilib/GUILabelControl.h"
#include "guilib/GUIButtonControl.h"
#include "LocalizeStrings.h"
#include "Client/MyPlex/MyPlexManager.h"
#include "GUIWindowManager.h"
#include "guilib/GUITextBox.h"
#include "guilib/GUIEditControl.h"

#include "PlexApplication.h"

void CGUIWindowMyPlex::Setup()
{
  SET_CONTROL_HIDDEN(ID_ICON_ERROR);
  SET_CONTROL_HIDDEN(ID_ICON_SUCCESS);
  SET_CONTROL_HIDDEN(ID_SPINNER);

  SET_CONTROL_LABEL(ID_BUTTON_CANCEL, g_localizeStrings.Get(222));
  SET_CONTROL_LABEL(ID_BUTTON_SUBMIT, g_localizeStrings.Get(44004));

  ShowPinInput();
}

void CGUIWindowMyPlex::ShowManualInput()
{
  m_manual = true;

  /* make sure that we stop polling the pin updates */
  g_plexApplication.myPlexManager->StopPinLogin();

  SET_CONTROL_HIDDEN(ID_PIN_NR);
  SET_CONTROL_HIDDEN(ID_SPINNER);

  SET_CONTROL_VISIBLE(ID_USERNAME);
  SET_CONTROL_VISIBLE(ID_PASSWORD);
  SET_CONTROL_VISIBLE(ID_BUTTON_SUBMIT);

  CGUIEditControl *edit = (CGUIEditControl*)GetControl(ID_PASSWORD);
  if (edit)
    edit->SetInputType(CGUIEditControl::INPUT_TYPE_PASSWORD, 0);

  SET_CONTROL_LABEL(ID_DESC_TEXT, g_localizeStrings.Get(44013));
  SET_CONTROL_LABEL(ID_BUTTON_MANUAL, g_localizeStrings.Get(44002));
  SET_CONTROL_FOCUS(ID_USERNAME, 0);
}

void CGUIWindowMyPlex::ShowPinInput()
{
  m_manual = false;
  SET_CONTROL_HIDDEN(ID_USERNAME);
  SET_CONTROL_HIDDEN(ID_PASSWORD);
  SET_CONTROL_HIDDEN(ID_BUTTON_SUBMIT);

  SET_CONTROL_VISIBLE(ID_PIN_NR);

  SET_CONTROL_LABEL(ID_BUTTON_MANUAL, g_localizeStrings.Get(413));

  g_plexApplication.myPlexManager->StartPinLogin();
}

void CGUIWindowMyPlex::ShowSuccess()
{
  SET_CONTROL_HIDDEN(ID_USERNAME);
  SET_CONTROL_HIDDEN(ID_PASSWORD);
  SET_CONTROL_HIDDEN(ID_PIN_NR);
  SET_CONTROL_HIDDEN(ID_BUTTON_MANUAL);
  SET_CONTROL_HIDDEN(ID_BUTTON_SUBMIT);
  SET_CONTROL_HIDDEN(ID_ICON_ERROR);

  SET_CONTROL_VISIBLE(ID_ICON_SUCCESS);
  SET_CONTROL_VISIBLE(ID_BUTTON_CANCEL);

  SET_CONTROL_LABEL(ID_DESC_TEXT, g_localizeStrings.Get(44104));
  SET_CONTROL_LABEL(ID_BUTTON_CANCEL, g_localizeStrings.Get(186));
}

void CGUIWindowMyPlex::ShowFailure(int reason)
{
  SET_CONTROL_HIDDEN(ID_USERNAME);
  SET_CONTROL_HIDDEN(ID_PASSWORD);
  SET_CONTROL_HIDDEN(ID_PIN_NR);
  SET_CONTROL_HIDDEN(ID_BUTTON_MANUAL);
  SET_CONTROL_HIDDEN(ID_BUTTON_SUBMIT);
  SET_CONTROL_HIDDEN(ID_ICON_SUCCESS);

  SET_CONTROL_VISIBLE(ID_ICON_ERROR);
  SET_CONTROL_VISIBLE(ID_BUTTON_CANCEL);

  SET_CONTROL_LABEL(ID_BUTTON_CANCEL, g_localizeStrings.Get(186));
  SET_CONTROL_LABEL(ID_DESC_TEXT, g_localizeStrings.Get(reason));
}

void CGUIWindowMyPlex::HandleMyPlexState(int state, int errorCode)
{
  switch(state)
  {
    case CMyPlexManager::STATE_FETCH_PIN:
    {
      if (!m_manual)
      {
        SET_CONTROL_VISIBLE(ID_SPINNER);
        SET_CONTROL_LABEL(ID_DESC_TEXT, g_localizeStrings.Get(44102));
      }
      break;
    }

    case CMyPlexManager::STATE_WAIT_PIN:
    {
      if (!m_manual)
      {
        SET_CONTROL_HIDDEN(ID_SPINNER);
        SET_CONTROL_LABEL(ID_DESC_TEXT, g_localizeStrings.Get(44101) + " " + g_localizeStrings.Get(44103));
        const CMyPlexPinInfo pinInfo = g_plexApplication.myPlexManager->GetCurrentPinInfo();

        /* Spaces between the numbers to show it properly in the skin */
        std::string codeWithSpace = pinInfo.code;
        codeWithSpace.insert(1, " ");
        codeWithSpace.insert(3, " ");
        codeWithSpace.insert(5, " ");

        SET_CONTROL_LABEL(ID_PIN_NR, codeWithSpace);
      }
      break;
    }

    case CMyPlexManager::STATE_LOGGEDIN:
    {
      SET_CONTROL_HIDDEN(ID_SPINNER);
      ShowSuccess();
      break;
    }

    case CMyPlexManager::STATE_TRY_LOGIN:
    {
      SET_CONTROL_VISIBLE(ID_SPINNER);
      SET_CONTROL_LABEL(ID_DESC_TEXT, g_localizeStrings.Get(20186));
      break;
    }

    case CMyPlexManager::STATE_NOT_LOGGEDIN:
    {
      SET_CONTROL_HIDDEN(ID_SPINNER);
      ShowFailure(15206);
      break;
    }

  }
}

bool
CGUIWindowMyPlex::OnMessage(CGUIMessage &message)
{
  switch(message.GetMessage())
  {
    case GUI_MSG_WINDOW_INIT:
    {
      CGUIWindow::OnMessage(message);
      Setup();

      return true;
    }
    case GUI_MSG_CLICKED:
    {
      if (message.GetSenderId() == ID_BUTTON_MANUAL)
      {
        ToggleInput();
        return true;
      }
      else if (message.GetSenderId() == ID_BUTTON_SUBMIT)
      {
        CGUIEditControl* username = (CGUIEditControl*)GetControl(ID_USERNAME);
        if (!username)
          Close();

        CGUIEditControl* password = (CGUIEditControl*)GetControl(ID_PASSWORD);
        if (!password)
          Close();

        CStdString ustr = username->GetLabel2();
        CStdString pstr = password->GetLabel2();

        if (ustr.empty() || pstr.empty())
        {
          ShowFailure(15206);
          return true;
        }

        SET_CONTROL_VISIBLE(ID_SPINNER);
        g_plexApplication.myPlexManager->Login(ustr, pstr);

      }
      else if (message.GetSenderId() == ID_BUTTON_CANCEL)
        Close();


      break;
    }
    case GUI_MSG_MYPLEX_STATE_CHANGE:
    {
      HandleMyPlexState(message.GetParam1(), message.GetParam2());
      break;
    }
    default:
      break;
  }
  return CGUIWindow::OnMessage(message);
}
