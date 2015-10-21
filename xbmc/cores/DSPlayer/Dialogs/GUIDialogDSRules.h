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

class CRules
{
public:
  CStdString strName;
  CStdString strfileName;
  CStdString strfileTypes;
  CStdString strVideoCodec;
  CStdString strProtocols;
  CStdString strPriority;
  CStdString strRule;
  int id;
};

class CGUIDialogDSRules : public CGUIDialogSettingsManualBase
{
public:
  CGUIDialogDSRules();
  virtual ~CGUIDialogDSRules();

  static CGUIDialogDSRules* Get();
  static void Destroy()
  {
    delete m_pSingleton;
    m_pSingleton = NULL;
  }

  static int ShowDSRulesList();

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

  static CGUIDialogDSRules* m_pSingleton;

  void ResetValue();
  void HideUnused();
  void HideUnused(ConfigType type, ConfigType subType);
  void SetVisible(CStdString id, bool visible, ConfigType subType, bool isChild = false);
  bool NodeHasAttr(TiXmlElement *pNode, CStdString attr);

  std::vector<DSConfigList *> m_ruleList;
  CGUIDialogDSManager* m_dsmanager;

  bool isEdited;
  bool m_allowchange;

private:
  static bool compare_by_word(CRules* lhs, CRules* rhs)
  {
    CStdString strLine1 = lhs->strPriority;
    CStdString strLine2 = rhs->strPriority;
    StringUtils::ToLower(strLine1);
    StringUtils::ToLower(strLine2);
    return strcmp(strLine1.c_str(), strLine2.c_str()) < 0;
  }
};
