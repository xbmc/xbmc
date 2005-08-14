
#include "../stdafx.h"
#include "dlgcache.h"

extern "C" void mplayer_exit_player(void);

CDlgCache::CDlgCache()
{
  m_pDlg = (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
  if (m_pDlg)
  {
    m_pDlg->SetHeading(438);
    m_pDlg->SetLine(0, "");
    m_pDlg->SetLine(1, "");
    m_pDlg->SetLine(2, "");
    m_pDlg->StartModal( m_gWindowManager.GetActiveWindow());
    m_strLinePrev = "";
  }
  bSentCancel = false;
  Create();
}

CDlgCache::~CDlgCache()
{
  StopThread();
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
    if( !bSentCancel && m_pDlg->IsCanceled())
    {
      mplayer_exit_player();      
    }
  }
}

void CDlgCache::SetMessage(const CStdString& strMessage)
{
  if (m_pDlg)
  {    
    m_pDlg->SetLine(0, m_strLinePrev);
    m_pDlg->SetLine(1, strMessage);
    m_strLinePrev = strMessage;
  }
}

void CDlgCache::Process()
{
  while(!CThread::m_bStop )
  {
    try 
    {
      CGraphicContext::CLock lock(g_graphicsContext);
      Update();
    }
    catch(...)
    {
      CLog::Log(LOGERROR, "Exception in CDlgCache::Process()");
    }
    Sleep(10);
  }
};
