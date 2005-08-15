#pragma once

class CDlgCache : public CThread
{
public:
  CDlgCache(DWORD dwDelay = 0);
  virtual ~CDlgCache();
  void Update();
  void SetMessage(const CStdString& strMessage);
  bool IsCanceled() const;
  void ShowProgressBar(bool bOnOff);
  void SetPercentage(int iPercentage);

  void Close();

  virtual void Process();

protected:

  void OpenDialog();

  DWORD m_dwTimeStamp;
  DWORD m_dwDelay;
  CGUIDialogProgress* m_pDlg;
  CStdString m_strLinePrev;  
  bool bSentCancel;
};
