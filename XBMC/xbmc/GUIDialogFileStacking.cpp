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
#include "GUIDialogFileStacking.h"

#define STACK_LIST 450

CGUIDialogFileStacking::CGUIDialogFileStacking(void)
    : CGUIDialog(WINDOW_DIALOG_FILESTACKING, "DialogFileStacking.xml")
{
  m_iSelectedFile = -1;
  m_iNumberOfFiles = 0;
}

CGUIDialogFileStacking::~CGUIDialogFileStacking(void)
{}

bool CGUIDialogFileStacking::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_DEINIT:
    CGUIDialog::OnMessage(message);
    m_stackItems.Clear();
    return true;
  case GUI_MSG_WINDOW_INIT:
    {
      CGUIDialog::OnMessage(message);
      m_iSelectedFile = -1;
#ifdef PRE_SKIN_VERSION_2_1_COMPATIBILITY
      m_iFrames = 0;

      // enable the CD's
      for (int i = 1; i <= m_iNumberOfFiles; ++i)
      {
        CONTROL_ENABLE(i);
        SET_CONTROL_VISIBLE(i);
      }

      // disable CD's we dont use
      for (int i = m_iNumberOfFiles + 1; i <= 40; ++i)
      {
        SET_CONTROL_HIDDEN(i);
        CONTROL_DISABLE(i);
      }
#endif
      if (GetControl(STACK_LIST))
      { // have the new stack list instead - fill it up
        CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), STACK_LIST);
        OnMessage(msg);
        for (int i = 0; i < m_iNumberOfFiles; i++)
        {
          CStdString label;
          label.Format("Part %i", i+1);
          CFileItem *item = new CFileItem(label);
          m_stackItems.Add(item);
          CGUIMessage msg(GUI_MSG_LABEL_ADD, GetID(), STACK_LIST, 0, 0, item);
          OnMessage(msg);
        }
      }
      return true;
    }
    break;

  case GUI_MSG_CLICKED:
    {
#ifdef PRE_SKIN_VERSION_2_1_COMPATIBILITY
      if (message.GetSenderId() != STACK_LIST)
        m_iSelectedFile = message.GetSenderId();
      else if (message.GetParam1() == ACTION_SELECT_ITEM)
      {
        // grab the selected item
        CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), STACK_LIST);
        OnMessage(msg);
        m_iSelectedFile = msg.GetParam1() + 1;
      }
#endif
      Close();
      return true;
    }
    break;
  }
  return CGUIDialog::OnMessage(message);
}

int CGUIDialogFileStacking::GetSelectedFile() const
{
  return m_iSelectedFile;
}

void CGUIDialogFileStacking::SetNumberOfFiles(int iFiles)
{
#ifdef PRE_SKIN_VERSION_2_1_COMPATIBILITY
  if (!GetControl(STACK_LIST))  // maximum number of files is 20 in the old system
    m_iNumberOfFiles = std::min(iFiles,20);
  else
    m_iNumberOfFiles = iFiles;
#endif
}

void CGUIDialogFileStacking::Render()
{
#ifdef PRE_SKIN_VERSION_2_1_COMPATIBILITY
  if (m_iFrames <= 25)
  {
    // slide in...
    int dwScreenWidth = g_settings.m_ResInfo[m_coordsRes].iWidth;
    for (int i = 1; i <= m_iNumberOfFiles; ++i)
    {
      CGUIControl* pControl = (CGUIControl*)GetControl(i);
      if (pControl)
      {
        DWORD dwEndPos = dwScreenWidth - ((m_iNumberOfFiles - i) * 32) - 140;
        DWORD dwStartPos = dwScreenWidth;
        float fStep = (float)(dwStartPos - dwEndPos);
        fStep /= 25.0f;
        fStep *= m_iFrames;
        pControl->SetPosition((float)dwStartPos - fStep, pControl->GetYPosition() );
      }
    }
    m_iFrames++;
  }
#endif
  CGUIDialog::Render();
}
