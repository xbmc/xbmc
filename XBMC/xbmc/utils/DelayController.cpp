
#include "../stdafx.h"
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


//Currently set static till the need is established
CDelayController::CDelayController(DWORD dwMoveDelay, DWORD dwRepeatDelay) :
	m_wLastDir(0),
	m_dwRepeatDelayOrig(300), m_dwMoveDelay(50)
{
}

void CDelayController::SetDelays( DWORD dwMoveDelay, DWORD dwRepeatDelay )
{
	//Not accepted anymore, cause not likly it is needed
	//m_dwRepeatDelayOrig = dwRepeatDelay;
}


WORD    CDelayController::DirInput( WORD wDir )
{
	WORD wResult= 0;
	if (!wDir) 
		return 0;

	if(GetTickCount() > m_dwLastTime + m_dwMoveDelay || wDir != m_wLastDir) //New button or too much time
	{
		wResult = wDir;
		m_wLastDir = wDir;
		m_dwLastTime = GetTickCount();
		m_bRepeat = false;
	}
	else
	{
		if(m_bRepeat) //We are in repeat mode
		{
			if(GetTickCount() > m_dwLastRepeat + m_dwRepeatDelay + 50) //Only allow key if enough time has passed
			{

				//Speed up the repeat rate each time we come in here
				m_dwRepeatDelay = (DWORD)(m_dwRepeatDelay/1.5);

				m_dwLastRepeat = GetTickCount();
				wResult = wDir;
			}
			else 
				wResult = 0;

		}
		else //Not in repeat mode, disallow key and enable repeat mode
		{
			m_bRepeat = true;
			m_dwRepeatDelay = m_dwRepeatDelayOrig;
			m_dwLastRepeat = GetTickCount();
			wResult = 0;
		}
		
		m_dwLastTime = GetTickCount();
	}
	
	return wResult;}

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
	m_dwMoveDelay = 50;
	return DirInput( wDir );
}

WORD	CDelayController::IRInput( WORD wIR )
{
	//Work around to allow faster navigation, with remotes
	if(wIR >= XINPUT_IR_REMOTE_UP && wIR <= XINPUT_IR_REMOTE_LEFT)
		m_dwMoveDelay = 100;
	else
		m_dwMoveDelay = 200;

	return DirInput( wIR );
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

