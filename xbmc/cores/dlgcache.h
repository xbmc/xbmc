#pragma once

class CDlgCache
{
public:
  CDlgCache();
  virtual ~CDlgCache();
  void Update();
  void SetMessage(const CStdString& strMessage);
  bool IsCanceled() const { return m_pDlg->IsCanceled(); }
  void ShowProgressBar(bool bOnOff) { m_pDlg->ShowProgressBar(bOnOff); }
  void SetPercentage(int iPercentage) { m_pDlg->SetPercentage(iPercentage); }
protected:
  CGUIDialogProgress* m_pDlg;
  CStdString m_strLinePrev;

};
