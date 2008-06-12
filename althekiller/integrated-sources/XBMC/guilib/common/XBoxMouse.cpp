/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "../include.h"
#include "XBoxMouse.h"

static DWORD anMouseBitmapTable[4*2] =
  {
    1 << 0, 1 << 1, 1 << 2, 1 << 3,
    1 << 16, 1 << 17, 1 << 18, 1 << 19
  };

CXBoxMouse::CXBoxMouse()
{
  ZeroMemory(&m_CurrentState, sizeof(XINPUT_STATE));
  ZeroMemory(m_hMouseDevice, sizeof(HANDLE)*4*2);
  ZeroMemory(m_dwLastMousePacket, sizeof(DWORD)*4*2);
  ZeroMemory(m_MouseState, sizeof(XINPUT_STATE)*4*2);
  m_dwMousePort = 0;
}

CXBoxMouse::~CXBoxMouse()
{}

void CXBoxMouse::Initialize(void *appData)
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
}

bool CXBoxMouse::Update(MouseState &state)
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
    state.dx = m_CurrentState.DebugMouse.cMickeysX;
    state.dy = m_CurrentState.DebugMouse.cMickeysY;
    state.x += state.dx;
    state.y += state.dy;
    state.dz = m_CurrentState.DebugMouse.cWheel;

    state.button[MOUSE_LEFT_BUTTON] = (m_CurrentState.DebugMouse.bButtons & XINPUT_DEBUG_MOUSE_LEFT_BUTTON) != 0;
    state.button[MOUSE_RIGHT_BUTTON] = (m_CurrentState.DebugMouse.bButtons & XINPUT_DEBUG_MOUSE_RIGHT_BUTTON) != 0;
    state.button[MOUSE_MIDDLE_BUTTON] = (m_CurrentState.DebugMouse.bButtons & XINPUT_DEBUG_MOUSE_MIDDLE_BUTTON) != 0;
    state.button[MOUSE_EXTRA_BUTTON1] = (m_CurrentState.DebugMouse.bButtons & XINPUT_DEBUG_MOUSE_XBUTTON1) != 0;
    state.button[MOUSE_EXTRA_BUTTON2] = (m_CurrentState.DebugMouse.bButtons & XINPUT_DEBUG_MOUSE_XBUTTON2) != 0;
    return true;
  }
  return false;
}
