/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
 
#include "threads/SystemClock.h"
#include "GUIDialogCache.h"
#include "ApplicationMessenger.h"
#include "guilib/GUIWindowManager.h"
#include "dialogs/GUIDialogProgress.h"
#include "guilib/LocalizeStrings.h"
#include "utils/log.h"
#include "threads/SingleLock.h"

CGUIDialogCache::CGUIDialogCache(DWORD dwDelay, const std::string& strHeader, const std::string& strMsg) : CThread("GUIDialogCache"),
  m_strHeader(strHeader),
  m_strLinePrev(strMsg)
{
  bSentCancel = false;

  m_pDlg = (CGUIDialogProgress*)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);

  if (!m_pDlg)
    return;

  /* if progress dialog is already running, take it over */
  if( m_pDlg->IsDialogRunning() )
    dwDelay = 0;

  if(dwDelay == 0)
    OpenDialog();    
  else
    m_endtime.Set((unsigned int)dwDelay);

  Create(true);
}

void CGUIDialogCache::Close(bool bForceClose)
{
  bSentCancel = true;

  // we cannot wait for the app thread to process the close message
  // as this might happen during player startup which leads to a deadlock
  if (m_pDlg && m_pDlg->IsDialogRunning())
    CApplicationMessenger::Get().Close(m_pDlg,bForceClose,false);

  //Set stop, this will kill this object, when thread stops  
  CThread::m_bStop = true;
}

CGUIDialogCache::~CGUIDialogCache()
{
  if(m_pDlg && m_pDlg->IsDialogRunning())
    m_pDlg->Close();
}

void CGUIDialogCache::OpenDialog()
{
  if (m_pDlg)
  {
    if (m_strHeader.empty())
      m_pDlg->SetHeading(438);
    else
      m_pDlg->SetHeading(m_strHeader);

    m_pDlg->SetLine(2, m_strLinePrev);
    m_pDlg->StartModal();
  }
  bSentCancel = false;
}

void CGUIDialogCache::SetHeader(const std::string& strHeader)
{
  m_strHeader = strHeader;
}

void CGUIDialogCache::SetHeader(int nHeader)
{
  SetHeader(g_localizeStrings.Get(nHeader));
}

void CGUIDialogCache::SetMessage(const std::string& strMessage)
{
  if (m_pDlg)
  {
    m_pDlg->SetLine(0, m_strLinePrev2);
    m_pDlg->SetLine(1, m_strLinePrev);
    m_pDlg->SetLine(2, strMessage);
  }
  m_strLinePrev2 = m_strLinePrev;
  m_strLinePrev = strMessage; 
}

bool CGUIDialogCache::OnFileCallback(void* pContext, int ipercent, float avgSpeed)
{
  if (m_pDlg)
  {
    m_pDlg->ShowProgressBar(true);
    m_pDlg->SetPercentage(ipercent);
  }

  if( IsCanceled() ) 
    return false;
  else
    return true;
}

void CGUIDialogCache::Process()
{
  if (!m_pDlg)
    return;

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
        else if( !m_pDlg->IsDialogRunning() && m_endtime.IsTimePast() 
              && !g_windowManager.IsWindowActive(WINDOW_DIALOG_YES_NO) )
          OpenDialog();
      }
      catch(...)
      {
        CLog::Log(LOGERROR, "Exception in CGUIDialogCache::Process()");
      }
    }

    Sleep(10);
  }
}

void CGUIDialogCache::ShowProgressBar(bool bOnOff) 
{
  if (m_pDlg)
    m_pDlg->ShowProgressBar(bOnOff);
}

void CGUIDialogCache::SetPercentage(int iPercentage) 
{ 
  if (m_pDlg)
    m_pDlg->SetPercentage(iPercentage);
}

bool CGUIDialogCache::IsCanceled() const
{
  if (m_pDlg && m_pDlg->IsDialogRunning())
    return m_pDlg->IsCanceled();
  else
    return false;
}
