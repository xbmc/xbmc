#include "../include.h"
#include "XBoxMouse.h"
#include "../key.h"
#include "../GraphicContext.h"

CMouse g_Mouse; // global

static DWORD anMouseBitmapTable[4*2] =
  {
    1 << 0, 1 << 1, 1 << 2, 1 << 3,
    1 << 16, 1 << 17, 1 << 18, 1 << 19
  };


CMouse::CMouse()
{
  ZeroMemory(&m_CurrentState, sizeof XINPUT_STATE);
  m_dwMousePort = 0;
  m_dwExclusiveWindowID = WINDOW_INVALID;
  m_dwExclusiveControlID = WINDOW_INVALID;
  m_dwState = MOUSE_STATE_NORMAL;
  m_bActive = false;
}

CMouse::~CMouse()
{}

void CMouse::Initialize(HWND hWnd)
{
  m_dwMousePort = XGetDevices( XDEVICE_TYPE_DEBUG_MOUSE );

  // See if a mouse is attached and get a handle to it, if it is.
  for ( DWORD i = 0; i < XGetPortCount()*2; i++ )
  {
    if ( ( m_hMouseDevice[i] == NULL ) && ( m_dwMousePort & anMouseBitmapTable[i] ) )
    {
      // Get a handle to the device
      if (i < XGetPortCount())
      {
        m_hMouseDevice[i] = XInputOpen( XDEVICE_TYPE_DEBUG_MOUSE, i,
                                        XDEVICE_NO_SLOT, NULL );
      }
      else
      {
        m_hMouseDevice[i] = XInputOpen( XDEVICE_TYPE_DEBUG_MOUSE, i - XGetPortCount(),
                                        XDEVICE_BOTTOM_SLOT, NULL );
      }
      CLog::Log(LOGINFO, "Found mouse on port %i", i);
    }
  }
  // Set the default resolution (PAL)
  SetResolution(720, 576, 1, 1);
}

void CMouse::Update()
{
  // Check if mouse or mice were removed or attached.
  // We'll get the handle(s) next frame in the above code.
  DWORD dwNumInsertions, dwNumRemovals;
  if ( XGetDeviceChanges( XDEVICE_TYPE_DEBUG_MOUSE, &dwNumInsertions,
                          &dwNumRemovals ) )
  {
    // Loop through all ports and remove any mice that have been unplugged
    for ( DWORD i = 0; i < XGetPortCount()*2; i++ )
    {
      if ( ( dwNumRemovals & anMouseBitmapTable[i]) && ( m_hMouseDevice[i] != NULL ) )
      {
        XInputClose( m_hMouseDevice[i] );
        m_hMouseDevice[i] = NULL;
        CLog::Log(LOGINFO, "Mouse removed from port %i", i);
      }
    }

    // Set the bits for all of the mice plugged in.
    // We get the handles on the next pass through.
    m_dwMousePort = dwNumInsertions;
    for ( DWORD i = 0; i < XGetPortCount()*2; i++ )
    {
      if ( ( m_hMouseDevice[i] == NULL ) && ( m_dwMousePort & anMouseBitmapTable[i] ) )
      {
        // Get a handle to the device
        if (i < XGetPortCount())
        {
          m_hMouseDevice[i] = XInputOpen( XDEVICE_TYPE_DEBUG_MOUSE, i,
                                          XDEVICE_NO_SLOT, NULL );
        }
        else
        {
          m_hMouseDevice[i] = XInputOpen( XDEVICE_TYPE_DEBUG_MOUSE, i - XGetPortCount(),
                                          XDEVICE_BOTTOM_SLOT, NULL );
        }
        CLog::Log(LOGINFO, "Mouse inserted on port %i", i);
      }
    }
  }

  // Poll the mouse.
  DWORD bMouseMoved = 0;
  for ( DWORD i = 0; i < XGetPortCount()*2; i++ )
  {
    if ( m_hMouseDevice[i] )
      XInputGetState( m_hMouseDevice[i], &m_MouseState[i] );

    if ( m_dwLastMousePacket[i] != m_MouseState[i].dwPacketNumber )
    {
      bMouseMoved |= anMouseBitmapTable[i];
      m_dwLastMousePacket[i] = m_MouseState[i].dwPacketNumber;
    }
  }

  // Check if we have an update...
  if (bMouseMoved)
  {
    // Yes - update our current state
    for ( DWORD i = 0; i < XGetPortCount()*2; i++ )
    {
      if ( bMouseMoved & anMouseBitmapTable[i] )
      {
        m_CurrentState = m_MouseState[i];
      }
    }
    cMickeyX = m_CurrentState.DebugMouse.cMickeysX;
    cMickeyY = m_CurrentState.DebugMouse.cMickeysY;
    posX += ((float)cMickeyX * m_fSpeedX); if (posX < 0) posX = 0; if (posX > m_iMaxX) posX = (float)m_iMaxX;
    posY += ((float)cMickeyY * m_fSpeedY); if (posY < 0) posY = 0; if (posY > m_iMaxY) posY = (float)m_iMaxY;
    cWheel = m_CurrentState.DebugMouse.cWheel;
    // reset our activation timer
    m_bActive = true;
    dwLastActiveTime = timeGetTime();
  }
  else
  {
    cMickeyX = 0;
    cMickeyY = 0;
    cWheel = 0;
    // check how long we've been inactive
    if (timeGetTime() - dwLastActiveTime > MOUSE_ACTIVE_LENGTH) m_bActive = false;
  }
  // Fill in the public members
  bDown[MOUSE_LEFT_BUTTON] = (m_CurrentState.DebugMouse.bButtons & XINPUT_DEBUG_MOUSE_LEFT_BUTTON) != 0;
  bDown[MOUSE_RIGHT_BUTTON] = (m_CurrentState.DebugMouse.bButtons & XINPUT_DEBUG_MOUSE_RIGHT_BUTTON) != 0;
  bDown[MOUSE_MIDDLE_BUTTON] = (m_CurrentState.DebugMouse.bButtons & XINPUT_DEBUG_MOUSE_MIDDLE_BUTTON) != 0;
  bDown[MOUSE_EXTRA_BUTTON1] = (m_CurrentState.DebugMouse.bButtons & XINPUT_DEBUG_MOUSE_XBUTTON1) != 0;
  bDown[MOUSE_EXTRA_BUTTON2] = (m_CurrentState.DebugMouse.bButtons & XINPUT_DEBUG_MOUSE_XBUTTON2) != 0;
  // Perform the click mapping (for single + double click detection)
  bool bNothingDown = true;
  for (int i = 0; i < 5; i++)
  {
    bClick[i] = false;
    bDoubleClick[i] = false;
    bHold[i] = false;
    if (bDown[i])
    {
      bNothingDown = false;
      if (bLastDown[i])
      { // start of hold
        bHold[i] = true;
      }
      else
      {
        if (timeGetTime() - dwLastClickTime[i] < MOUSE_DOUBLE_CLICK_LENGTH)
        { // Double click
          bDoubleClick[i] = true;
        }
        else
        { // Mouse down
        }
      }
    }
    else
    {
      if (bLastDown[i])
      { // Mouse up
        bNothingDown = false;
        bClick[i] = true;
        dwLastClickTime[i] = timeGetTime();
      }
      else
      { // no change
      }
    }
    bLastDown[i] = bDown[i];
  }
  if (bNothingDown)
  { // reset mouse pointer
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
  posX = m_iMaxX * 0.5f;
  posY = m_iMaxY * 0.5f;
}

// IsActive - returns true if we have been active in the last MOUSE_ACTIVE_LENGTH period
bool CMouse::IsActive() const
{
  return m_bActive;
}

// turns off mouse activation
void CMouse::SetInactive()
{
  m_bActive = false;
}

bool CMouse::HasMoved() const
{
  return (cMickeyX && cMickeyY);
}

void CMouse::SetExclusiveAccess(DWORD dwControlID, DWORD dwWindowID, const CPoint &point)
{
  m_dwExclusiveControlID = dwControlID;
  m_dwExclusiveWindowID = dwWindowID;
  // convert posX, posY to screen coords...
  // NOTE: This relies on the window resolution having been set correctly beforehand in CGUIWindow::OnMouseAction()
  CPoint mouseCoords(posX, posY);
  g_graphicsContext.InvertFinalCoords(mouseCoords.x, mouseCoords.y);
  m_exclusiveOffset = point - mouseCoords;
}

void CMouse::EndExclusiveAccess(DWORD dwControlID, DWORD dwWindowID)
{
  if (m_dwExclusiveControlID == dwControlID && m_dwExclusiveWindowID == dwWindowID)
    SetExclusiveAccess(WINDOW_INVALID, WINDOW_INVALID, CPoint(0, 0));
}
