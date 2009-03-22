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

#include "GUIWindow.h"
#include "SettingsControls.h"
#include "Settings.h"

class CGUIWindowSettingsCategory :
      public CGUIWindow
{
public:
  CGUIWindowSettingsCategory(void);
  virtual ~CGUIWindowSettingsCategory(void);
  virtual bool OnMessage(CGUIMessage &message);
  virtual bool OnAction(const CAction &action);
  virtual void Render();
  virtual DWORD GetID() const { return CGUIWindow::GetID() + (DWORD)m_iScreen; };

  // static function as it's accessed elsewhere
  static void FillInVisualisations(CSetting *pSetting, int iControlID);
protected:
  virtual void OnInitWindow();

  void CheckNetworkSettings();
  void FillInSubtitleHeights(CSetting *pSetting);
  void FillInSubtitleFonts(CSetting *pSetting);
  void FillInCharSets(CSetting *pSetting);
  void FillInSkinFonts(CSetting *pSetting);
  void FillInSkins(CSetting *pSetting);
  void FillInSoundSkins(CSetting *pSetting);
  void FillInLanguages(CSetting *pSetting);
  void FillInVoiceMasks(DWORD dwPort, CSetting *pSetting);   // Karaoke patch (114097)
  void FillInVoiceMaskValues(DWORD dwPort, CSetting *pSetting); // Karaoke patch (114097)
  void FillInResolutions(CSetting *pSetting, bool playbackSetting);
  void FillInScreenSavers(CSetting *pSetting);
  void FillInRegions(CSetting *pSetting);
  void FillInFTPServerUser(CSetting *pSetting);
  void FillInStartupWindow(CSetting *pSetting);
  void FillInViewModes(CSetting *pSetting, int windowID);
  void FillInSortMethods(CSetting *pSetting, int windowID);
  void ClearFolderViews(CSetting *pSetting, int windowID);
  bool SetFTPServerUserPass();

  void FillInSkinThemes(CSetting *pSetting);
  void FillInSkinColors(CSetting *pSetting);
  void FillInScrapers(CGUISpinControlEx *pControl, const CStdString& strSelected, const CStdString& strContent);

  virtual void SetupControls();
  void CreateSettings();
  void UpdateSettings();
  void UpdateRealTimeSettings();
  void CheckForUpdates();
  void FreeSettingsControls();
  virtual void FreeControls();
  virtual void OnClick(CBaseSettingControl *pSettingControl);
  virtual void OnSettingChanged(CBaseSettingControl *pSettingControl);
  void AddSetting(CSetting *pSetting, float width, int &iControlID);
  CBaseSettingControl* GetSetting(const CStdString &strSetting);

  void JumpToSection(DWORD dwWindowId, const CStdString &section);
  void JumpToPreviousSection();

  std::vector<CBaseSettingControl *> m_vecSettings;
  int m_iSection;
  int m_iScreen;
  RESOLUTION m_NewResolution;
  vecSettingsCategory m_vecSections;
  CGUISpinControlEx *m_pOriginalSpin;
  CGUIRadioButtonControl *m_pOriginalRadioButton;
  CGUIButtonControl *m_pOriginalCategoryButton;
  CGUIButtonControl *m_pOriginalButton;
  CGUIEditControl *m_pOriginalEdit;
  CGUIImage *m_pOriginalImage;
  // Network settings
  int m_iNetworkAssignment;
  CStdString m_strNetworkIPAddress;
  CStdString m_strNetworkSubnet;
  CStdString m_strNetworkGateway;
  CStdString m_strNetworkDNS;
  // look + feel settings (for delayed loading)
  CStdString m_strNewSkinFontSet;
  CStdString m_strNewSkin;
  CStdString m_strNewLanguage;
  CStdString m_strNewSkinTheme;
  CStdString m_strNewSkinColors;

  CStdString m_strErrorMessage;

  CStdString m_strOldTrackFormat;
  CStdString m_strOldTrackFormatRight;

  // state of the window saved in JumpToSection()
  // to get to the previous settings screen when
  // using JumpToPreviousSection()
  int m_iSectionBeforeJump;
  int m_iControlBeforeJump;
  int m_iWindowBeforeJump;

  bool m_returningFromSkinLoad; // true if we are returning from loading the skin
};
