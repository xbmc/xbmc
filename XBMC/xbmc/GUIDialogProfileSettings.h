#pragma once

#include "GUIDialogSettings.h"

class CGUIDialogProfileSettings : public CGUIDialogSettings
{
public:
  CGUIDialogProfileSettings(void);
  virtual ~CGUIDialogProfileSettings(void);
  virtual bool OnMessage(CGUIMessage &message);

  static bool ShowForProfile(unsigned int iProfile, bool bDetails=true);
protected:
  virtual void OnCancel();
  virtual void OnWindowLoaded();
  virtual void OnInitWindow();
  virtual void SetupPage();
  virtual void CreateSettings();
  void OnSettingChanged(unsigned int setting);
  
  bool m_bNeedSave;
  CStdString m_strName;
  CStdString m_strThumb;
  CStdString m_strDirectory;
  int m_iSourcesMode;
  int m_iDbMode;
  bool m_bIsDefault;
  bool m_bIsNewUser;
  bool m_bShowDetails;

  // lock stuff
  CStdString m_strLockCode;
  int m_iLockMode;
  bool m_bLockSettings;
  bool m_bLockMusic;
  bool m_bLockVideo;
  bool m_bLockFiles;
  bool m_bLockPictures;
  bool m_bLockPrograms;

  CStdString m_strDefaultImage;
};

