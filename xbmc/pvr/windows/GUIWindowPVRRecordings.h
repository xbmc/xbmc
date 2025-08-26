/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "dialogs/GUIDialogContextMenu.h"
#include "pvr/windows/GUIWindowPVRBase.h"
#include "video/VideoDatabase.h"
#include "video/VideoThumbLoader.h"

#include <memory>
#include <string>

class CFileItem;

namespace PVR
{
class CPVRSettings;

class CGUIWindowPVRRecordingsBase : public CGUIWindowPVRBase
{
public:
  CGUIWindowPVRRecordingsBase(bool bRadio, int id, const std::string& xmlFile);
  ~CGUIWindowPVRRecordingsBase() override;

  void OnWindowLoaded() override;
  void OnInitWindow() override;
  void OnDeinitWindow(int nextWindowID) override;
  bool OnMessage(CGUIMessage& message) override;
  bool OnAction(const CAction& action) override;
  void GetContextButtons(int itemNumber, CContextButtons& buttons) override;
  bool OnPopupMenu(int iItem) override;
  bool OnContextButton(int itemNumber, CONTEXT_BUTTON button) override;
  bool Update(const std::string& strDirectory, bool updateFilterPath = true) override;
  void UpdateButtons() override;

protected:
  std::string GetRootPath() override;
  std::string GetDirectoryPath() override;
  void OnPrepareFileItems(CFileItemList& items) override;
  bool GetFilteredItems(const std::string& filter, CFileItemList& items) override;

private:
  bool OnContextButtonDeleteAll(CONTEXT_BUTTON button) const;

  bool m_bShowDeletedRecordings{false};
  bool m_forceUngrouped{false};
  CVideoThumbLoader m_thumbLoader;
  CVideoDatabase m_database;
  std::unique_ptr<CPVRSettings> m_settings;
};

class CGUIWindowPVRTVRecordings : public CGUIWindowPVRRecordingsBase
{
public:
  CGUIWindowPVRTVRecordings()
    : CGUIWindowPVRRecordingsBase(false, WINDOW_TV_RECORDINGS, "MyPVRRecordings.xml")
  {
  }
};

class CGUIWindowPVRRadioRecordings : public CGUIWindowPVRRecordingsBase
{
public:
  CGUIWindowPVRRadioRecordings()
    : CGUIWindowPVRRecordingsBase(true, WINDOW_RADIO_RECORDINGS, "MyPVRRecordings.xml")
  {
  }
};
} // namespace PVR
