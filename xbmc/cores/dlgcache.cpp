
#include "stdafx.h"
#include "dlgcache.h"
#include "guiWindowManager.h"

CDlgCache::CDlgCache()
{
  m_pDlg=(CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
  if (m_pDlg)
  {
    m_pDlg->SetHeading(438);
		m_pDlg->SetLine(0,"");
		m_pDlg->SetLine(1,"");
		m_pDlg->SetLine(2,"");
    m_pDlg->StartModal( m_gWindowManager.GetActiveWindow());
    m_strLinePrev="";
  }
}

CDlgCache::~CDlgCache()
{
  if (m_pDlg)
  {
    m_pDlg->Close();
  }
}

void CDlgCache::Update()
{
  if (m_pDlg)
  {
    m_pDlg->Progress();
  }
}
	  
void CDlgCache::SetMessage(const CStdString& strMessage)
{
  if (m_pDlg)
  {
    m_pDlg->SetLine(0,m_strLinePrev);
    m_pDlg->SetLine(1,strMessage);
    m_strLinePrev=strMessage;
  }
}