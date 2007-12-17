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
#include "GUIDialogAccessPoints.h"
#include "NetworkLinux.h"
#include "Application.h"

#define CONTROL_ACCESS_POINTS 3

CGUIDialogAccessPoints::CGUIDialogAccessPoints(void)
    : CGUIDialog(WINDOW_DIALOG_ACCESS_POINTS, "DialogAccessPoints.xml")
{
}

CGUIDialogAccessPoints::~CGUIDialogAccessPoints(void)
{}

bool CGUIDialogAccessPoints::OnAction(const CAction &action)
{
  if (action.wID == ACTION_SELECT_ITEM)
  {
    CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_ACCESS_POINTS, 0, 0, NULL);
    OnMessage(msg);
    int iItem = msg.GetParam1();
    
    if (iItem == (int) m_aps.size())
    {
       m_selectedAPEssId = "";
       if (CGUIDialogKeyboard::ShowAndGetInput(m_selectedAPEssId, g_localizeStrings.Get(789), false))
       {
         m_selectedAPEncMode = m_aps[iItem].getEncryptionMode();
         m_wasItemSelected = true;
         Close();
         return true;
       }
    }
    else
    {
       m_selectedAPEssId = m_aps[iItem].getEssId();
       m_selectedAPEncMode = m_aps[iItem].getEncryptionMode();
       m_wasItemSelected = true;
       Close();       
       return true;
    } 
  }
  
  return CGUIDialog::OnAction(action);
}

void CGUIDialogAccessPoints::OnInitWindow()
{
  m_wasItemSelected = false;
  
  CGUIDialog::OnInitWindow();

  CGUIMessage msgReset(GUI_MSG_LABEL_RESET, GetID(), CONTROL_ACCESS_POINTS);
  OnMessage(msgReset);

  CStdString ifaceName(m_interfaceName);
  CNetworkInterface* iface = g_application.getNetwork().GetInterfaceByName(ifaceName);  
  m_aps = iface->GetAccessPoints();
  
  for (int i = 0; i < (int) m_aps.size(); i++)
  {
      CFileItem *item = new CFileItem(m_aps[i].getEssId());
      
      int q = m_aps[i].getQuality();
      if (q <= 20) item->SetThumbnailImage("ap-signal1.png");
      else if (q <= 40) item->SetThumbnailImage("ap-signal2.png");
      else if (q <= 60) item->SetThumbnailImage("ap-signal3.png");
      else if (q <= 80) item->SetThumbnailImage("ap-signal4.png");
      else if (q <= 100) item->SetThumbnailImage("ap-signal5.png");

      if (m_aps[i].getEncryptionMode() != ENC_NONE)      
         item->SetIconImage("ap-lock.png");
         
      CGUIMessage msg(GUI_MSG_LABEL_ADD, GetID(), CONTROL_ACCESS_POINTS, 0, 0, (void*)item);
      OnMessage(msg);
  }
  
  CFileItem *item = new CFileItem(g_localizeStrings.Get(1047));
  CGUIMessage msg(GUI_MSG_LABEL_ADD, GetID(), CONTROL_ACCESS_POINTS, 0, 0, (void*)item);
  OnMessage(msg);  
}

void CGUIDialogAccessPoints::SetInterfaceName(CStdString interfaceName)
{
  m_interfaceName = interfaceName;
}

CStdString CGUIDialogAccessPoints::GetSelectedAccessPointEssId()
{
  return m_selectedAPEssId;
}

EncMode CGUIDialogAccessPoints::GetSelectedAccessPointEncMode()
{
  return m_selectedAPEncMode;
}

bool CGUIDialogAccessPoints::WasItemSelected()
{
  return m_wasItemSelected;
}
