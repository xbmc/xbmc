/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogCache.h"

#include "ServiceBroker.h"
#include "dialogs/GUIDialogProgress.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "messaging/ApplicationMessenger.h"
#include "threads/SystemClock.h"
#include "utils/Variant.h"
#include "utils/log.h"

#include <mutex>

using namespace std::chrono_literals;

CGUIDialogCache::CGUIDialogCache(std::chrono::milliseconds delay,
                                 const std::string& strHeader,
                                 const std::string& strMsg)
  : CThread("GUIDialogCache"), m_strHeader(strHeader), m_strLinePrev(strMsg)
{
  bSentCancel = false;

  m_pDlg = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogProgress>(WINDOW_DIALOG_PROGRESS);

  if (!m_pDlg)
    return;

  /* if progress dialog is already running, take it over */
  if( m_pDlg->IsDialogRunning() )
    delay = 0ms;

  if (delay == 0ms)
    OpenDialog();
  else
    m_endtime.Set(delay);

  Create(true);
}

void CGUIDialogCache::Close(bool bForceClose)
{
  bSentCancel = true;

  // we cannot wait for the app thread to process the close message
  // as this might happen during player startup which leads to a deadlock
  if (m_pDlg && m_pDlg->IsDialogRunning())
    CServiceBroker::GetAppMessenger()->PostMsg(TMSG_GUI_WINDOW_CLOSE, -1, bForceClose ? 1 : 0,
                                               static_cast<void*>(m_pDlg));

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
      m_pDlg->SetHeading(CVariant{438});
    else
      m_pDlg->SetHeading(CVariant{m_strHeader});

    m_pDlg->SetLine(2, CVariant{m_strLinePrev});
    m_pDlg->Open();
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
    m_pDlg->SetLine(0, CVariant{m_strLinePrev2});
    m_pDlg->SetLine(1, CVariant{m_strLinePrev});
    m_pDlg->SetLine(2, CVariant{strMessage});
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
        std::unique_lock<CCriticalSection> lock(CServiceBroker::GetWinSystem()->GetGfxContext());
        m_pDlg->Progress();
        if( bSentCancel )
        {
          CThread::Sleep(10ms);
          continue;
        }

        if(m_pDlg->IsCanceled())
        {
          bSentCancel = true;
        }
        else if( !m_pDlg->IsDialogRunning() && m_endtime.IsTimePast()
              && !CServiceBroker::GetGUI()->GetWindowManager().IsWindowActive(WINDOW_DIALOG_YES_NO) )
          OpenDialog();
      }
      catch(...)
      {
        CLog::Log(LOGERROR, "Exception in CGUIDialogCache::Process()");
      }
    }

    CThread::Sleep(10ms);
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
