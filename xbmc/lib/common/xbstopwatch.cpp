//-----------------------------------------------------------------------------
// File: XbStopWatch.cpp
//
// Desc: StopWatch object using QueryPerformanceCounter
//
// Hist: 01.19.01 - New for March XDK release
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#include "stdafx.h"
#include "XbStopWatch.h"




//-----------------------------------------------------------------------------
// Name: CXBStopWatch()
// Desc: Creates the stopwatch. By default, stopwatch starts running immediately
//-----------------------------------------------------------------------------
CXBStopWatch::CXBStopWatch()
{
    m_fTimerPeriod      = 0.0f;
    m_nStartTick        = 0;
    m_nPrevElapsedTicks = 0;
    m_bIsRunning        = FALSE;

    // Get the timer frequency (ticks per second)
    LARGE_INTEGER qwTimerFreq;
    QueryPerformanceFrequency( &qwTimerFreq );

    // Store as period to avoid division in GetElapsed()
    m_fTimerPeriod = 1.0f / (FLOAT)( qwTimerFreq.QuadPart );
}




//-----------------------------------------------------------------------------
// Name: Start()
// Desc: Starts the stopwatch running. Does not clear any previous time held
//       by the watch.
//-----------------------------------------------------------------------------
VOID CXBStopWatch::Start()
{
    m_nStartTick = GetTicks();
    m_bIsRunning = TRUE;
}




//-----------------------------------------------------------------------------
// Name: StartZero()
// Desc: Starts the stopwatch running at time zero. Same as calling Reset() and
//       then Start();
//-----------------------------------------------------------------------------
VOID CXBStopWatch::StartZero()
{
    m_nPrevElapsedTicks = 0;
    m_nStartTick = GetTicks();
    m_bIsRunning = TRUE;
}




//-----------------------------------------------------------------------------
// Name: Stop()
// Desc: Stops the stopwatch. After stopping the watch, GetElapsed() will 
//       return the same value until the watch is started or reset.
//-----------------------------------------------------------------------------
VOID CXBStopWatch::Stop()
{
    if( m_bIsRunning )
    {
        // Store the elapsed time
        m_nPrevElapsedTicks += GetTicks() - m_nStartTick;
        m_nStartTick = 0;

        // Stop running
        m_bIsRunning = FALSE;
    }
}




//-----------------------------------------------------------------------------
// Name: Reset()
// Desc: Resets the stopwatch to zero. If the watch was running, it continues
//       to run.
//-----------------------------------------------------------------------------
VOID CXBStopWatch::Reset()
{
    m_nPrevElapsedTicks = 0;
    if( m_bIsRunning )
        m_nStartTick = GetTicks();
}




//-----------------------------------------------------------------------------
// Name: IsRunning()
// Desc: TRUE if the stopwatch is running, else FALSE
//-----------------------------------------------------------------------------
BOOL CXBStopWatch::IsRunning() const
{
    return( m_bIsRunning );
}




//-----------------------------------------------------------------------------
// Name: GetElapsedSeconds()
// Desc: The amount of time the watch has been running since it was last
//       reset, in seconds
//-----------------------------------------------------------------------------
FLOAT CXBStopWatch::GetElapsedSeconds() const
{
    // Start with any previous time
    LONGLONG nTotalTicks( m_nPrevElapsedTicks );

    // If the watch is running, add the time since the last start
    if( m_bIsRunning )
        nTotalTicks += GetTicks() - m_nStartTick;

    // Convert to floating pt
    FLOAT fSeconds = (FLOAT)( nTotalTicks ) * m_fTimerPeriod;
    return( fSeconds );
}




//-----------------------------------------------------------------------------
// Name: GetElapsedMilliseconds()
// Desc: The amount of time the watch has been running since it was last
//       reset, in mS
//-----------------------------------------------------------------------------
FLOAT CXBStopWatch::GetElapsedMilliseconds() const
{
    return( GetElapsedSeconds() * 1000.0f );
}




//-----------------------------------------------------------------------------
// Name: GetTicks()
// Desc: Private function wraps QueryPerformanceCounter()
//-----------------------------------------------------------------------------
LONGLONG CXBStopWatch::GetTicks() const
{
    // Grab the current tick count
    LARGE_INTEGER qwCurrTicks;
    QueryPerformanceCounter( &qwCurrTicks );
    return( qwCurrTicks.QuadPart );
}
