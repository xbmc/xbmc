#pragma once
#include "stdstring.h"
#include "../GUIDialogprogress.h"

class CDlgCache
{
  public:
	  CDlgCache();
	  virtual ~CDlgCache();
	  void Update();
	  void SetMessage(const CStdString& strMessage);
	  bool IsCanceled() const { return m_pDlg->IsCanceled(); }
  protected:
    CGUIDialogProgress* m_pDlg;
    CStdString m_strLinePrev;
  
};