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

#include "guilib/GUIWindow.h"
#include "SettingsControls.h"
#include "GUISettings.h"
#include "utils/Stopwatch.h"

class CGUIWindowSettingsCategory :
      public CGUIWindow
{
public:
  CGUIWindowSettingsCategory(void);
  virtual ~CGUIWindowSettingsCategory(void);
  virtual bool OnMessage(CGUIMessage &message);
  virtual bool OnBack(int actionID);
  virtual void FrameMove();
  virtual void Render();
  virtual void DoProcess(unsigned int currentTime, CDirtyRegionList &dirtyregions);
  virtual int GetID() const { return CGUIWindow::GetID() + m_iScreen; };

protected:
  virtual void OnInitWindow();

  void CheckNetworkSettings();
  void FillInSubtitleHeights(CSetting *pSetting, CGUISpinControlEx *pControl);
  void FillInSubtitleFonts(CSetting *pSetting);
  void FillInCharSets(CSetting *pSetting);
  void FillInSkinFonts(CSetting *pSetting);
  void FillInSoundSkins(CSetting *pSetting);
  void FillInLanguages(CSetting *pSetting);
  DisplayMode FillInScreens(CStdString strSetting, RESOLUTION res);
  void FillInResolutions(CStdString strSetting, DisplayMode mode, RESOLUTION res, bool UserChange);
  void FillInRefreshRates(CStdString strSetting, RESOLUTION res, bool UserChange);
  void OnRefreshRateChanged(RESOLUTION resolution);
  void FillInRegions(CSetting *pSetting);
  void FillInStartupWindow(CSetting *pSetting);
  void FillInViewModes(CSetting *pSetting, int windowID);
  void FillInSortMethods(CSetting *pSetting, int windowID);
  void FillInEpgGuideView(CSetting *pSetting);
  void FillInPvrStartLastChannel(CSetting *pSetting);

  void FillInSkinThemes(CSetting *pSetting);
  void FillInSkinColors(CSetting *pSetting);

  void FillInNetworkInterfaces(CSetting *pSetting, float groupWidth, int &iControlID);
  void NetworkInterfaceChanged(void);

  void FillInAudioDevices(CSetting* pSetting, bool Passthrough = false);

  virtual void SetupControls();
  CGUIControl* AddIntBasedSpinControl(CSetting *pSetting, float groupWidth, int &iControlID);
  void CreateSettings();
  void UpdateSettings();
  void CheckForUpdates();
  void FreeSettingsControls();
  virtual void FreeControls();
  virtual void OnClick(CBaseSettingControl *pSettingControl);
  virtual void OnSettingChanged(CBaseSettingControl *pSettingControl);
  CGUIControl* AddSetting(CSetting *pSetting, float width, int &iControlID);
  CBaseSettingControl* GetSetting(const CStdString &strSetting);

  void ValidatePortNumber(CBaseSettingControl* pSettingControl, const CStdString& userPort, const CStdString& privPort, bool listening=true);

  std::vector<CBaseSettingControl *> m_vecSettings;
  int m_iSection;
  int m_iScreen;
  vecSettingsCategory m_vecSections;
  CGUISpinControlEx *m_pOriginalSpin;
  CGUIRadioButtonControl *m_pOriginalRadioButton;
  CGUIButtonControl *m_pOriginalCategoryButton;
  CGUIButtonControl *m_pOriginalButton;
  CGUIEditControl *m_pOriginalEdit;
  CGUIImage *m_pOriginalImage;

  CStdString m_strErrorMessage;

  CStdString m_strOldTrackFormat;
  CStdString m_strOldTrackFormatRight;

  std::map<CStdString, CStdString> m_AnalogAudioSinkMap;
  std::map<CStdString, CStdString> m_DigitalAudioSinkMap;
  std::map<CStdString, CStdString> m_SkinFontSetIDs;

  bool m_returningFromSkinLoad; // true if we are returning from loading the skin

  CBaseSettingControl *m_delayedSetting; ///< Current delayed setting \sa CBaseSettingControl::SetDelayed()
  CStopWatch           m_delayedTimer;   ///< Delayed setting timer
};

