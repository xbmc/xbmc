#pragma once

#include "GUIDialogSettings.h"
#include "ProgramDatabase.h"

class CTrainer;

class CGUIDialogTrainerSettings : public CGUIDialogSettings
{
public:
  CGUIDialogTrainerSettings(void);
  virtual ~CGUIDialogTrainerSettings(void);
  virtual bool OnMessage(CGUIMessage &message);

  static bool ShowForTitle(unsigned int iTitleId, CProgramDatabase* database);
protected:
  virtual void OnInitWindow();
  virtual void SetupPage();
  virtual void CreateSettings();
  virtual void OnCancel();
  void OnSettingChanged(unsigned int setting);

  void AddBool(unsigned int id, const CStdString& strLabel, unsigned char* on);
  
  std::vector<CStdString> m_vecOptions;
  int m_iTrainer;
  int m_iOldTrainer;
  unsigned int m_iTitleId;
  std::vector<CTrainer*> m_vecTrainers;
  CProgramDatabase* m_database;
  bool m_bNeedSave;
  bool m_bCanceled;
  CStdString m_strActive; // active trainer at start - to save db work
};

