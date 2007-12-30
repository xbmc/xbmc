
#include "stdafx.h"
#include "dlgcache.h"

extern "C" void mplayer_exit_player(void);

CDlgCache::CDlgCache(DWORD dwDelay)
{
  m_pDlg = (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);

  /* if progress dialog is already running, take it over */
  if( m_pDlg->IsDialogRunning() )
    dwDelay = 0;

  m_strLinePrev = "";

  if(dwDelay == 0)
    OpenDialog();    
  else
    m_dwTimeStamp = GetTickCount() + dwDelay;

  Create(true);
}

void CDlgCache::Close(bool bForceClose)
{
  if (m_pDlg->IsDialogRunning())
    m_pDlg->Close(bForceClose);

  //Set stop, this will kill this object, when thread stops  
  CThread::m_bStop = true;
}

CDlgCache::~CDlgCache()
{
  if(m_pDlg->IsDialogRunning())
    m_pDlg->Close();
}

void CDlgCache::OpenDialog()
{  
  m_pDlg->SetHeading(438);
  m_pDlg->SetLine(2, "");
  m_pDlg->StartModal();
  bSentCancel = false;
}

void CDlgCache::SetMessage(const CStdString& strMessage)
{
  m_pDlg->SetLine(0, m_strLinePrev2);
  m_pDlg->SetLine(1, m_strLinePrev);
  m_pDlg->SetLine(2, strMessage);
  m_strLinePrev2 = m_strLinePrev;
  m_strLinePrev = strMessage; 
}

bool CDlgCache::OnFileCallback(void* pContext, int ipercent, float avgSpeed)
{
  m_pDlg->ShowProgressBar(true);
  m_pDlg->SetPercentage(ipercent); 

  if( IsCanceled() ) 
    return false;
  else
    return true;
}

void CDlgCache::Process()
{
  while( true )
  {
    
    { //Section to make the lock go out of scope before sleep
      
      if( CThread::m_bStop ) break;

      try 
      {
        CSingleLock lock(g_graphicsContext);
        m_pDlg->Progress();
        if( !bSentCancel && m_pDlg->IsCanceled())
        {
          bSentCancel = true;
          mplayer_exit_player(); 
        }
        else if( !m_pDlg->IsDialogRunning() && GetTickCount() > m_dwTimeStamp 
              && !m_gWindowManager.IsWindowActive(WINDOW_DIALOG_YES_NO) )
          OpenDialog();        
      }
      catch(...)
      {
        CLog::Log(LOGERROR, "Exception in CDlgCache::Process()");
      }
    }

    Sleep(10);
  }
}

void CDlgCache::ShowProgressBar(bool bOnOff) 
{ 
  m_pDlg->ShowProgressBar(bOnOff); 
}
void CDlgCache::SetPercentage(int iPercentage) 
{ 
  m_pDlg->SetPercentage(iPercentage); 
}
bool CDlgCache::IsCanceled() const
{
  if (m_pDlg->IsDialogRunning())
    return m_pDlg->IsCanceled();
  else
    return false;
}
