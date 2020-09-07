/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/Scraper.h"
#include "settings/dialogs/GUIDialogSettingsManualBase.h"

#include <map>

namespace VIDEO
{
  struct SScanSettings;
}
class CFileItemList;

class CGUIDialogContentSettings : public CGUIDialogSettingsManualBase
{
public:
  CGUIDialogContentSettings();

  // specialization of CGUIWindow
  bool HasListItems() const override { return true; };

  CONTENT_TYPE GetContent() const { return m_content; }
  void SetContent(CONTENT_TYPE content);
  void ResetContent();

  const ADDON::ScraperPtr& GetScraper() const { return m_scraper; }
  void SetScraper(ADDON::ScraperPtr scraper) { m_scraper = scraper; }

  void SetScanSettings(const VIDEO::SScanSettings &scanSettings);
  bool GetScanRecursive() const { return m_scanRecursive; }
  bool GetUseDirectoryNames() const { return m_useDirectoryNames; }
  bool GetContainsSingleItem() const { return m_containsSingleItem; }
  bool GetExclude() const { return m_exclude; }
  bool GetNoUpdating() const { return m_noUpdating; }
  bool GetUseAllExternalAudio() const { return m_allExternalAudio; }

  static bool Show(ADDON::ScraperPtr& scraper, CONTENT_TYPE content = CONTENT_NONE);
  static bool Show(ADDON::ScraperPtr& scraper, VIDEO::SScanSettings& settings, CONTENT_TYPE content = CONTENT_NONE);

protected:
  // specializations of CGUIWindow
  void OnInitWindow() override;

  // implementations of ISettingCallback
  void OnSettingChanged(std::shared_ptr<const CSetting> setting) override;
  void OnSettingAction(std::shared_ptr<const CSetting> setting) override;

  // specialization of CGUIDialogSettingsBase
  bool AllowResettingSettings() const override { return false; }
  void Save() override;
  void SetupView() override;

  // specialization of CGUIDialogSettingsManualBase
  void InitializeSettings() override;

private:
  void SetLabel2(const std::string &settingid, const std::string &label);
  void ToggleState(const std::string &settingid, bool enabled);
  using CGUIDialogSettingsManualBase::SetFocus;
  void SetFocusToSetting(const std::string& settingid);

  /*!
  * @brief The currently selected content type
  */
  CONTENT_TYPE m_content = CONTENT_NONE;
  /*!
  * @brief The selected content type at dialog creation
  */
  CONTENT_TYPE m_originalContent = CONTENT_NONE;
  /*!
  * @brief The currently selected scraper
  */
  ADDON::ScraperPtr m_scraper;

  bool m_showScanSettings = false;
  bool m_scanRecursive = false;
  bool m_useDirectoryNames = false;
  bool m_containsSingleItem = false;
  bool m_exclude = false;
  bool m_noUpdating = false;
  bool m_allExternalAudio = false;
};
