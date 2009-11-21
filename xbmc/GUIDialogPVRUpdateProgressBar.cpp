/*
 *      Copyright (C) 2005-2009 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "GUIDialogEpgScan.h"
#include "GUISliderControl.h"
#include "GUIProgressControl.h"
#include "utils/SingleLock.h"

#define CONTROL_CHANNELNAME       31
#define CONTROL_PROGRESS          32

CGUIDialogEpgScan::CGUIDialogEpgScan(void)
    : CGUIDialog(WINDOW_DIALOG_EPG_SCAN, "DialogEpgScan.xml")
{
  m_loadOnDemand = false;
}

CGUIDialogEpgScan::~CGUIDialogEpgScan(void)
{}

bool CGUIDialogEpgScan::OnAction(const CAction &action)
{
  return CGUIDialog::OnAction(action);
}

bool CGUIDialogEpgScan::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_INIT:
    {
      //resources are allocated in g_application
      CGUIDialog::OnMessage(message);

      m_strTitle.Empty();
      m_fPercentDone=-1.0f;

      UpdateState();
      return true;
    }
    break;

  case GUI_MSG_WINDOW_DEINIT:
    {
      //don't deinit, g_application handles it
      return CGUIDialog::OnMessage(message);
    }
    break;
  }
  return false; // don't process anything other than what we need!
}

void CGUIDialogEpgScan::Render()
{
  // and render the controls
  CGUIDialog::Render();
}

void CGUIDialogEpgScan::SetTitle(CStdString strTitle)
{
  CSingleLock lock (m_critical);

  m_strTitle = strTitle;
}

void CGUIDialogEpgScan::SetProgress(int currentItem, int itemCount)
{
  CSingleLock lock (m_critical);

  m_fPercentDone=(float)((currentItem*100)/itemCount);
  if (m_fPercentDone>100.0F) m_fPercentDone=100.0F;
}

void CGUIDialogEpgScan::UpdateState()
{
  CSingleLock lock (m_critical);

  SET_CONTROL_LABEL(CONTROL_CHANNELNAME, m_strTitle);

  if (m_fPercentDone>-1.0f)
  {
    SET_CONTROL_VISIBLE(CONTROL_PROGRESS);
    CGUIProgressControl* pProgressCtrl=(CGUIProgressControl*)GetControl(CONTROL_PROGRESS);
    if (pProgressCtrl) pProgressCtrl->SetPercentage(m_fPercentDone);
  }
}

