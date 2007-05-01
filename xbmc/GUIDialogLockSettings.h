#pragma once

#include "GUIDialogSettings.h"

class CGUIDialogLockSettings : public CGUIDialogSettings
{
public:
  CGUIDialogLockSettings(void);
  virtual ~CGUIDialogLockSettings(void);
  virtual bool OnMessage(CGUIMessage &message);
  static bool ShowAndGetLock(int& iLockMode, CStdString& strPassword, int iHeader=20091);
  static bool ShowAndGetLock(int& iLockMode, CStdString& strPassword, bool& bLockMusic, bool& bLockVideo, bool& bLockPictures, bool& bLockPrograms, bool& bLockFiles, bool& bLockSettings, int iButtonLabel=20091,bool bConditional=false, bool bDetails=true);
  static bool ShowAndGetUserAndPassword(CStdString& strUser, CStdString& strPassword, const CStdString& strURL);
protected:
  virtual void OnCancel();
  virtual void OnInitWindow();
  virtual void SetupPage();
  virtual void CreateSettings();
  void OnSettingChanged(unsigned int setting);
  void EnableDetails(bool bEnable);

  int m_iLock;
  CStdString m_strLock;
  CStdString m_strUser;
  CStdString m_strURL;
  bool m_bChanged;
  bool m_bDetails;
  bool m_bConditionalDetails;
  bool m_bGetUser;
  int m_iButtonLabel;
  bool m_bLockMusic; 
  bool m_bLockVideo;
  bool m_bLockPictures;
  bool m_bLockPrograms;
  bool m_bLockFiles;
  bool m_bLockSettings;
};

