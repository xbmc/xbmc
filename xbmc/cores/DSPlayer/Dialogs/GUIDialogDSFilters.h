#pragma once

/*
 *      Copyright (C) 2005-2014 Team XBMC
 *      http://xbmc.org
 *
 *      Copyright (C) 2014-2015 Aracnoz
 *      http://github.com/aracnoz/xbmc 
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

#include "settings/dialogs/GUIDialogSettingsManualBase.h"
#include "utils/stdstring.h"

enum FilterType {
  EDITATTRFILTER,
  OSDGUID,
  SPINNERATTRFILTER,
  FILTERSYSTEM,
};

class DSFiltersList
{
public:
  DSFiltersList(FilterType type);

  CStdString strFilterAttr;
  CStdString strFilterName;
  CStdString strFilterValue;
  CStdString settingFilter;
  int filterLabel;
  StringSettingOptionsFiller filler;
  FilterType m_filterType;

};

class CGUIDialogDSFilters : public CGUIDialogSettingsManualBase
{
public:
  CGUIDialogDSFilters();
  virtual ~CGUIDialogDSFilters();

  static CGUIDialogDSFilters* Get();
  static void Destroy()
  {
    delete m_pSingleton;
    m_pSingleton = NULL;
  }

  static int ShowDSFiltersList();
  void SetNewFilter(bool b);
  bool GetNewFilter();
  void SetFilterIndex(int index);
  int GetFilterIndex();

protected:

  // implementations of ISettingCallback
  virtual void OnSettingChanged(const CSetting *setting);
  virtual void OnSettingAction(const CSetting *setting);
  virtual void OnDeinitWindow(int nextWindowID);

  // specialization of CGUIDialogSettingsBase
  virtual bool AllowResettingSettings() const { return false; }
  virtual void Save();

  // specialization of CGUIDialogSettingsManualBase
  virtual void InitializeSettings();

  virtual void SetupView();

  static void DSFilterOptionFiller(const CSetting *setting, std::vector< std::pair<std::string, std::string> > &list, std::string &current, void *data);
  static void TypeOptionFiller(const CSetting *setting, std::vector< std::pair<std::string, std::string> > &list, std::string &current, void *data);
  static bool compare_by_word(const DynamicStringSettingOption& lhs, const DynamicStringSettingOption& rhs);
  static CGUIDialogDSFilters* m_pSingleton;
  void LoadDsXML(CXBMCTinyXML *XML, TiXmlElement* &pNode, CStdString &xmlFile, bool forceCreate = false);
  void InitFilters(FilterType type, CStdString settingFilter, int FilterLabel, CStdString strFilterName = "", CStdString strFilterAttr = "", StringSettingOptionsFiller filler = NULL);
  void ResetValue();

  bool m_newfilter;
  int m_filterIndex;

  std::vector<DSFiltersList *> m_filterList;
};
