//-----------------------------------------------------------------------------
// File: DelayController.cpp
//
// Desc: CDelayController source file
//
// Hist: 
//
// 
//-----------------------------------------------------------------------------

#include <xtl.h>
#include "../XBInputEx.h"
#include "DelayController.h"



CDelayController::CDelayController( DWORD dwMoveDelay, DWORD dwRepeatDelay ) :
	m_wLastDir(0),
	m_dwTimer(0),
	m_iCount(0),
	m_dwMoveDelay(dwMoveDelay),
	m_dwRepeatDelay(dwRepeatDelay)
{
}

void CDelayController::SetDelays( DWORD dwMoveDelay, DWORD dwRepeatDelay )
{
	m_dwMoveDelay = dwMoveDelay;
	m_dwRepeatDelay = dwRepeatDelay;
}


WORD    CDelayController::DirInput( WORD wDir )
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
			m_dwTimer = GetTickCount()+m_dwRepeatDelay;
		}
		else if ( wDir == DC_DOWN )
		{
			wResult |= DC_DOWN;
			m_dwTimer = GetTickCount()+m_dwRepeatDelay;
		}
		else if ( wDir == DC_RIGHT )
		{
			wResult |= DC_RIGHT;
			m_dwTimer = GetTickCount()+m_dwRepeatDelay;
		}
		else if ( wDir == DC_LEFT )
		{
			wResult |= DC_LEFT;
			m_dwTimer = GetTickCount()+m_dwRepeatDelay;
		}
		else if ( wDir == DC_LEFTTRIGGER )
		{
			wResult |= DC_LEFTTRIGGER;
			m_dwTimer = GetTickCount()+m_dwRepeatDelay;
		}
		else if ( wDir == DC_RIGHTTRIGGER )
		{
			wResult |= DC_RIGHTTRIGGER;
			m_dwTimer = GetTickCount()+m_dwRepeatDelay;
		}
		else
		{
			m_dwTimer = GetTickCount();
		}
		m_iCount=0;
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
				m_iCount=0;
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
					m_dwTimer = GetTickCount()+m_dwMoveDelay;
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
					m_dwTimer = GetTickCount()+m_dwMoveDelay;
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
					m_dwTimer = GetTickCount()+m_dwMoveDelay;
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
					m_dwTimer = GetTickCount()+m_dwMoveDelay;
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
					m_dwTimer = GetTickCount()+m_dwMoveDelay;
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
					m_dwTimer = GetTickCount()+m_dwMoveDelay;
				}
				else
				{
					wResult |= DC_SKIP;
				}
			}
			else
			{
				m_dwTimer = GetTickCount();
				m_iCount=0;
			}
		}
		
	}
	m_wLastDir = wDir;
	return wResult;
}

WORD	CDelayController::DpadInput( WORD wDpad,bool bLeftTrigger,bool bRightTrigger )
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

WORD	CDelayController::IRInput( WORD wIR )
{
	WORD wDir = 0;
	if ( wIR == XINPUT_IR_REMOTE_UP )       wDir = DC_UP;
	if ( wIR == XINPUT_IR_REMOTE_DOWN )     wDir = DC_DOWN;
	if ( wIR == XINPUT_IR_REMOTE_LEFT )     wDir = DC_LEFT;
	if ( wIR == XINPUT_IR_REMOTE_RIGHT )    wDir = DC_RIGHT;
	if ( wIR == XINPUT_IR_REMOTE_REVERSE )  wDir = DC_LEFTTRIGGER;
	if ( wIR == XINPUT_IR_REMOTE_FORWARD)   wDir = DC_RIGHTTRIGGER;
	if ( wIR == XINPUT_IR_REMOTE_DISPLAY)		wDir=DC_REMOTE_DISPLAY;
	if ( wIR == XINPUT_IR_REMOTE_PLAY)       wDir=DC_REMOTE_PLAY;
	if ( wIR == XINPUT_IR_REMOTE_SKIP_MINUS) wDir=DC_REMOTE_SKIP_MINUS;
	if ( wIR == XINPUT_IR_REMOTE_STOP)       wDir=DC_REMOTE_STOP;
	if ( wIR == XINPUT_IR_REMOTE_PAUSE)      wDir=DC_REMOTE_PAUSE;
	if ( wIR == XINPUT_IR_REMOTE_SKIP_PLUS)  wDir=DC_REMOTE_SKIP_PLUS;
	if ( wIR == XINPUT_IR_REMOTE_TITLE)      wDir=DC_REMOTE_TITLE;
	if ( wIR == XINPUT_IR_REMOTE_INFO)       wDir=DC_REMOTE_INFO;
	if ( wIR == XINPUT_IR_REMOTE_SELECT)     wDir=DC_REMOTE_SELECT;
	if ( wIR == XINPUT_IR_REMOTE_MENU)       wDir=DC_REMOTE_MENU;
	if ( wIR == XINPUT_IR_REMOTE_BACK)       wDir=DC_REMOTE_BACK;
	if ( wIR == XINPUT_IR_REMOTE_1)          wDir=DC_REMOTE_1;
	if ( wIR == XINPUT_IR_REMOTE_2)          wDir=DC_REMOTE_2;
	if ( wIR == XINPUT_IR_REMOTE_3)          wDir=DC_REMOTE_3;
	if ( wIR == XINPUT_IR_REMOTE_4)          wDir=DC_REMOTE_4;
	if ( wIR == XINPUT_IR_REMOTE_5)          wDir=DC_REMOTE_5;
	if ( wIR == XINPUT_IR_REMOTE_6)          wDir=DC_REMOTE_6;
	if ( wIR == XINPUT_IR_REMOTE_7)          wDir=DC_REMOTE_7;
	if ( wIR == XINPUT_IR_REMOTE_8)          wDir=DC_REMOTE_8;
	if ( wIR == XINPUT_IR_REMOTE_9)          wDir=DC_REMOTE_9;
	if ( wIR == XINPUT_IR_REMOTE_0)          wDir=DC_REMOTE_0;


	return DIRInput( wDir );
}

WORD	CDelayController::StickInput( int x, int y )
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

WORD    CDelayController::DIRInput( WORD wDir )
{
	WORD wResult = 0;
	if (!wDir) return 0;
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
		// key pressed is different then previous key
		if ( wDir >=DC_UP )
		{
			// key is pressed
			wResult = wDir;
			m_dwTimer = GetTickCount()+m_dwRepeatDelay;
		}
		else
		{
			// no key is pressed
			m_dwTimer = GetTickCount();
		}
		m_iCount=0;
	}
	else
	{
		// key pressed is the same as last time
		if ( m_iCount > 10 )
		{
			if ( wDir >= DC_UP )
			{
				wResult = wDir;
			}
			else
			{
				m_dwTimer = GetTickCount();
				m_iCount=0;
			}
		}
		else
		{
			if ( wDir >= DC_UP )
			{
				if ( m_dwTimer < GetTickCount() )
				{
					wResult = wDir;
					m_iCount++;
					m_dwTimer = GetTickCount()+m_dwMoveDelay;
				}
				else
				{
					wResult = DC_SKIP;
				}
			}
			else
			{
				m_dwTimer = GetTickCount();
				m_iCount=0;
			}
		}

	}
	m_wLastDir = wDir;
	return wResult;
}
