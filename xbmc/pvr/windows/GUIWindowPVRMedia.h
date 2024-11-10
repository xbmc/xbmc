/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "dialogs/GUIDialogContextMenu.h"
#include "pvr/settings/PVRSettings.h"
#include "pvr/windows/GUIWindowPVRBase.h"
#include "video/VideoDatabase.h"
#include "video/VideoThumbLoader.h"

#include <string>

class CFileItem;

namespace PVR
{
class CGUIWindowPVRMediaBase : public CGUIWindowPVRBase
{
public:
  CGUIWindowPVRMediaBase(bool bRadio, int id, const std::string& xmlFile);
  ~CGUIWindowPVRMediaBase() override;

  void OnWindowLoaded() override;
  bool OnMessage(CGUIMessage& message) override;
  bool OnAction(const CAction& action) override;
  void GetContextButtons(int itemNumber, CContextButtons& buttons) override;
  bool OnPopupMenu(int iItem) override;
  bool OnContextButton(int itemNumber, CONTEXT_BUTTON button) override;
  bool Update(const std::string& strDirectory, bool updateFilterPath = true) override;
  void UpdateButtons() override;

protected:
  std::string GetDirectoryPath() override;
  void OnPrepareFileItems(CFileItemList& items) override;
  bool GetFilteredItems(const std::string& filter, CFileItemList& items) override;

private:
  CVideoThumbLoader m_thumbLoader;
  CVideoDatabase m_database;
  CPVRSettings m_settings;
};

class CGUIWindowPVRTVMedia : public CGUIWindowPVRMediaBase
{
public:
  CGUIWindowPVRTVMedia() : CGUIWindowPVRMediaBase(false, WINDOW_TV_MEDIA, "MyPVRMedia.xml") {}
};

class CGUIWindowPVRRadioMedia : public CGUIWindowPVRMediaBase
{
public:
  CGUIWindowPVRRadioMedia() : CGUIWindowPVRMediaBase(true, WINDOW_RADIO_MEDIA, "MyPVRMedia.xml") {}
};
} // namespace PVR
