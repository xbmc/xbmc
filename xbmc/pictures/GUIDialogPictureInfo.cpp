/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogPictureInfo.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "GUIInfoManager.h"
#include "ServiceBroker.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/guiinfo/GUIInfoLabels.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"

#include <array>

namespace
{
constexpr unsigned int CONTROL_PICTURE_INFO = 5;

constexpr std::array slideShowInfoTranslationList{std::pair{SLIDESHOW_FILE_NAME, 21800},
                                                  std::pair{SLIDESHOW_FILE_PATH, 21801},
                                                  std::pair{SLIDESHOW_FILE_SIZE, 21802},
                                                  std::pair{SLIDESHOW_FILE_DATE, 21803},
                                                  std::pair{SLIDESHOW_COLOUR, 21807},
                                                  std::pair{SLIDESHOW_PROCESS, 21808},
                                                  std::pair{SLIDESHOW_INDEX, 21804},
                                                  std::pair{SLIDESHOW_RESOLUTION, 21805},
                                                  std::pair{SLIDESHOW_COMMENT, 21806},
                                                  std::pair{SLIDESHOW_EXIF_DATE_TIME, 21820},
                                                  std::pair{SLIDESHOW_EXIF_DESCRIPTION, 21821},
                                                  std::pair{SLIDESHOW_EXIF_CAMERA_MAKE, 21822},
                                                  std::pair{SLIDESHOW_EXIF_CAMERA_MODEL, 21823},
                                                  std::pair{SLIDESHOW_EXIF_COMMENT, 21824},
                                                  std::pair{SLIDESHOW_EXIF_APERTURE, 21826},
                                                  std::pair{SLIDESHOW_EXIF_FOCAL_LENGTH, 21827},
                                                  std::pair{SLIDESHOW_EXIF_FOCUS_DIST, 21828},
                                                  std::pair{SLIDESHOW_EXIF_EXPOSURE, 21829},
                                                  std::pair{SLIDESHOW_EXIF_EXPOSURE_TIME, 21830},
                                                  std::pair{SLIDESHOW_EXIF_EXPOSURE_BIAS, 21831},
                                                  std::pair{SLIDESHOW_EXIF_EXPOSURE_MODE, 21832},
                                                  std::pair{SLIDESHOW_EXIF_FLASH_USED, 21833},
                                                  std::pair{SLIDESHOW_EXIF_WHITE_BALANCE, 21834},
                                                  std::pair{SLIDESHOW_EXIF_LIGHT_SOURCE, 21835},
                                                  std::pair{SLIDESHOW_EXIF_METERING_MODE, 21836},
                                                  std::pair{SLIDESHOW_EXIF_ISO_EQUIV, 21837},
                                                  std::pair{SLIDESHOW_EXIF_DIGITAL_ZOOM, 21838},
                                                  std::pair{SLIDESHOW_EXIF_CCD_WIDTH, 21839},
                                                  std::pair{SLIDESHOW_EXIF_GPS_LATITUDE, 21840},
                                                  std::pair{SLIDESHOW_EXIF_GPS_LONGITUDE, 21841},
                                                  std::pair{SLIDESHOW_EXIF_GPS_ALTITUDE, 21842},
                                                  std::pair{SLIDESHOW_EXIF_ORIENTATION, 21843},
                                                  std::pair{SLIDESHOW_EXIF_XPCOMMENT, 21844},
                                                  std::pair{SLIDESHOW_IPTC_SUBLOCATION, 21857},
                                                  std::pair{SLIDESHOW_IPTC_IMAGETYPE, 21858},
                                                  std::pair{SLIDESHOW_IPTC_TIMECREATED, 21859},
                                                  std::pair{SLIDESHOW_IPTC_SUP_CATEGORIES, 21860},
                                                  std::pair{SLIDESHOW_IPTC_KEYWORDS, 21861},
                                                  std::pair{SLIDESHOW_IPTC_CAPTION, 21862},
                                                  std::pair{SLIDESHOW_IPTC_AUTHOR, 21863},
                                                  std::pair{SLIDESHOW_IPTC_HEADLINE, 21864},
                                                  std::pair{SLIDESHOW_IPTC_SPEC_INSTR, 21865},
                                                  std::pair{SLIDESHOW_IPTC_CATEGORY, 21866},
                                                  std::pair{SLIDESHOW_IPTC_BYLINE, 21867},
                                                  std::pair{SLIDESHOW_IPTC_BYLINE_TITLE, 21868},
                                                  std::pair{SLIDESHOW_IPTC_CREDIT, 21869},
                                                  std::pair{SLIDESHOW_IPTC_SOURCE, 21870},
                                                  std::pair{SLIDESHOW_IPTC_COPYRIGHT_NOTICE, 21871},
                                                  std::pair{SLIDESHOW_IPTC_OBJECT_NAME, 21872},
                                                  std::pair{SLIDESHOW_IPTC_CITY, 21873},
                                                  std::pair{SLIDESHOW_IPTC_STATE, 21874},
                                                  std::pair{SLIDESHOW_IPTC_COUNTRY, 21875},
                                                  std::pair{SLIDESHOW_IPTC_TX_REFERENCE, 21876},
                                                  std::pair{SLIDESHOW_IPTC_DATE, 21877},
                                                  std::pair{SLIDESHOW_IPTC_URGENCY, 21878},
                                                  std::pair{SLIDESHOW_IPTC_COUNTRY_CODE, 21879},
                                                  std::pair{SLIDESHOW_IPTC_REF_SERVICE, 21880}};
} // namespace

CGUIDialogPictureInfo::CGUIDialogPictureInfo(void)
  : CGUIDialog(WINDOW_DIALOG_PICTURE_INFO, "DialogPictureInfo.xml"),
    m_pictureInfo{std::make_unique<CFileItemList>()}
{
  m_loadType = KEEP_IN_MEMORY;
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
  for (const auto& [info, code] : slideShowInfoTranslationList)
  {
    const std::string picInfo =
        CServiceBroker::GetGUI()->GetInfoManager().GetLabel(info, INFO::DEFAULT_CONTEXT);
    if (!picInfo.empty())
    {
      auto item{std::make_shared<CFileItem>(g_localizeStrings.Get(code))};
      item->SetLabel2(picInfo);
      m_pictureInfo->Add(item);
    }
  }
  CGUIMessage msg(GUI_MSG_LABEL_BIND, GetID(), CONTROL_PICTURE_INFO, 0, 0, m_pictureInfo.get());
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
