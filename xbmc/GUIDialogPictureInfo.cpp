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
#include "GUIDialogPictureInfo.h"
#include "utils/GUIInfoManager.h"

#define CONTROL_PICTURE_INFO 5

#define SLIDE_STRING_BASE 21800 - SLIDE_INFO_START

CGUIDialogPictureInfo::CGUIDialogPictureInfo(void)
    : CGUIDialog(WINDOW_DIALOG_PICTURE_INFO, "DialogPictureInfo.xml")
{
}

CGUIDialogPictureInfo::~CGUIDialogPictureInfo(void)
{
}

void CGUIDialogPictureInfo::SetPicture(const CStdString &picture)
{
  g_infoManager.SetCurrentSlide(picture);
}

void CGUIDialogPictureInfo::OnInitWindow()
{
  CGUIDialog::OnInitWindow();
  UpdatePictureInfo();
}

void CGUIDialogPictureInfo::Render()
{
  CStdString currentPicture = g_infoManager.GetLabel(SLIDE_FILE_NAME) + g_infoManager.GetLabel(SLIDE_FILE_PATH);
  if (currentPicture != m_currentPicture)
  {
    UpdatePictureInfo();
    m_currentPicture = currentPicture;
  }
  CGUIDialog::Render();
}

void CGUIDialogPictureInfo::UpdatePictureInfo()
{
  // add stuff from the current slide to the list
  CGUIMessage msgReset(GUI_MSG_LABEL_RESET, GetID(), CONTROL_PICTURE_INFO);
  OnMessage(msgReset);
  m_pictureInfo.Clear();
  for (int info = SLIDE_INFO_START; info <= SLIDE_INFO_END; ++info)
  {
    CStdString picInfo = g_infoManager.GetLabel(info);
    if (!picInfo.IsEmpty())
    {
      CFileItem *item = new CFileItem(g_localizeStrings.Get(SLIDE_STRING_BASE + info));
      item->SetLabel2(picInfo);
      m_pictureInfo.Add(item);
      CGUIMessage msg(GUI_MSG_LABEL_ADD, GetID(), CONTROL_PICTURE_INFO, 0, 0, (void*)item);
      OnMessage(msg);
    }
  }
}

void CGUIDialogPictureInfo::OnDeinitWindow(int nextWindowID)
{
  CGUIDialog::OnDeinitWindow(nextWindowID);
  CGUIMessage msgReset(GUI_MSG_LABEL_RESET, GetID(), CONTROL_PICTURE_INFO);
  OnMessage(msgReset);
  m_pictureInfo.Clear();
  m_currentPicture.Empty();
}