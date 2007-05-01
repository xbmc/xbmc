/*
 *      Copyright (C) 2005-2007 Team XboxMediaCenter
 *      http://www.xboxmediacenter.com
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "GUIDialogHost.h"
#include "GUISpinControl.h"
#include "GUILabelControl.h"
#include "GUIButtonControl.h"
#include "GUIRadioButtonControl.h"

#define CTL_CHECKMARK_HOSTING_TYPE 3101
#define CTL_LABEL_PLAYER_LIMIT  3102
#define CTL_SPIN_PLAYER_LIMIT  3103
#define CTL_LABEL_PASSWORD   3104
#define CTL_BUTTON_PASSWORD   3105
#define CTL_LABEL_DESCRIPTION  3106
#define CTL_BUTTON_DESCRIPTION  3107
#define CTL_BUTTON_OK    3108
#define CTL_BUTTON_CANCEL   3109

#define KAI_PRIVATE_ARENA_LIMIT  32

CGUIDialogHost::CGUIDialogHost(void)
: CGUIDialog(WINDOW_DIALOG_HOST, "DialogHost.xml")
{
  m_bPrivate = false;
}

CGUIDialogHost::~CGUIDialogHost(void)
{}

void CGUIDialogHost::OnInitWindow()
{
  CGUISpinControl& spin_control = *((CGUISpinControl*)GetControl(CTL_SPIN_PLAYER_LIMIT));

  spin_control.Clear();
  spin_control.SetRange(1, 16);

  for (int i = 0; i < KAI_PRIVATE_ARENA_LIMIT; i++)
  {
    CStdString strItem;
    strItem.Format("%d/%d", i + 1, KAI_PRIVATE_ARENA_LIMIT);
    spin_control.AddLabel(strItem, i);
  }

  m_strPassword = g_stSettings.szOnlineArenaPassword;
  m_strDescription = g_stSettings.szOnlineArenaDescription;
  m_nPlayerLimit = 8;

  spin_control.SetValue(m_nPlayerLimit - 1);

  Update();
  CGUIWindow::OnInitWindow();
}

bool CGUIDialogHost::OnMessage(CGUIMessage& message)
{
  CGUIDialog::OnMessage(message);

  switch ( message.GetMessage() )
  {
  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();

      switch (iControl)
      {
      case CTL_CHECKMARK_HOSTING_TYPE:
        {
          CGUIRadioButtonControl& radiobutton = *(CGUIRadioButtonControl*)GetControl(CTL_CHECKMARK_HOSTING_TYPE);
          m_bPrivate = radiobutton.IsSelected();
          Update();
          break;
        }

      case CTL_SPIN_PLAYER_LIMIT:
        {
          CGUISpinControl& spin_control = *((CGUISpinControl*)GetControl(CTL_SPIN_PLAYER_LIMIT));
          m_nPlayerLimit = spin_control.GetValue() + 1;
          break;
        }

      case CTL_BUTTON_PASSWORD:
        {
          CStdString strCaption = g_localizeStrings.Get(15047); // Enter a password for your arena.
          if (!CGUIDialogKeyboard::ShowAndGetInput(m_strPassword, strCaption, true))
            break;

          Update();

          if (m_strPassword.length() < 32)
          {
            strcpy(g_stSettings.szOnlineArenaPassword, m_strPassword.c_str());
            g_settings.Save();
          }

          break;
        }

      case CTL_BUTTON_DESCRIPTION:
        {
          CStdString strCaption = g_localizeStrings.Get(15048); // Enter a description for your arena.
          if (!CGUIDialogKeyboard::ShowAndGetInput(m_strDescription, strCaption, false))
            break;

          Update();

          if (m_strDescription.length() < 64)
          {
            strcpy(g_stSettings.szOnlineArenaDescription, m_strDescription.c_str());
            g_settings.Save();
          }

          break;
        }

      case CTL_BUTTON_OK:
        {
          m_bOK = true;
          Close();
          break;
        }

      case CTL_BUTTON_CANCEL:
        {
          m_bOK = false;
          Close();
          break;
        }
      }
    }
    break;
  }

  return true;
}

void CGUIDialogHost::Update()
{
  CONTROL_ENABLE_ON_CONDITION(CTL_SPIN_PLAYER_LIMIT, m_bPrivate);
  CONTROL_ENABLE_ON_CONDITION(CTL_BUTTON_PASSWORD, m_bPrivate);
  CONTROL_ENABLE_ON_CONDITION(CTL_BUTTON_DESCRIPTION, m_bPrivate);

  SET_CONTROL_LABEL(CTL_BUTTON_DESCRIPTION, m_strDescription);

  unsigned int len = m_strPassword.length();
  CStdString strPassword;
  for (unsigned int i = 0; i < len; i++) strPassword += '*';
  SET_CONTROL_LABEL(CTL_BUTTON_PASSWORD, strPassword);
}

bool CGUIDialogHost::IsOK() const
{
  return m_bOK;
}

bool CGUIDialogHost::IsPrivate() const
{
  return m_bPrivate;
}

void CGUIDialogHost::GetConfiguration(CStdString& aPassword, CStdString& aDescription, INT& aPlayerLimit)
{
  aPassword = m_strPassword;
  aDescription = m_strDescription;
  aPlayerLimit = m_nPlayerLimit;
}

