#ifndef _EVRSCHEDULER_H
#define _EVRSCHEDULER_H
#pragma once


#include "streams.h"
#include <mfapi.h>
#include <mfidl.h>
#include <mferror.h>

#include "EvrHelper.h"
struct SchedulerCallback;


class CEvrScheduler
{
public:
    CEvrScheduler();
    virtual ~CEvrScheduler();

    void SetCallback(SchedulerCallback *pCB)
    {
        m_pCB = pCB;
    }

    void SetFrameRate(const MFRatio& fps);
    void SetClockRate(float fRate) { m_fRate = fRate; }

    const LONGLONG& LastSampleTime() const { return m_LastSampleTime; }
    const LONGLONG& FrameDuration() const { return m_PerFrameInterval; }

    HRESULT StartScheduler(IMFClock *pClock);
    HRESULT StopScheduler();

    HRESULT ScheduleSample(IMFSample *pSample, BOOL bPresentNow);
    HRESULT ProcessSamplesInQueue(LONG *plNextSleep);
    //HRESULT GetScheduledSample(IMFSample** ppSample);
    float   GetFps();
    //void    FlushSamples();
    HRESULT ProcessSample(IMFSample *pSample, LONG *plNextSleep);
    HRESULT Flush();

    // ThreadProc for the scheduler thread.
    static DWORD WINAPI SchedulerThreadProc(LPVOID lpParameter);

private: 
    // non-static version of SchedulerThreadProc.
  DWORD SchedulerThreadProcPrivate();
  CCritSec m_SampleQueueLock;
  
private:
	ThreadSafeQueue<IMFSample>	m_ScheduledSamples;		// Samples waiting to be presented.
  //CInterfaceList<IMFSample, &IID_IMFSample>    m_ScheduledSamples;

    IMFClock            *m_pClock;  // Presentation clock. Can be NULL.
    SchedulerCallback   *m_pCB;     // Weak reference; do not delete.

    DWORD               m_dwThreadID;
    HANDLE              m_hSchedulerThread;
    HANDLE              m_hThreadReadyEvent;
    HANDLE              m_hFlushEvent;

    float               m_fRate;                // Playback rate.
    MFTIME              m_PerFrameInterval;     // Duration of each frame.
    LONGLONG            m_PerFrame_1_4th;       // 1/4th of the frame duration.
    MFTIME              m_LastSampleTime;       // Most recent sample time.
};


//-----------------------------------------------------------------------------
// SchedulerCallback
//
// Defines the callback method to present samples. 
//-----------------------------------------------------------------------------

struct SchedulerCallback
{
    virtual HRESULT PresentSample(IMFSample *pSample, LONGLONG llTarget) = 0;
};


#endif