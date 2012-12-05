#pragma once

/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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

#include "GUIDialogSettings.h"
#include "addons/Scraper.h"
#include "addons/AddonManager.h"
#include <vector>

namespace VIDEO
{
  struct SScanSettings;
}
class CFileItemList;

class CGUIDialogContentSettings : public CGUIDialogSettings
{
public:
  CGUIDialogContentSettings(void);
  virtual ~CGUIDialogContentSettings(void);
  virtual bool OnMessage(CGUIMessage& message);

  static bool Show(ADDON::ScraperPtr& scraper, CONTENT_TYPE musicContext = CONTENT_NONE);
  static bool Show(ADDON::ScraperPtr& scraper, VIDEO::SScanSettings& settings, CONTENT_TYPE musicContext = CONTENT_NONE);
  static bool ShowForDirectory(const CStdString& strDirectory, ADDON::ScraperPtr& scraper, VIDEO::SScanSettings& settings);
  virtual bool HasListItems() const { return true; };
  virtual CFileItemPtr GetCurrentListItem(int offset = 0);
protected:
  virtual void OnOkay();
  virtual void OnCancel();
  virtual void OnInitWindow();
  virtual void SetupPage();
  virtual void CreateSettings();
  void FillContentTypes();
  void FillContentTypes(const CONTENT_TYPE& content);
  void AddContentType(const CONTENT_TYPE& content);
  void FillListControl();
  virtual void OnSettingChanged(SettingInfo& setting);

  bool m_bNeedSave;

  bool m_bShowScanSettings;
  bool m_bScanRecursive;
  bool m_bUseDirNames;
  bool m_bSingleItem;
  bool m_bExclude;
  bool m_bNoUpdate;
  std::map<CONTENT_TYPE, ADDON::VECADDONS> m_scrapers;
  std::map<CONTENT_TYPE, ADDON::AddonPtr>  m_lastSelected;
  CFileItemList* m_vecItems;

  CStdString m_strContentType;
  ADDON::AddonPtr m_scraper;
  CStdString m_defaultScraper;
  CONTENT_TYPE m_content;
  CONTENT_TYPE m_origContent;
};
