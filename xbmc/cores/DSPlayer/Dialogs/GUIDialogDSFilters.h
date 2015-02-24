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
#include "GUIDialogDSManager.h"

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

protected:

  // implementations of ISettingCallback
  virtual void OnSettingChanged(const CSetting *setting);
  virtual void OnSettingAction(const CSetting *setting);
  virtual void OnInitWindow();
  virtual void OnDeinitWindow(int nextWindowID);
  virtual bool OnBack(int actionID);

  // specialization of CGUIDialogSettingsBase
  virtual bool AllowResettingSettings() const { return false; }
  virtual void Save();

  // specialization of CGUIDialogSettingsManualBase
  virtual void InitializeSettings();

  virtual void SetupView();

  static void TypeOptionFiller(const CSetting *setting, std::vector< std::pair<std::string, std::string> > &list, std::string &current, void *data);
  static CGUIDialogDSFilters* m_pSingleton;
  CStdString GetFilterName(CStdString guid);

  bool isEdited;

  std::vector<DSConfigList *> m_filterList;
  CGUIDialogDSManager* m_dsmanager;
};
