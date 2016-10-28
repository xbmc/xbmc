/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "GUIDialogAccessPoints.h"
#include "guilib/GUIKeyboardFactory.h"
#if defined(TARGET_ANDROID)
#include "android/NetworkAndroid.h"
#elif defined(TARGET_POSIX)
#include "linux/NetworkLinux.h"
#endif
#include "Application.h"
#include "FileItem.h"
#include "input/Key.h"
#include "guilib/LocalizeStrings.h"
#include "utils/Variant.h"

#define CONTROL_ACCESS_POINTS 3

CGUIDialogAccessPoints::CGUIDialogAccessPoints(void)
    : CGUIDialog(WINDOW_DIALOG_ACCESS_POINTS, "DialogAccessPoints.xml")
{
  m_accessPoints = new CFileItemList;
}

CGUIDialogAccessPoints::~CGUIDialogAccessPoints(void)
{
  delete m_accessPoints;
}

bool CGUIDialogAccessPoints::OnAction(const CAction &action)
{
  if (action.GetID() == ACTION_SELECT_ITEM)
  {
    CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_ACCESS_POINTS);
    OnMessage(msg);
    int iItem = msg.GetParam1();

    if (iItem == (int) m_aps.size())
    {
       m_selectedAPEssId = "";
       if (CGUIKeyboardFactory::ShowAndGetInput(m_selectedAPEssId, CVariant{g_localizeStrings.Get(789)}, false))
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

  m_accessPoints->Clear();

  std::string ifaceName(m_interfaceName);
  CNetworkInterface* iface = g_application.getNetwork().GetInterfaceByName(ifaceName);
  m_aps = iface->GetAccessPoints();

  for (int i = 0; i < (int) m_aps.size(); i++)
  {
      CFileItemPtr item(new CFileItem(m_aps[i].getEssId()));

      int q = m_aps[i].getQuality();
      if (q <= 20) item->SetArt("thumb", "ap-signal1.png");
      else if (q <= 40) item->SetArt("thumb", "ap-signal2.png");
      else if (q <= 60) item->SetArt("thumb", "ap-signal3.png");
      else if (q <= 80) item->SetArt("thumb", "ap-signal4.png");
      else if (q <= 100) item->SetArt("thumb", "ap-signal5.png");

      if (m_aps[i].getEncryptionMode() != ENC_NONE)
         item->SetIconImage("ap-lock.png");

      m_accessPoints->Add(item);
  }

  CFileItemPtr item(new CFileItem(g_localizeStrings.Get(1047)));
  m_accessPoints->Add(item);

  CGUIMessage msg(GUI_MSG_LABEL_BIND, GetID(), CONTROL_ACCESS_POINTS, 0, 0, m_accessPoints);
  OnMessage(msg);
}

void CGUIDialogAccessPoints::SetInterfaceName(std::string interfaceName)
{
  m_interfaceName = interfaceName;
}

std::string CGUIDialogAccessPoints::GetSelectedAccessPointEssId()
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
