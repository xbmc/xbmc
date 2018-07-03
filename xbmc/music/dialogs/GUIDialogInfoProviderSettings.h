/*
 *      Copyright (C) 2017 Team XBMC
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

#pragma once

#include <map>

#include "addons/Addon.h"
#include "addons/Scraper.h"
#include "settings/dialogs/GUIDialogSettingsManualBase.h"

class CFileItemList;

// Enumeration of what combination of items to apply the scraper settings
enum INFOPROVIDERAPPLYOPTIONS
{
  INFOPROVIDER_DEFAULT = 0x0000,
  INFOPROVIDER_ALLVIEW = 0x0001,
  INFOPROVIDER_THISITEM = 0x0002
};

class CGUIDialogInfoProviderSettings : public CGUIDialogSettingsManualBase
{
public:
  CGUIDialogInfoProviderSettings();

  // specialization of CGUIWindow
  bool HasListItems() const override { return true; };

  const ADDON::ScraperPtr& GetAlbumScraper() const { return m_albumscraper; }
  void SetAlbumScraper(ADDON::ScraperPtr scraper) { m_albumscraper = scraper; }
  const ADDON::ScraperPtr& GetArtistScraper() const { return m_artistscraper; }
  void SetArtistScraper(ADDON::ScraperPtr scraper) { m_artistscraper = scraper; }

  /*! \brief Show dialog to change information provider for either artists or albums (not both).
   Has a list to select how settings are to be applied - as system default, to just current item or to all the filtered items on the node.
   This does not save the settings itself, that is left to the caller
  \param scraper [in/out] the selected scraper addon and settings. Scraper content must be artists or albums.
  \return 0 settings apply as system default, 1 to all items on node, 2 to just the selected item or -1 if dialog cancelled or error occurs
  */
  static int Show(ADDON::ScraperPtr& scraper);

  /*! \brief Show dialog to change the music scraping settings including default information providers for both artists or albums.
  This saves the settings when the dialog is confirmed.
  \return true if the dialog is confirmed, false otherwise
  */
  static bool Show();

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
  void ResetDefaults();

  /*!
  * @brief The currently selected album scraper
  */
  ADDON::ScraperPtr m_albumscraper;
  /*!
  * @brief The currently selected artist scraper
  */
  ADDON::ScraperPtr m_artistscraper;

  std::string m_strArtistInfoPath;
  bool m_showSingleScraper = false;
  CONTENT_TYPE m_singleScraperType = CONTENT_NONE;
  bool m_fetchInfo;
  unsigned int m_applyToItems = INFOPROVIDER_THISITEM;
};
