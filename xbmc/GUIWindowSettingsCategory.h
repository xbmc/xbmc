#pragma once

#include "GUIWindow.h"
#include "SettingsControls.h"

class CGUIWindowSettingsCategory :
      public CGUIWindow
{
public:
  CGUIWindowSettingsCategory(void);
  virtual ~CGUIWindowSettingsCategory(void);
  virtual bool OnMessage(CGUIMessage &message);
  virtual bool OnAction(const CAction &action);
  virtual void Render();
  virtual DWORD GetID() const { return m_dwWindowId + (DWORD)m_iScreen; };
protected:
  void CheckNetworkSettings();
  void FillInSubtitleHeights(CSetting *pSetting);
  void FillInSubtitleFonts(CSetting *pSetting);
  void FillInCharSets(CSetting *pSetting);
  void FillInSkinFonts(CSetting *pSetting);
  void FillInSkins(CSetting *pSetting);
  void FillInSoundSkins(CSetting *pSetting);
  void FillInLanguages(CSetting *pSetting);
  void FillInVisualisations(CSetting *pSetting);
  void FillInVoiceMasks(DWORD dwPort, CSetting *pSetting);   // Karaoke patch (114097)
  void FillInVoiceMaskValues(DWORD dwPort, CSetting *pSetting); // Karaoke patch (114097)
  void FillInResolutions(CSetting *pSetting);
  void FillInScreenSavers(CSetting *pSetting);
  void FillInXBDateTime(CSetting *pSetting, int bState);

  virtual void SetupControls();
  void CreateSettings();
  void UpdateSettings();
  void FreeSettingsControls();
  virtual void FreeControls();
  virtual void OnClick(CBaseSettingControl *pSettingControl);
  void AddSetting(CSetting *pSetting, int iPosX, int iPosY, int iWidth, int iControlID);
  CBaseSettingControl* GetSetting(const CStdString &strSetting);

  int m_iLastControl;
  vector<CBaseSettingControl *> m_vecSettings;
  int m_iSection;
  int m_iScreen;
  DWORD m_dwResTime;
  RESOLUTION m_OldResolution;
  RESOLUTION m_NewResolution;
  vecSettingsCategory m_vecSections;
  CGUISpinControlEx *m_pOriginalSpin;
  CGUIRadioButtonControl *m_pOriginalRadioButton;
  CGUIButtonControl *m_pOriginalSettingsButton;
  CGUIButtonControl *m_pOriginalButton;
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
  CStdStringW m_strErrorMessage;

  CStdString m_strOldTrackFormat;
  CStdString m_strOldTrackFormatRight;
};
