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

#include "settings/dialogs/GUIDialogSettingsManagerBase.h"

class CSettings;

class CGUIWindowSettingsCategory : public CGUIDialogSettingsManagerBase
{
public:
  CGUIWindowSettingsCategory();
  virtual ~CGUIWindowSettingsCategory();

  // specialization of CGUIControl
  virtual bool OnMessage(CGUIMessage &message);
  virtual bool OnAction(const CAction &action);
  virtual bool OnBack(int actionID);
  virtual int GetID() const { return CGUIDialogSettingsManagerBase::GetID() + m_iSection; };

  // specialization of CGUIWindow
  virtual bool IsDialog() const { return false; }

protected:
  // specialization of CGUIWindow
  virtual void OnWindowLoaded();

  // implementation of CGUIDialogSettingsBase
  virtual int GetSettingLevel() const;
  virtual CSettingSection* GetSection();
  virtual void Save();
  
  CSettings& m_settings;
  int m_iSection;
  bool m_returningFromSkinLoad; // true if we are returning from loading the skin
};
