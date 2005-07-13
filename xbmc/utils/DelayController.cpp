//-----------------------------------------------------------------------------
// File: DelayController.cpp
//
// Desc: CDelayController source file
//
// Hist:
//
//
//-----------------------------------------------------------------------------


#include "../stdafx.h"
#include "../XBInputEx.h"
#include "DelayController.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CDelayController::CDelayController( DWORD dwMoveDelay, DWORD dwRepeatDelay ) :
    m_wLastDir(0),
    m_dwTimer(0),
    m_iCount(0),
    m_dwMoveDelay(dwMoveDelay),
    m_dwRepeatDelay(dwRepeatDelay)
{}

void CDelayController::SetDelays( DWORD dwMoveDelay, DWORD dwRepeatDelay )
{
  m_dwMoveDelay = dwMoveDelay;
  m_dwRepeatDelay = dwRepeatDelay;
}


WORD CDelayController::DirInput( WORD wDir )
{
  WORD wResult = 0;

  if ( m_dwRepeatDelay == 0 )
  {
    m_dwRepeatDelay = 200;
  }
  if ( m_dwMoveDelay == 0 )
  {
    m_dwMoveDelay = 700;
  }

  if ( wDir != m_wLastDir )
  {
    if ( wDir == DC_UP )
    {
      wResult |= DC_UP;
      m_dwTimer = GetTickCount() + m_dwRepeatDelay;
    }
    else if ( wDir == DC_DOWN )
    {
      wResult |= DC_DOWN;
      m_dwTimer = GetTickCount() + m_dwRepeatDelay;
    }
    else if ( wDir == DC_RIGHT )
    {
      wResult |= DC_RIGHT;
      m_dwTimer = GetTickCount() + m_dwRepeatDelay;
    }
    else if ( wDir == DC_LEFT )
    {
      wResult |= DC_LEFT;
      m_dwTimer = GetTickCount() + m_dwRepeatDelay;
    }
    else if ( wDir == DC_LEFTTRIGGER )
    {
      wResult |= DC_LEFTTRIGGER;
      m_dwTimer = GetTickCount() + m_dwRepeatDelay;
    }
    else if ( wDir == DC_RIGHTTRIGGER )
    {
      wResult |= DC_RIGHTTRIGGER;
      m_dwTimer = GetTickCount() + m_dwRepeatDelay;
    }
    else
    {
      m_dwTimer = GetTickCount();
    }
    m_iCount = 0;
  }
  else
  {
    if ( m_iCount > 10 )
    {
      if ( wDir == DC_UP )
      {
        wResult |= DC_UP;
      }
      else if ( wDir == DC_DOWN )
      {
        wResult |= DC_DOWN;
      }
      else if ( wDir == DC_LEFT )
      {
        wResult |= DC_LEFT;
      }
      else if ( wDir == DC_RIGHT )
      {
        wResult |= DC_RIGHT;
      }
      else if ( wDir == DC_LEFTTRIGGER )
      {
        wResult |= DC_LEFTTRIGGER;
      }
      else if ( wDir == DC_RIGHTTRIGGER )
      {
        wResult |= DC_RIGHTTRIGGER;
      }
      else
      {
        m_dwTimer = GetTickCount();
        m_iCount = 0;
      }
    }
    else
    {
      if ( wDir == DC_UP )
      {
        if ( m_dwTimer < GetTickCount() )
        {
          wResult |= DC_UP;
          m_iCount++;
          m_dwTimer = GetTickCount() + m_dwMoveDelay;
        }
        else
        {
          wResult |= DC_SKIP;
        }
      }
      else if ( wDir == DC_DOWN )
      {
        if ( m_dwTimer < GetTickCount() )
        {
          wResult |= DC_DOWN;
          m_iCount++;
          m_dwTimer = GetTickCount() + m_dwMoveDelay;
        }
        else
        {
          wResult |= DC_SKIP;
        }
      }
      else if ( wDir == DC_LEFT )
      {
        if ( m_dwTimer < GetTickCount() )
        {
          wResult |= DC_LEFT;
          m_iCount++;
          m_dwTimer = GetTickCount() + m_dwMoveDelay;
        }
        else
        {
          wResult |= DC_SKIP;
        }
      }
      else if ( wDir == DC_RIGHT )
      {
        if ( m_dwTimer < GetTickCount() )
        {
          wResult |= DC_RIGHT;
          m_iCount++;
          m_dwTimer = GetTickCount() + m_dwMoveDelay;
        }
        else
        {
          wResult |= DC_SKIP;
        }
      }
      else if ( wDir == DC_RIGHTTRIGGER )
      {
        if ( m_dwTimer < GetTickCount() )
        {
          wResult |= DC_RIGHTTRIGGER;
          m_iCount++;
          m_dwTimer = GetTickCount() + m_dwMoveDelay;
        }
        else
        {
          wResult |= DC_SKIP;
        }
      }
      else if ( wDir == DC_LEFTTRIGGER )
      {
        if ( m_dwTimer < GetTickCount() )
        {
          wResult |= DC_LEFTTRIGGER;
          m_iCount++;
          m_dwTimer = GetTickCount() + m_dwMoveDelay;
        }
        else
        {
          wResult |= DC_SKIP;
        }
      }
      else
      {
        m_dwTimer = GetTickCount();
        m_iCount = 0;
      }
    }

  }
  m_wLastDir = wDir;
  return wResult;
}

WORD CDelayController::DpadInput( WORD wDpad, bool bLeftTrigger, bool bRightTrigger )
{
  WORD wDir = 0;
  if ( wDpad & XINPUT_GAMEPAD_DPAD_UP )
  {
    wDir = DC_UP;
  }
  if ( wDpad & XINPUT_GAMEPAD_DPAD_DOWN )
  {
    wDir = DC_DOWN;
  }
  if ( wDpad & XINPUT_GAMEPAD_DPAD_LEFT )
  {
    wDir = DC_LEFT;
  }
  if ( wDpad & XINPUT_GAMEPAD_DPAD_RIGHT )
  {
    wDir = DC_RIGHT;
  }
  if ( bLeftTrigger)
  {
    wDir = DC_LEFTTRIGGER;
  }
  if ( bRightTrigger )
  {
    wDir = DC_RIGHTTRIGGER;
  }

  return DirInput( wDir );
}

WORD CDelayController::StickInput( int x, int y )
{
  WORD wDir = 0;
  if ( y == 1 )
  {
    wDir = DC_UP;
  }
  if ( y == -1 )
  {
    wDir = DC_DOWN;
  }
  if ( x == -1 )
  {
    wDir = DC_LEFT;
  }
  if ( x == 1 )
  {
    wDir = DC_RIGHT;
  }
  return DirInput( wDir );
}
