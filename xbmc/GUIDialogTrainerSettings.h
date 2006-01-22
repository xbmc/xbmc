#pragma once

#include "GUIDialog.h"
#include "GUISpinControlEx.h"
#include "GUIRadioButtonControl.h"
#include "GUIButtonControl.h"
#include "ProgramDatabase.h"

class CTrainer;

class TrainerSetting
{
public:
  enum SETTING_TYPE { NONE=0, BUTTON, CHECK, SPIN, SEPARATOR };
  TrainerSetting()
  {
    id = 0;
    data = NULL;
    type = NONE;
  };
  SETTING_TYPE type;
  CStdString name;
  unsigned int id;
  void *data;
  std::vector<CStdString> entry;
};

class CGUIDialogTrainerSettings : public CGUIDialog
{
public:
  CGUIDialogTrainerSettings(void);
  virtual ~CGUIDialogTrainerSettings(void);
  virtual bool OnMessage(CGUIMessage &message);
  virtual void Render();
  virtual bool HasID(DWORD dwID);

  static void ShowForTitle(unsigned int iTitleId, CProgramDatabase* database);
protected:
  virtual void SetupPage();
  void CreateSettings();
  void UpdateSetting(unsigned int setting);
  void OnSettingChanged(unsigned int setting);
  virtual void FreeControls();
  void OnClick(int iControlID);
  void AddSetting(TrainerSetting &setting, int iPosX, int iPosY, int iWidth, int iControlID);

  void AddSpin(unsigned int id, int label, int* current);
  void AddBool(unsigned int id, const CStdString& strLabel, unsigned char* on);
  void AddSeparator(unsigned int id);
  
  int m_iLastControl;
  int m_iScreen;      // current screen
  int m_iPageOffset;  // offset into the settings list of our current page.
  int m_iCurrentPage;
  int m_iNumPages;
  int m_iNumPerPage;

  CGUISpinControlEx *m_pOriginalSpinButton;
  CGUIRadioButtonControl *m_pOriginalRadioButton;
  CGUIButtonControl *m_pOriginalSettingsButton;
  CGUIImage *m_pOriginalSeparator;

  vector<TrainerSetting> m_settings;

  std::vector<CStdString> m_vecOptions;
  int m_iTrainer;
  int m_iOldTrainer;
  unsigned int m_iTitleId;
  std::vector<CTrainer*> m_vecTrainers;
  CProgramDatabase* m_database;
  bool m_bNeedSave;
  CStdString m_strActive; // active trainer at start - to save db work
};