#pragma once
#include "GUIDialogBoxBase.h"

class CGUIDialogProgress :
      public CGUIDialogBoxBase
{
public:
  CGUIDialogProgress(void);
  virtual ~CGUIDialogProgress(void);

  void StartModal(DWORD dwParentId);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);
  void Progress();
  void ProgressKeys();
  bool IsCanceled() const { return m_bCanceled; }
  void SetPercentage(int iPercentage);
  void ShowProgressBar(bool bOnOff);
  void SetHeading(const wstring& strLine);
  void SetHeading(const string& strLine);
  void SetHeading(int iString);
  void SetProgressBarMax(int iMax);
  void StepProgressBar(int nSteps=1);
protected:
  bool m_bCanceled;
  wstring m_strHeading;

  int  m_iCurrent;
  int  m_iMax;
};
