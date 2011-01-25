/*
 *      Copyright (C) 2005-2010 Team XBMC
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

#include "GUIDialogPVRUpdateProgressBar.h"
#include "guilib/GUIProgressControl.h"
#include "guilib/GUISliderControl.h"
#include "threads/SingleLock.h"

#define CONTROL_LABELHEADER       30
#define CONTROL_LABELTITLE        31
#define CONTROL_PROGRESS          32

CGUIDialogPVRUpdateProgressBar::CGUIDialogPVRUpdateProgressBar(void)
  : CGUIDialog(WINDOW_DIALOG_EPG_SCAN, "DialogPVRUpdateProgressBar.xml")
{
  m_loadOnDemand = false;
}

bool CGUIDialogPVRUpdateProgressBar::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_WINDOW_INIT:
    {
      CGUIDialog::OnMessage(message);

      m_strTitle.Empty();
      m_strHeader.Empty();
      m_fPercentDone = -1.0f;

      UpdateState();
      return true;
    }
    break;
  }

  return CGUIDialog::OnMessage(message);
}

void CGUIDialogPVRUpdateProgressBar::Render()
{
  if (m_bRunning)
    UpdateState();

  CGUIDialog::Render();
}

void CGUIDialogPVRUpdateProgressBar::SetHeader(const CStdString& strHeader)
{
  CSingleLock lock (m_critical);

  m_strHeader = strHeader;
}

void CGUIDialogPVRUpdateProgressBar::SetTitle(const CStdString& strTitle)
{
  CSingleLock lock (m_critical);

  m_strTitle = strTitle;
}

void CGUIDialogPVRUpdateProgressBar::SetProgress(int currentItem, int itemCount)
{
  CSingleLock lock (m_critical);

  m_fPercentDone = (float)((currentItem*100)/itemCount);
  if (m_fPercentDone > 100.0F)
    m_fPercentDone = 100.0F;
}

void CGUIDialogPVRUpdateProgressBar::UpdateState()
{
  CSingleLock lock (m_critical);

  SET_CONTROL_LABEL(CONTROL_LABELHEADER, m_strHeader);
  SET_CONTROL_LABEL(CONTROL_LABELTITLE, m_strTitle);

  if (m_fPercentDone > -1.0f)
  {
    SET_CONTROL_VISIBLE(CONTROL_PROGRESS);
    CGUIProgressControl* pProgressCtrl=(CGUIProgressControl*)GetControl(CONTROL_PROGRESS);
    if (pProgressCtrl) pProgressCtrl->SetPercentage(m_fPercentDone);
  }
}

