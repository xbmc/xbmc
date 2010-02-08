/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */
 
#include "dlgcache.h"
#include "Application.h"
#include "GUIWindowManager.h"
#include "GUIDialogProgress.h"
#include "LocalizeStrings.h"
#include "utils/log.h"
#include "utils/SingleLock.h"
#include "utils/TimeUtils.h"

CDlgCache::CDlgCache(DWORD dwDelay, const CStdString& strHeader, const CStdString& strMsg)
{
  m_pDlg = (CGUIDialogProgress*)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);

  /* if progress dialog is already running, take it over */
  if( m_pDlg->IsDialogRunning() )
    dwDelay = 0;

  m_strHeader = strHeader;
  m_strLinePrev = strMsg;
  bSentCancel = false;

  if(dwDelay == 0)
    OpenDialog();    
  else
    m_dwTimeStamp = CTimeUtils::GetTimeMS() + dwDelay;

  Create(true);
}

void CDlgCache::Close(bool bForceClose)
{
  bSentCancel = true;

  // we cannot wait for the app thread to process the close message
  // as this might happen during player startup which leads to a deadlock
  if (m_pDlg->IsDialogRunning())
    g_application.getApplicationMessenger().Close(m_pDlg,bForceClose,false);

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
  if (m_strHeader.IsEmpty())
    m_pDlg->SetHeading(438);
  else
    m_pDlg->SetHeading(m_strHeader);

  m_pDlg->SetLine(2, m_strLinePrev);
  m_pDlg->StartModal();
  bSentCancel = false;
}

void CDlgCache::SetHeader(const CStdString& strHeader)
{
  m_strHeader = strHeader;
}

void CDlgCache::SetHeader(int nHeader)
{
  SetHeader(g_localizeStrings.Get(nHeader));
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
        if( bSentCancel )
        {
          Sleep(10);
          continue;
        }

        if(m_pDlg->IsCanceled())
        {
          bSentCancel = true;
        }
        else if( !m_pDlg->IsDialogRunning() && CTimeUtils::GetTimeMS() > m_dwTimeStamp 
              && !g_windowManager.IsWindowActive(WINDOW_DIALOG_YES_NO) )
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
