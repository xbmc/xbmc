#pragma once
#include "GUIDialogBoxBase.h"
#include "IProgressCallback.h"

class CGUIDialogProgress :
      public CGUIDialogBoxBase, public IProgressCallback
{
public:
  CGUIDialogProgress(void);
  virtual ~CGUIDialogProgress(void);

  void StartModal();
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);
  virtual void OnWindowLoaded();
  void Progress();
  void ProgressKeys();
  bool IsCanceled() const { return m_bCanceled; }
  void SetPercentage(int iPercentage);
  int GetPercentage() const { return m_percentage; };
  void ShowProgressBar(bool bOnOff);
  void SetHeading(const string& strLine);
  void SetHeading(int iString);             // for convenience to lookup in strings.xml

  // Implements IProgressCallback
  virtual void SetProgressMax(int iMax);
  virtual void SetProgressAdvance(int nSteps=1);
  virtual bool Abort();

  void SetCanCancel(bool bCanCancel);

protected:
  bool m_bCanCancel;
  bool m_bCanceled;
  string m_strHeading;

  int  m_iCurrent;
  int  m_iMax;
  int m_percentage;
};
