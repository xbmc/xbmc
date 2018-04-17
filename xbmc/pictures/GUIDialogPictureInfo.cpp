/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://kodi.tv
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

#include "GUIDialogPictureInfo.h"
#include "GUIInfoManager.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "FileItem.h"
#include "ServiceBroker.h"
#include "input/Key.h"
#include "guilib/LocalizeStrings.h"
#include "PictureInfoTag.h"
#include "guilib/guiinfo/GUIInfoLabels.h"

#define CONTROL_PICTURE_INFO 5

#define SLIDESHOW_STRING_BASE 21800 - SLIDESHOW_LABELS_START

CGUIDialogPictureInfo::CGUIDialogPictureInfo(void)
    : CGUIDialog(WINDOW_DIALOG_PICTURE_INFO, "DialogPictureInfo.xml")
{
  m_pictureInfo = new CFileItemList;
  m_loadType = KEEP_IN_MEMORY;
}

CGUIDialogPictureInfo::~CGUIDialogPictureInfo(void)
{
  delete m_pictureInfo;
}

void CGUIDialogPictureInfo::SetPicture(CFileItem *item)
{
  CServiceBroker::GetGUI()->GetInfoManager().GetInfoProviders().GetPicturesInfoProvider().SetCurrentSlide(item);
}

void CGUIDialogPictureInfo::OnInitWindow()
{
  UpdatePictureInfo();
  CGUIDialog::OnInitWindow();
}

bool CGUIDialogPictureInfo::OnAction(const CAction& action)
{
  switch (action.GetID())
  {
    // if we're running from slideshow mode, drop the "next picture" and "previous picture" actions through.
    case ACTION_NEXT_PICTURE:
    case ACTION_PREV_PICTURE:
    case ACTION_PLAYER_PLAY:
    case ACTION_PAUSE:
      if (CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == WINDOW_SLIDESHOW)
      {
        CGUIWindow* pWindow = CServiceBroker::GetGUI()->GetWindowManager().GetWindow(WINDOW_SLIDESHOW);
        return pWindow->OnAction(action);
      }
      break;

    case ACTION_SHOW_INFO:
      Close();
      return true;
  };
  return CGUIDialog::OnAction(action);
}

void CGUIDialogPictureInfo::FrameMove()
{
  const CFileItem* item = CServiceBroker::GetGUI()->GetInfoManager().GetInfoProviders().GetPicturesInfoProvider().GetCurrentSlide();
  if (item && item->GetPath() != m_currentPicture)
  {
    UpdatePictureInfo();
    m_currentPicture = item->GetPath();
  }
  CGUIDialog::FrameMove();
}

void CGUIDialogPictureInfo::UpdatePictureInfo()
{
  // add stuff from the current slide to the list
  CGUIMessage msgReset(GUI_MSG_LABEL_RESET, GetID(), CONTROL_PICTURE_INFO);
  OnMessage(msgReset);
  m_pictureInfo->Clear();
  for (int info = SLIDESHOW_LABELS_START; info <= SLIDESHOW_LABELS_END; ++info)
  {
    // we only want to add SLIDESHOW_EXIF_DATE_TIME
    // so we skip the other date formats
    if (info == SLIDESHOW_EXIF_DATE || info == SLIDESHOW_EXIF_LONG_DATE || info == SLIDESHOW_EXIF_LONG_DATE_TIME )
      continue;

    std::string picInfo = CServiceBroker::GetGUI()->GetInfoManager().GetLabel(info);
    if (!picInfo.empty())
    {
      CFileItemPtr item(new CFileItem(g_localizeStrings.Get(SLIDESHOW_STRING_BASE + info)));
      item->SetLabel2(picInfo);
      m_pictureInfo->Add(item);
    }
  }
  CGUIMessage msg(GUI_MSG_LABEL_BIND, GetID(), CONTROL_PICTURE_INFO, 0, 0, m_pictureInfo);
  OnMessage(msg);
}

void CGUIDialogPictureInfo::OnDeinitWindow(int nextWindowID)
{
  CGUIDialog::OnDeinitWindow(nextWindowID);
  CGUIMessage msgReset(GUI_MSG_LABEL_RESET, GetID(), CONTROL_PICTURE_INFO);
  OnMessage(msgReset);
  m_pictureInfo->Clear();
  m_currentPicture.clear();
}
