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

#include "GUIDialogBoxBase.h"
#include "addons/Addon.h"

class CGUIDialogAddonSettings : public CGUIDialogBoxBase
{
public:
  CGUIDialogAddonSettings(void);
  virtual ~CGUIDialogAddonSettings(void);
  virtual bool OnMessage(CGUIMessage& message);
  static bool ShowAndGetInput(const ADDON::AddonPtr &addon);
  virtual void Render();

protected:
  virtual void OnInitWindow();

private:
  /*! \brief return a (localized) addon string.
   \param value either a character string (which is used directly) or a number to lookup in the addons strings.xml
   \param subsetting whether the character string should be prefixed by "- ", defaults to false
   \return the localized addon string
   */
  CStdString GetString(const char *value, bool subSetting = false) const;
  void CreateSections();
  void FreeSections();
  void CreateControls();
  void FreeControls();
  void UpdateFromControls();
  void EnableControls();
  void SetDefaults();
  bool GetCondition(const CStdString &condition, const int controlId);

  void SaveSettings(void);
  bool ShowVirtualKeyboard(int iControl);
  bool TranslateSingleString(const CStdString &strCondition, std::vector<CStdString> &enableVec);

  const TiXmlElement *GetFirstSetting() const;

  ADDON::AddonPtr m_addon;
  CStdString m_strHeading;
  std::map<CStdString,CStdString> m_buttonValues;
  bool m_changed;

  unsigned int m_currentSection;
  unsigned int m_totalSections;

  std::map<CStdString,CStdString> m_settings; // local storage of values
};

