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

class CGUIDialogMadvrSettingsBase : public CGUIDialogSettingsManualBase
{
public:
  CGUIDialogMadvrSettingsBase(int windowId, const std::string &xmlFile);
  virtual ~CGUIDialogMadvrSettingsBase(); 
  
protected:
  virtual void InitializeSettings();
  virtual void OnSettingChanged(const CSetting *setting);
  virtual void OnSettingAction(const CSetting *setting);

  virtual void SaveControlStates();
  virtual void RestoreControlStates();

  static void SetSection(int iSectionId, int label = -1 ) { m_iSectionId = iSectionId; m_label = label; }
  static void MadvrSettingsOptionsString(const CSetting *setting, std::vector< std::pair<std::string, std::string> > &list, std::string &current, void *data);
  static int m_iSectionId;
  static int m_label;

  void SaveMadvrSettings();

  CSettingCategory *m_category;
  std::map<int, int> m_focusPositions;
  bool m_bMadvr;
  int m_iSectionIdInternal;
};
