#pragma once

/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "GUIDialogSettings.h"
#include "ScraperSettings.h"

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
  virtual bool OnMessage(CGUIMessage &message);

  static bool Show(SScraperInfo& scraper, bool& bRunScan, int iLabel=-1);
  static bool Show(SScraperInfo& scraper, VIDEO::SScanSettings& settings, bool& bRunScan, int iLabel=-1);
  static bool ShowForDirectory(const CStdString& strDirectory, SScraperInfo& scraper, VIDEO::SScanSettings& settings, bool& bRunScan);
  virtual bool HasListItems() const { return true; };
  virtual CFileItemPtr GetCurrentListItem(int offset = 0);
protected:
  virtual void OnCancel();
  virtual void OnWindowLoaded();
  virtual void OnInitWindow();
  virtual void SetupPage();
  virtual void CreateSettings();
  void FillListControl();
  void OnSettingChanged(unsigned int setting);
  SScraperInfo FindDefault(const CStdString& strType, const CStdString& strDefault);

  bool m_bNeedSave;

  bool m_bRunScan;
  bool m_bScanRecursive;
  bool m_bUseDirNames;
  bool m_bSingleItem;
  bool m_bExclude;
  bool m_bUpdate;
  std::map<CStdString,std::vector<SScraperInfo> > m_scrapers; // key = content type
  CFileItemList* m_vecItems;

  SScraperInfo m_info;
  CScraperSettings m_scraperSettings; // needed so we have a basis
  CStdString m_strContentType; // used for artist/albums
};

