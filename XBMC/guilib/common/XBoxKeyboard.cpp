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
#include "XBoxKeyboard.h"

static DWORD anKeyboardBitmapTable[4*2] =
  {
    1 << 0, 1 << 1, 1 << 2, 1 << 3,
    1 << 16, 1 << 17, 1 << 18, 1 << 19
  };


CLowLevelKeyboard::CLowLevelKeyboard()
{
  ZeroMemory(&m_CurrentKeyStroke, sizeof XINPUT_DEBUG_KEYSTROKE);
  m_dwKeyboardPort = 0;
  m_bShift = false;
  m_bCtrl = false;
  m_bAlt = false;
  m_bRAlt = false;
  m_cAscii = '\0';
  m_bInitialized = false;
}

CLowLevelKeyboard::~CLowLevelKeyboard()
{}

void CLowLevelKeyboard::Initialize(HWND hWnd)

{
  // Check that we are not already initialized and then initialize if necessary
  if ( m_bInitialized )
    return ;

  XINPUT_DEBUG_KEYQUEUE_PARAMETERS keyboardSettings;
  keyboardSettings.dwFlags = XINPUT_DEBUG_KEYQUEUE_FLAG_KEYDOWN | XINPUT_DEBUG_KEYQUEUE_FLAG_KEYREPEAT | XINPUT_DEBUG_KEYQUEUE_FLAG_KEYUP;
  keyboardSettings.dwQueueSize = 25;
  keyboardSettings.dwRepeatDelay = 500;
  keyboardSettings.dwRepeatInterval = 50;

  if ( ERROR_SUCCESS != XInputDebugInitKeyboardQueue( &keyboardSettings ) )
    return ;

  m_bInitialized = true;

  m_dwKeyboardPort = XGetDevices( XDEVICE_TYPE_DEBUG_KEYBOARD );

  // Obtain handles to keyboard devices
  for ( DWORD i = 0; i < XGetPortCount()*2; i++ )
  {
    if ( ( m_hKeyboardDevice[i] == NULL ) && ( m_dwKeyboardPort & anKeyboardBitmapTable[i] ) )
    {
      // Get a handle to the device
      XINPUT_POLLING_PARAMETERS pollValues;
      pollValues.fAutoPoll = TRUE;
      pollValues.fInterruptOut = TRUE;
      pollValues.bInputInterval = 32;
      pollValues.bOutputInterval = 32;
      pollValues.ReservedMBZ1 = 0;
      pollValues.ReservedMBZ2 = 0;

      if (i < XGetPortCount())
      {
        m_hKeyboardDevice[i] = XInputOpen( XDEVICE_TYPE_DEBUG_KEYBOARD, i,
                                           XDEVICE_NO_SLOT, &pollValues );
      }
      else
      {
        m_hKeyboardDevice[i] = XInputOpen( XDEVICE_TYPE_DEBUG_KEYBOARD, i - XGetPortCount(),
                                           XDEVICE_BOTTOM_SLOT, &pollValues );
      }
      CLog::Log(LOGINFO, "Keyboard found on port %i", i);
    }
  }
}

void CLowLevelKeyboard::Update()
{
  // Check if keyboard or keyboards were removed or attached.
  DWORD dwInsertions, dwRemovals;
  XGetDeviceChanges( XDEVICE_TYPE_DEBUG_KEYBOARD, &dwInsertions, &dwRemovals );

  // zero out our current state
  ZeroMemory(&m_CurrentKeyStroke, sizeof XINPUT_DEBUG_KEYSTROKE);

  // Loop through all ports
  for ( DWORD i = 0; i < XGetPortCount()*2; i++ )
  {
    // Handle removed devices.
    if ( dwRemovals & anKeyboardBitmapTable[i] )
    {
      XInputClose( m_hKeyboardDevice[i] );
      m_hKeyboardDevice[i] = NULL;
      CLog::Log(LOGINFO, "Keyboard removed from port %i", i);
    }

    // Handle inserted devices
    if ( dwInsertions & anKeyboardBitmapTable[i] )
    {
      // Now open the device
      XINPUT_POLLING_PARAMETERS pollValues;
      pollValues.fAutoPoll = TRUE;
      pollValues.fInterruptOut = TRUE;
      pollValues.bInputInterval = 32;
      pollValues.bOutputInterval = 32;
      pollValues.ReservedMBZ1 = 0;
      pollValues.ReservedMBZ2 = 0;

      if (i < XGetPortCount())
      {
        m_hKeyboardDevice[i] = XInputOpen( XDEVICE_TYPE_DEBUG_KEYBOARD, i,
                                           XDEVICE_NO_SLOT, &pollValues );
      }
      else
      {
        m_hKeyboardDevice[i] = XInputOpen( XDEVICE_TYPE_DEBUG_KEYBOARD, i - XGetPortCount(),
                                           XDEVICE_BOTTOM_SLOT, &pollValues );
      }
      CLog::Log(LOGINFO, "Keyboard inserted in port %i", i);

      m_bKeyDown = false;
    }

    // If we have a valid device, poll it's state and track button changes
    if ( m_hKeyboardDevice[i] )
    {
      if ( ERROR_SUCCESS == XInputDebugGetKeystroke( &m_CurrentKeyStroke ) )
        break;
    }
  }

  // decide if we need to update our member variables

  if (m_CurrentKeyStroke.Flags & XINPUT_DEBUG_KEYSTROKE_FLAG_KEYUP)
  {
    m_bKeyDown = false;
    //  CLog::DebugLog("Keyboard: Key Up Event :%i", m_CurrentKeyStroke.VirtualKey);
  }
  else if (m_CurrentKeyStroke.Flags & XINPUT_DEBUG_KEYSTROKE_FLAG_REPEAT)
  {
    /*  if (m_bKeyDown)
       CLog::DebugLog("Keyboard: Key Repeat Event :%i", m_CurrentKeyStroke.VirtualKey);
      else
       CLog::DebugLog("Keyboard: Key Repeat Event :%i (ignored)", m_CurrentKeyStroke.VirtualKey);*/
  }
  else if (m_CurrentKeyStroke.VirtualKey != 0 || m_CurrentKeyStroke.Ascii != 0) // keydown event
  {
    m_bKeyDown = true;
    //  CLog::DebugLog("Keyboard: Key Down Event :%i", m_CurrentKeyStroke.VirtualKey);
  }
  if (m_bKeyDown)
  {
    m_cAscii = m_CurrentKeyStroke.Ascii;
    m_bShift = (m_CurrentKeyStroke.Flags & XINPUT_DEBUG_KEYSTROKE_FLAG_SHIFT) != 0;
    m_bCtrl = (m_CurrentKeyStroke.Flags & XINPUT_DEBUG_KEYSTROKE_FLAG_CTRL) != 0;
    m_bAlt = (m_CurrentKeyStroke.Flags & XINPUT_DEBUG_KEYSTROKE_FLAG_ALT) != 0;
    m_VKey = m_CurrentKeyStroke.VirtualKey;
  }
  else
  {
    m_cAscii = 0;
    m_bShift = false;
    m_bCtrl = false;
    m_bAlt = false;
    m_VKey = 0;
  }
}
