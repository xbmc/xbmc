#include "mouse.h"
#include "../key.h"

CMouse g_Mouse;	// global

CMouse::CMouse()
{
	ZeroMemory(&m_CurrentState, sizeof XINPUT_STATE);
	m_dwMousePort = 0;
	m_dwExclusiveWindowID = WINDOW_INVALID;
	m_dwExclusiveControlID = WINDOW_INVALID;
	m_dwState = MOUSE_STATE_NORMAL;
}

CMouse::~CMouse()
{
}

void CMouse::Initialize()
{
	m_dwMousePort = XGetDevices( XDEVICE_TYPE_DEBUG_MOUSE );

	// Obtain handles to mice
    // We obtain the actual handle(s) below in the 
    // XInput_GetMouseInput function.
	// Set the default resolution (PAL)
	SetResolution(720,576,1,1);
}

void CMouse::Update()
{
    // See if a mouse is attached and get a handle to it, if it is.
    for( DWORD i=0; i < XGetPortCount(); i++ )
    {
        if( ( m_hMouseDevice[i] == NULL ) && ( m_dwMousePort & ( 1 << i ) ) ) 
        {
            // Get a handle to the device
            m_hMouseDevice[i] = XInputOpen( XDEVICE_TYPE_DEBUG_MOUSE, i, 
                                            XDEVICE_NO_SLOT, NULL );
        }
    }

    // Check if mouse or mice were removed or attached.
    // We'll get the handle(s) next frame in the above code.
    DWORD dwNumInsertions, dwNumRemovals;
    if( XGetDeviceChanges( XDEVICE_TYPE_DEBUG_MOUSE, &dwNumInsertions, 
                           &dwNumRemovals ) )
    {
        // Loop through all ports and remove any mice that have been unplugged
        for( DWORD i=0; i < XGetPortCount(); i++ )
        {
            if( ( dwNumRemovals & ( 1 << i ) ) && ( m_hMouseDevice[i] != NULL ) )
            {
                XInputClose( m_hMouseDevice[i] );
                m_hMouseDevice[i] = NULL;
            }
        }

        // Set the bits for all of the mice plugged in.
        // We get the handles on the next pass through.
        m_dwMousePort = dwNumInsertions;
    }

    // Poll the mouse.
    DWORD bMouseMoved = 0;
    for( DWORD i=0; i < XGetPortCount(); i++ )
    {
        if( m_hMouseDevice[i] )
            XInputGetState( m_hMouseDevice[i], &m_MouseState[i] );

        if( m_dwLastMousePacket[i] != m_MouseState[i].dwPacketNumber )
        {
            bMouseMoved |= (1 << i); 
            m_dwLastMousePacket[i] = m_MouseState[i].dwPacketNumber;
        }
    }

	// Check if we have an update...
	if (bMouseMoved)
	{
		// Yes - update our current state
		for( DWORD i=0; i < XGetPortCount(); i++ )
		{
			if( bMouseMoved & ( 1 << i ) )
			{
				m_CurrentState = m_MouseState[i];
			}
		}
		cMickeyX = m_CurrentState.DebugMouse.cMickeysX;
		cMickeyY = m_CurrentState.DebugMouse.cMickeysY;
		iPosX += (int)((float)cMickeyX*m_fSpeedX); if (iPosX < 0) iPosX = 0; if (iPosX > m_iMaxX) iPosX = m_iMaxX;
		iPosY += (int)((float)cMickeyY*m_fSpeedY); if (iPosY < 0) iPosY = 0; if (iPosY > m_iMaxY) iPosY = m_iMaxY;
		cWheel = m_CurrentState.DebugMouse.cWheel;
		// reset our activation timer
		dwLastActiveTime = timeGetTime();
	}
	else
	{
		cMickeyX = 0;
		cMickeyY = 0;
		cWheel = 0;
	}
	// Fill in the public members
	bDown[MOUSE_LEFT_BUTTON] = (m_CurrentState.DebugMouse.bButtons & XINPUT_DEBUG_MOUSE_LEFT_BUTTON)!=0;
	bDown[MOUSE_RIGHT_BUTTON] = (m_CurrentState.DebugMouse.bButtons & XINPUT_DEBUG_MOUSE_RIGHT_BUTTON)!=0;
	bDown[MOUSE_MIDDLE_BUTTON] = (m_CurrentState.DebugMouse.bButtons & XINPUT_DEBUG_MOUSE_MIDDLE_BUTTON)!=0;
	bDown[MOUSE_EXTRA_BUTTON1] = (m_CurrentState.DebugMouse.bButtons & XINPUT_DEBUG_MOUSE_XBUTTON1)!=0;
	bDown[MOUSE_EXTRA_BUTTON2] = (m_CurrentState.DebugMouse.bButtons & XINPUT_DEBUG_MOUSE_XBUTTON2)!=0;
	// Perform the click mapping (for single + double click detection)
	bool bNothingDown = true;
	for (int i=0; i<5; i++)
	{
		bClick[i] = false;
		bDoubleClick[i] = false;
		bHold[i] = false;
		if (bDown[i])
		{
			bNothingDown = false;
			if (bLastDown[i])
			{	// start of hold
				bHold[i] = true;
			}
			else
			{
				if (timeGetTime()-dwLastClickTime[i] < MOUSE_DOUBLE_CLICK_LENGTH)
				{	// Double click
					bDoubleClick[i] = true;
				}
				else
				{	// Mouse down
				}
			}
		}
		else
		{
			if (bLastDown[i])
			{	// Mouse up
				bNothingDown = false;
				bClick[i] = true;
				dwLastClickTime[i] = timeGetTime();
			}
			else
			{	// no change
			}
		}
		bLastDown[i] = bDown[i];
	}
	if (bNothingDown)
	{	// reset mouse pointer
		SetState(MOUSE_STATE_NORMAL);
	}
}

void CMouse::SetResolution(int iXmax, int iYmax, float fXspeed, float fYspeed)
{
	m_iMaxX = iXmax;
	m_iMaxY = iYmax;
	m_fSpeedX = fXspeed;
	m_fSpeedY = fYspeed;
	// reset the coordinates
	iPosX = m_iMaxX/2;
	iPosY = m_iMaxY/2;
}

// IsActive - returns true if we have been active in the last MOUSE_ACTIVE_LENGTH period
bool CMouse::IsActive() const
{
	return (timeGetTime() - dwLastActiveTime < MOUSE_ACTIVE_LENGTH);
}

// turns off mouse activation
void CMouse::SetInactive()
{
	dwLastActiveTime = timeGetTime() - MOUSE_ACTIVE_LENGTH;
}

bool CMouse::HasMoved() const
{
	return (cMickeyX && cMickeyY);
}

void CMouse::SetExclusiveAccess(DWORD dwControlID, DWORD dwWindowID)
{
	m_dwExclusiveControlID = dwControlID;
	m_dwExclusiveWindowID = dwWindowID;
}

void CMouse::EndExclusiveAccess(DWORD dwControlID, DWORD dwWindowID)
{
	if (m_dwExclusiveControlID == dwControlID && m_dwExclusiveWindowID == dwWindowID)
		SetExclusiveAccess(WINDOW_INVALID, WINDOW_INVALID);
}