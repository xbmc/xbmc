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

  // static function as it's accessed elsewhere
  static void FillInVisualisations(CSetting *pSetting, int iControlID);
protected:
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
  void FillInResolutions(CSetting *pSetting);
  void FillInScreenSavers(CSetting *pSetting);
  bool CheckMasterLockCode();
  void FillInFTPServerUser(CSetting *pSetting);
  bool SetFTPServerUserPass();

  void FillInSkinThemes(CSetting *pSetting);

  virtual void SetupControls();
  void CreateSettings();
  void UpdateSettings();
  void UpdateRealTimeSettings();
  void FreeSettingsControls();
  virtual void FreeControls();
  virtual void OnClick(CBaseSettingControl *pSettingControl);
  void AddSetting(CSetting *pSetting, int iPosX, int &iPosY, int iGap, int iWidth, int &iControlID);
  CBaseSettingControl* GetSetting(const CStdString &strSetting);

  void JumpToSection(DWORD dwWindowId, int iSection);
  void JumpToPreviousSection();

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
  CStdStringW m_strErrorMessage;

  CStdString m_strOldTrackFormat;
  CStdString m_strOldTrackFormatRight;

  // state of the window saved in JumpToSection()
  // to get to the previous settings screen when
  // using JumpToPreviousSection()
  int m_iSectionBeforeJump;
  int m_iControlBeforeJump;
  int m_iWindowBeforeJump;
};
