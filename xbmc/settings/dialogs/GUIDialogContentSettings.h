#pragma once

/*
 *      Copyright (C) 2005-2014 Team XBMC
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

#include <map>

#include "addons/Addon.h"
#include "addons/Scraper.h"
#include "settings/dialogs/GUIDialogSettingsManualBase.h"

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
  void SetFocus(const std::string &settingid);

  /*!
  * @brief The currently selected content type
  */
  CONTENT_TYPE m_content;
  /*!
  * @brief The selected content type at dialog creation
  */
  CONTENT_TYPE m_originalContent;
  /*!
  * @brief The currently selected scraper
  */
  ADDON::ScraperPtr m_scraper;

  bool m_showScanSettings;
  bool m_scanRecursive;
  bool m_useDirectoryNames;
  bool m_containsSingleItem;
  bool m_exclude;
  bool m_noUpdating;
};
