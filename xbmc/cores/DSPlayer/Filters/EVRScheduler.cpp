

#include "utils/log.h"
#include "EVRScheduler.h"

#include <mfapi.h>
#include <mferror.h>
#include "IPinHook.h"
#include <system.h>
#include "DshowUtil/dshowutil.h"

const DWORD SCHEDULER_TIMEOUT = 5000;

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------

CEvrScheduler::CEvrScheduler() : 
  m_pCB(NULL),
  m_pClock(NULL), 
  m_hThreadReadyEvent(NULL),
  m_hFlushEvent(NULL),
  m_fRate(1.0f),
  m_LastSampleTime(0), 
  m_PerFrameInterval(0), 
  m_PerFrame_1_4th(0)
{
}


//-----------------------------------------------------------------------------
// Destructor
//-----------------------------------------------------------------------------

CEvrScheduler::~CEvrScheduler()
{
  //SAFE_RELEASE(m_pClock);
}


//-----------------------------------------------------------------------------
// SetFrameRate
// Specifies the frame rate of the video, in frames per second.
//-----------------------------------------------------------------------------

void CEvrScheduler::SetFrameRate(MFTIME TimePerFrame)
{
  //UINT64 AvgTimePerFrame = 0;

  // Convert to a duration.
  //MFFrameRateToAverageTimePerFrame(fps.Numerator, fps.Denominator, &AvgTimePerFrame);

  m_PerFrameInterval = (MFTIME)TimePerFrame;

  // Calculate 1/4th of this value, because we use it frequently.
  m_PerFrame_1_4th = m_PerFrameInterval / 4;
}



//-----------------------------------------------------------------------------
// StartScheduler
// Starts the scheduler's worker thread.
//
// IMFClock: Pointer to the EVR's presentation clock. Can be NULL.
//-----------------------------------------------------------------------------

HRESULT CEvrScheduler::StartScheduler(IMFClock *pClock)
{
  if (m_ThreadHandle)
    return E_UNEXPECTED;

  HRESULT hr = S_OK;
  DWORD dwID = 0;

  m_pClock = NULL;
  m_pClock = pClock;

  // Set a high the timer resolution (ie, short timer period).
  timeBeginPeriod(1);

  // Create an event to wait for the thread to start.
  m_hThreadReadyEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
  if (!m_hThreadReadyEvent)
  {
    if (FAILED(hr))
      CLog::Log(LOGERROR,"%s",HRESULT_FROM_WIN32(GetLastError()));
  }

  // Create an event to wait for flush commands to complete.
  m_hFlushEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
  if (! m_hFlushEvent)
  {
    if (FAILED(hr))
      CLog::Log(LOGERROR,"%s",HRESULT_FROM_WIN32(GetLastError()));
  }

  // Create the scheduler thread.
  Create(false);
  if (!m_ThreadHandle)
  {
    if (FAILED(hr))
      CLog::Log(LOGERROR,"%s",HRESULT_FROM_WIN32(GetLastError()));
  }

  HANDLE hObjects[] = { m_hThreadReadyEvent, m_ThreadHandle };
  DWORD dwWait = 0;

  // Wait for the thread to signal the "thread ready" event.
  dwWait = WaitForMultipleObjects(2, hObjects, FALSE, INFINITE);  // Wait for EITHER of these handles.
  if (WAIT_OBJECT_0 != dwWait)
  {
    // The thread terminated early for some reason. This is an error condition.
    CloseHandle(m_ThreadHandle);
    m_ThreadHandle = NULL;
    //CHECK_HR(hr = E_UNEXPECTED);
  }

  // Regardless success/failure, we are done using the "thread ready" event.
  if (m_hThreadReadyEvent)
  {
    CloseHandle(m_hThreadReadyEvent);
    m_hThreadReadyEvent = NULL;
  }

  return hr;
}


//-----------------------------------------------------------------------------
// StopScheduler
//
// Stops the scheduler's worker thread.
//-----------------------------------------------------------------------------

HRESULT CEvrScheduler::StopScheduler()
{
  if (!m_ThreadHandle)
    return S_OK;

  // Ask the scheduler thread to exit.
  m_currentEvent = eTerminate;

  // Wait for the thread to exit.
  CLog::Log(LOGDEBUG, "%s Waiting for EVR Scheduler thread to exit", __FUNCTION__);
  StopThread(true);

  CloseHandle(m_hFlushEvent);
  m_hFlushEvent = NULL;

  // Discard samples.
  m_ScheduledSamples.Clear();

  // Restore the timer resolution.
  timeEndPeriod(1);

  return S_OK;
}


//-----------------------------------------------------------------------------
// Flush
//
// Flushes all samples that are queued for presentation.
//
// Note: This method is synchronous; ie., it waits for the flush operation to 
// complete on the worker thread.
//-----------------------------------------------------------------------------

HRESULT CEvrScheduler::Flush()
{
  //This is not an error
  if (!m_ThreadHandle)
    CLog::Log(LOGDEBUG,"%s No scheduler thread!",__FUNCTION__);

  if (m_ThreadHandle)
  {
    // Ask the scheduler thread to flush.
    m_currentEvent = eFlush;

    // Wait for the scheduler thread to signal the flush event,
    // OR for the thread to terminate.
    HANDLE objects[] = { m_hFlushEvent, m_ThreadHandle };

    WaitForMultipleObjects((sizeof(objects) / sizeof(objects[0])), objects, FALSE, SCHEDULER_TIMEOUT);
  }

  return S_OK;
}

//-----------------------------------------------------------------------------
// ScheduleSample
//
// Schedules a new sample for presentation.
//
// pSample:     Pointer to the sample.
// bPresentNow: If TRUE, the sample is presented immediately. Otherwise, the
//              sample's time stamp is used to schedule the sample.
//-----------------------------------------------------------------------------

HRESULT CEvrScheduler::ScheduleSample(IMFSample *pSample, BOOL bPresentNow)
{
  if (!m_pCB)
    return MF_E_NOT_INITIALIZED;

  if (! m_ThreadHandle)
    return MF_E_NOT_INITIALIZED;

  HRESULT hr = S_OK;
  DWORD dwExitCode = 0;

  GetExitCodeThread(m_ThreadHandle, &dwExitCode);
  if (dwExitCode != STILL_ACTIVE)
    return E_FAIL;

  if (bPresentNow || (!m_pClock))
  // Present the sample immediately.
    m_pCB->PresentSample(pSample, 0);
  else
  {
    // Queue the sample and ask the scheduler thread to wake up.
    hr = m_ScheduledSamples.Queue(pSample);
    //m_ScheduledSamples.AddTail(pSample);

    if (SUCCEEDED(hr))
      m_currentEvent = eSchedule;
  }
  if (FAILED(hr))
    CLog::Log(LOGERROR,"%s failed",__FUNCTION__);

  return hr;
}

//-----------------------------------------------------------------------------
// ProcessSamplesInQueue
//
// Processes all the samples in the queue.
//
// plNextSleep: Receives the length of time the scheduler thread should sleep
//              before it calls ProcessSamplesInQueue again.
//-----------------------------------------------------------------------------

HRESULT CEvrScheduler::ProcessSamplesInQueue(LONG *plNextSleep)
{
  HRESULT hr = S_OK;
  LONG lWait = 0;
  IMFSample *pSample = NULL;

  // Process samples until the queue is empty or until the wait time > 0.

  // Note: Dequeue returns S_FALSE when the queue is empty.
  while (m_ScheduledSamples.Dequeue(&pSample) == S_OK )//GetScheduledSample(&pSample) == S_OK) 
  {
    // Process the next sample in the queue. If the sample is not ready
    // for presentation. the value returned in lWait is > 0, which
    // means the scheduler should sleep for that amount of time.

    hr = ProcessSample(pSample, &lWait);
    SAFE_RELEASE(pSample);

    if (FAILED(hr) || lWait > 0)
    {
      break;
    }
  }

  // If the wait time is zero, it means we stopped because the queue is
  // empty (or an error occurred). Set the wait time to infinite; this will
  // make the scheduler thread sleep until it gets another thread message.
  if (lWait == 0)
  {
    lWait = INFINITE;
  }

  *plNextSleep = lWait;
  return hr;
}

float CEvrScheduler::GetFps()
{
  if (m_PerFrameInterval == 0) 
    return float ( 10000000.0/ 417166);
  
  return float (10000000.0 / m_PerFrameInterval);
}

/*HRESULT CEvrScheduler::GetScheduledSample(IMFSample** ppSample)
{
  CAutoLock lock(&m_SampleQueueLock);
  HRESULT    hr = S_OK;

  if (m_ScheduledSamples.dGetCount() > 0)
    *ppSample = m_ScheduledSamples.RemoveHead().Detach();
  else
    hr = MF_E_SAMPLEALLOCATOR_EMPTY;

  return hr;
}*/

//-----------------------------------------------------------------------------
// ProcessSample
//
// Processes a sample.
//
// plNextSleep: Receives the length of time the scheduler thread should sleep.
//-----------------------------------------------------------------------------
 

HRESULT CEvrScheduler::ProcessSample(IMFSample *pSample, LONG *plNextSleep)
{
  HRESULT hr = S_OK;

  LONGLONG hnsPresentationTime = 0;
  LONGLONG hnsTimeNow = 0;
  MFTIME   hnsSystemTime = 0;

  BOOL bPresentNow = TRUE;
  LONG lNextSleep = 0;

  if (m_pClock)
  {
    // Get the sample's time stamp. It is valid for a sample to
    // have no time stamp.
    hr = pSample->GetSampleTime(&hnsPresentationTime);

    // Get the clock time. (But if the sample does not have a time stamp, 
    // we don't need the clock time.)
    if (SUCCEEDED(hr))
    {
      hr = m_pClock->GetCorrelatedTime(0, &hnsTimeNow, &hnsSystemTime);
    }

    // Calculate the time until the sample's presentation time. 
    // A negative value means the sample is late.
    LONGLONG hnsDelta = hnsPresentationTime - hnsTimeNow;   
    if (m_fRate < 0)
    {
        // For reverse playback, the clock runs backward. Therefore the delta is reversed.
      hnsDelta = - hnsDelta;
    }

    if (hnsDelta < - m_PerFrame_1_4th)
    {
        // This sample is late. 
      bPresentNow = TRUE;
    }
    else if (hnsDelta > (3 * m_PerFrame_1_4th))
    {
      // This sample is still too early. Go to sleep.
      lNextSleep = DShowUtil::MFTimeToMsec(hnsDelta - (3 * m_PerFrame_1_4th));

      // Adjust the sleep time for the clock rate. (The presentation clock runs
      // at m_fRate, but sleeping uses the system clock.)
      lNextSleep = (LONG)(lNextSleep / fabsf(m_fRate));

      // Don't present yet.
      bPresentNow = FALSE;
    }
  }

  if (bPresentNow)
  {
    hr = m_pCB->PresentSample(pSample, hnsPresentationTime);
  }
  else
  {
    // The sample is not ready yet. Return it to the queue.
    m_ScheduledSamples.PutBack(pSample);//.AddTail
  }

  *plNextSleep = lNextSleep;

  return hr;
}


//-----------------------------------------------------------------------------
// SchedulerThreadProcPrivate
// 
// Non-static version of the ThreadProc.
//-----------------------------------------------------------------------------

void CEvrScheduler::Process()
{
  HRESULT hr = S_OK;
  LONG    lWait = INFINITE;

  // Signal to the scheduler that the thread is ready.
  SetEvent(m_hThreadReadyEvent);

  while(! m_bStop )
  {
    bool bProcessSamples = true;
    switch (m_currentEvent) 
    {
      case eTerminate:
        CLog::Log(LOGNOTICE,"%s Scheduler terminate" ,__FUNCTION__);
        m_currentEvent = eNone;
        break;

      case eFlush:
        // Flushing: Clear the sample queue and set the event.
        m_ScheduledSamples.Clear();//FlushSamples();

        lWait = INFINITE;
        SetEvent(m_hFlushEvent);
        m_currentEvent = eNone;
        break;

      case eSchedule:
        // Process as many samples as we can.
        if (bProcessSamples)
        {
          hr = ProcessSamplesInQueue(&lWait);
          if (FAILED(hr))
          {
            m_bStop = TRUE;
          }
          bProcessSamples = (lWait != INFINITE); 
        } else
          m_currentEvent = eNone;
        break;
    } // switch 
  
  }  // while (!bExitThread)
}

void CEvrScheduler::OnStartup()
{
  CThread::SetName("EVR Scheduler");
}