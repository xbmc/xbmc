#ifndef GUIDIALOG_PLUGIN_SETTINGS_
#define GUIDIALOG_PLUGIN_SETTINGS_

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
#include "settings/AddonSettings.h"

struct SScraperInfo;

class CGUIDialogAddonSettings : public CGUIDialogBoxBase
{
public:
  CGUIDialogAddonSettings(void);
  virtual ~CGUIDialogAddonSettings(void);
  virtual bool OnMessage(CGUIMessage& message);
  static void ShowAndGetInput(CURL& url);
  static void ShowAndGetInput(SScraperInfo& info);

private:
  void CreateControls();
  void FreeControls();
  void EnableControls();
  void SetDefaults();
  bool GetCondition(const CStdString &condition, const int controlId);

  bool SaveSettings(void);
  bool ShowVirtualKeyboard(int iControl);
  static CURL m_url;
  bool TranslateSingleString(const CStdString &strCondition, std::vector<CStdString> &enableVec);
  CAddonSettings m_settings;
  CStdString m_strHeading;
};

#endif

