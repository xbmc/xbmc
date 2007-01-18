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
  case GUI_MSG_WINDOW_INIT:
    {
      CGUIDialog::OnMessage(message);
      m_iSelectedFile = -1;
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

      return true;
    }
    break;

  case GUI_MSG_CLICKED:
    {
      m_iSelectedFile = message.GetSenderId();
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
  // maximum number of files is 20 (as we have 20 cd images)
  m_iNumberOfFiles = min(iFiles,20);
}

void CGUIDialogFileStacking::Render()
{
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
  CGUIDialog::Render();
}