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
#include "GUIDialogInvite.h"
#include "GUISpinControl.h"
#include "GUILabelControl.h"
#include "GUIButtonControl.h"
#include "utils/KaiClient.h"
#include "ArenaItem.h"


#define CTL_LABEL_CAPTION 1
#define CTL_LABEL_SCHEDULE 2
#define CTL_LABEL_GAME  3
#define CTL_LABEL_DATE  4
#define CTL_LABEL_TIME  5
#define CTL_LABEL_CONFIRM 6
#define CTL_LABEL_CONFIRM2 7
#define CTL_BUTTON_SEND  10
#define CTL_BUTTON_CANCEL 11
#define CTL_BUTTON_MESSAGE  20
#define CTL_SPIN_GAMES  30


CGUIDialogInvite::CGUIDialogInvite(void)
: CGUIDialog(WINDOW_DIALOG_INVITE, "DialogInvite.xml")
{
  m_bConfirmed = false;
}

CGUIDialogInvite::~CGUIDialogInvite(void)
{}

void CGUIDialogInvite::OnInitWindow()
{
  CGUISpinControl& spin_control = *((CGUISpinControl*)GetControl(CTL_SPIN_GAMES));

  // Initialise Spin Control
  if (m_pGames)
  {
    spin_control.Clear();

    CStdString strCurrentVector = g_localizeStrings.Get(15046); // Custom Arena
    spin_control.AddLabel( strCurrentVector, 0);

    CGUIList::GUILISTITEMS& list = m_pGames->Lock();

    for (int i = 0; i < (int) list.size(); ++i)
    {
      spin_control.AddLabel(list[i]->GetName(), i + 1);
    }

    m_pGames->Release();

    spin_control.SetValue(0);
  }
  CGUIWindow::OnInitWindow();
}

bool CGUIDialogInvite::OnMessage(CGUIMessage& message)
{
  CGUIDialog::OnMessage(message);

  switch ( message.GetMessage() )
  {
  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();

      switch (iControl)
      {
      case CTL_BUTTON_MESSAGE:
        {
          CStdString strHeading = g_localizeStrings.Get(15045); // Enter your personal message.
          if (!m_bMessage)
          {
            CGUISpinControl& spin_control = *((CGUISpinControl*)GetControl(CTL_SPIN_GAMES));
            INT nIndex = spin_control.GetValue();
            if (nIndex > 0)
            {
              CStdString strVector;
              CStdString strGame;
              CGUIList::GUILISTITEMS& list = m_pGames->Lock();
              strVector = ((CArenaItem*)list[nIndex - 1])->m_strVector;
              CArenaItem::GetTier(CArenaItem::Game, strVector, strGame);
              m_pGames->Release();
              // Do not localize this message, it may be send to a recipient
              // with a users "foreign language", if he uses xbmc it will be translated
              // to his language
              m_strMessage.Format("You're invited to play %s.", strGame);
            }
            else
            {
              m_strMessage = "";
            }
          }
          if (CGUIDialogKeyboard::ShowAndGetInput(m_strMessage, strHeading, false))
          {
            CGUIButtonControl& button = *((CGUIButtonControl*)GetControl(CTL_BUTTON_MESSAGE));
            button.SetLabel(m_strMessage);
            m_bMessage = TRUE;
          }
          break;
        }

      case CTL_BUTTON_SEND:
        {
          m_bConfirmed = true;
          CGUISpinControl& spin_control = *((CGUISpinControl*)GetControl(CTL_SPIN_GAMES));
          m_iSelectedIndex = spin_control.GetValue();
          Close();
          break;
        }

      case CTL_BUTTON_CANCEL:
        {
          m_bConfirmed = false;
          Close();
          break;
        }
      }
    }
    break;
  }

  return true;
}


void CGUIDialogInvite::GetPersonalMessage(CStdString& aMessage)
{
  if (m_bMessage)
  {
    aMessage = m_strMessage;
  }
}

bool CGUIDialogInvite::GetSelectedVector(CStdString& aVector)
{
  if (m_iSelectedIndex >= 0)
  {
    if (m_iSelectedIndex == 0)
    {
      aVector = CKaiClient::GetInstance()->GetCurrentVector();
    }
    else
    {
      CGUIList::GUILISTITEMS& list = m_pGames->Lock();
      aVector = ((CArenaItem*)list[m_iSelectedIndex - 1])->m_strVector;
      m_pGames->Release();
    }
    return true;
  }

  return false;
}

void CGUIDialogInvite::SetGames(CGUIList* aGamesList)
{
  m_pGames = aGamesList;
  m_iSelectedIndex = -1;
  m_bMessage = false;
}

bool CGUIDialogInvite::IsConfirmed() const
{
  return m_bConfirmed;
}
