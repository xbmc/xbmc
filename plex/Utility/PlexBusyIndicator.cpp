#include "PlexBusyIndicator.h"
#include "JobManager.h"
#include "dialogs/GUIDialogBusy.h"
#include "guilib/GUIWindowManager.h"
#include "boost/foreach.hpp"
#include "log.h"
#include "Application.h"
#include "PlexJobs.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
CPlexBusyIndicator::CPlexBusyIndicator()
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CPlexBusyIndicator::blockWaitingForJob(CJob* job, IJobCallback* callback, CFileItemListPtr *result)
{
  CSingleLock lk(m_section);
  m_blockEvent.Reset();
  int id = CJobManager::GetInstance().AddJob(job, this);

  m_callbackMap[id] = callback;
  m_resultMap[id] = result;

  lk.Leave();
  m_blockEvent.WaitMSec(300); // wait an initial 300ms if this is a fast operation.

  CGUIDialogBusy* busy = NULL;

  if (g_application.IsCurrentThread())
  {
    busy = (CGUIDialogBusy*)g_windowManager.GetWindow(WINDOW_DIALOG_BUSY);
    if (busy)
      busy->Show();
  }

  lk.Enter();

  bool success = true;
  while (m_callbackMap.size() > 0)
  {
    lk.Leave();
    while (!m_blockEvent.WaitMSec(20))
    {
      lk.Enter();
      if (busy && busy->IsCanceled())
      {
        std::pair<int, IJobCallback*> p;
        BOOST_FOREACH(p, m_callbackMap)
          CJobManager::GetInstance().CancelJob(p.first);

        // Let's get out of here.
        m_callbackMap.clear();
        m_resultMap.clear();
        m_blockEvent.Set();
        success = false;
      }
      lk.Leave();
      g_windowManager.ProcessRenderLoop();
    }
    lk.Enter();
  }

  if (busy && busy->IsActive())
    busy->Close();

  return success;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexBusyIndicator::OnJobComplete(unsigned int jobID, bool success, CJob* job)
{
  CSingleLock lk(m_section);

  if (m_callbackMap.find(jobID) != m_callbackMap.end())
  {
    IJobCallback* cb = m_callbackMap[jobID];
    lk.Leave();
    if (cb)
      cb->OnJobComplete(jobID, success, job);

    if (m_resultMap[jobID])
    {
      CPlexJob *plexjob = dynamic_cast<CPlexJob*>(job);
      if (plexjob)
      {
        *m_resultMap[jobID] = plexjob->getResult();
      }
    }

    lk.Enter();
    m_callbackMap.erase(jobID);
    m_resultMap.erase(jobID);

    if (m_callbackMap.size() == 0)
    {
      CLog::Log(LOGDEBUG, "CPlexBusyIndicator::OnJobComplete nothing more blocking, let's leave");
      m_blockEvent.Set();
    }
  }
  else
  {
    CLog::Log(LOGDEBUG, "CPlexBusyIndicator::OnJobComplete ouch, we got %d that we don't have a callback for?", jobID);
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexBusyIndicator::OnJobProgress(unsigned int jobID, unsigned int progress,
                                       unsigned int total, const CJob* job)
{
  CSingleLock lk(m_section);

  if (m_callbackMap.find(jobID) != m_callbackMap.end())
  {
    IJobCallback* cb = m_callbackMap[jobID];
    lk.Leave();
    if (cb)
      cb->OnJobProgress(jobID, progress, total, job);
  }
  else
  {
    CLog::Log(LOGDEBUG, "CPlexBusyIndicator::OnJobProgress ouch, we got %d that we don't have a callback for?", jobID);
  }
}
