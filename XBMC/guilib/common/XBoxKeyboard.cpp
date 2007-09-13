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
    //  CLog::Log(LOGDEBUG,"Keyboard: Key Up Event :%i", m_CurrentKeyStroke.VirtualKey);
  }
  else if (m_CurrentKeyStroke.Flags & XINPUT_DEBUG_KEYSTROKE_FLAG_REPEAT)
  {
    /*  if (m_bKeyDown)
       CLog::Log(LOGDEBUG,"Keyboard: Key Repeat Event :%i", m_CurrentKeyStroke.VirtualKey);
      else
       CLog::Log(LOGDEBUG,"Keyboard: Key Repeat Event :%i (ignored)", m_CurrentKeyStroke.VirtualKey);*/
  }
  else if (m_CurrentKeyStroke.VirtualKey != 0 || m_CurrentKeyStroke.Ascii != 0) // keydown event
  {
    m_bKeyDown = true;
    //  CLog::Log(LOGDEBUG,"Keyboard: Key Down Event :%i", m_CurrentKeyStroke.VirtualKey);
  }
  if (m_bKeyDown)
  {
    m_cAscii = m_CurrentKeyStroke.Ascii;
    m_bShift = (m_CurrentKeyStroke.Flags & XINPUT_DEBUG_KEYSTROKE_FLAG_SHIFT) != 0;
    m_bCtrl = (m_CurrentKeyStroke.Flags & XINPUT_DEBUG_KEYSTROKE_FLAG_CTRL) != 0;
    m_bAlt = (m_CurrentKeyStroke.Flags & XINPUT_DEBUG_KEYSTROKE_FLAG_ALT) != 0;
    m_VKey = m_CurrentKeyStroke.VirtualKey;
    m_bRAlt = m_VKey == 0xA5; // Right Alt FIXME TESTME HELPME or is this better?: (m_CurrentKeyStroke.Flags & XINPUT_DEBUG_KEYSTROKE_FLAG_ALT) != 0; // I can't know, does a Flag for RALT exist?

/*
    // FIXME HELPME is there any scancode feature?
    // Then we can map scancode to MS virtual keys (copied from SDLKeyboard):

      // following scancode infos are
      // 1. from ubuntu keyboard shortcut (hex) -> predefined 
      // 2. from unix tool xev and my keyboards (decimal)
      // m_VKey infos from CharProbe tool
      // Can we do the same for XBoxKeyboard and DirectInputKeyboard? Can we access the scancode of them? By the way how does SDL do it? I can't find it. (Automagically? But how exactly?)
      // Some pairs of scancode and virtual keys are only known half

      // special "keys" above F1 till F12 on my MS natural keyboard mapped to virtual keys "F13" till "F24"):
      else if (m_keyEvent.key.keysym.scancode == 0xf5) m_VKey = 0x7c; // F13 Launch help browser
      else if (m_keyEvent.key.keysym.scancode == 0x87) m_VKey = 0x7d; // F14 undo
      else if (m_keyEvent.key.keysym.scancode == 0x8a) m_VKey = 0x7e; // F15 redo
      else if (m_keyEvent.key.keysym.scancode == 0x89) m_VKey = 0x7f; // F16 new
      else if (m_keyEvent.key.keysym.scancode == 0xbf) m_VKey = 0x80; // F17 open
      else if (m_keyEvent.key.keysym.scancode == 0xaf) m_VKey = 0x81; // F18 close
      else if (m_keyEvent.key.keysym.scancode == 0xe4) m_VKey = 0x82; // F19 reply
      else if (m_keyEvent.key.keysym.scancode == 0x8e) m_VKey = 0x83; // F20 forward
      else if (m_keyEvent.key.keysym.scancode == 0xda) m_VKey = 0x84; // F21 send
//      else if (m_keyEvent.key.keysym.scancode == 0x) m_VKey = 0x85; // F22 check spell (doesn't work for me with ubuntu)
      else if (m_keyEvent.key.keysym.scancode == 0xd5) m_VKey = 0x86; // F23 save
      else if (m_keyEvent.key.keysym.scancode == 0xb9) m_VKey = 0x87; // 0x2a?? F24 print
      // end of special keys above F1 till F12

      else if (m_keyEvent.key.keysym.scancode == 234) m_VKey = 0xa6; // Browser back
      else if (m_keyEvent.key.keysym.scancode == 233) m_VKey = 0xa7; // Browser forward
//      else if (m_keyEvent.key.keysym.scancode == ) m_VKey = 0xa8; // Browser refresh
//      else if (m_keyEvent.key.keysym.scancode == ) m_VKey = 0xa9; // Browser stop
      else if (m_keyEvent.key.keysym.scancode == 122) m_VKey = 0xaa; // Browser search
      else if (m_keyEvent.key.keysym.scancode == 0xe5) m_VKey = 0xaa; // Browser search
//      else if (m_keyEvent.key.keysym.scancode == ) m_VKey = 0xab; // Browser favorites
      else if (m_keyEvent.key.keysym.scancode == 130) m_VKey = 0xac; // Browser home
      else if (m_keyEvent.key.keysym.scancode == 0xa0) m_VKey = 0xad; // Volume mute
      else if (m_keyEvent.key.keysym.scancode == 0xae) m_VKey = 0xae; // Volume down
      else if (m_keyEvent.key.keysym.scancode == 0xb0) m_VKey = 0xaf; // Volume up
      else if (m_keyEvent.key.keysym.scancode == 0x99) m_VKey = 0xb0; // Next track
      else if (m_keyEvent.key.keysym.scancode == 0x90) m_VKey = 0xb1; // Prev track
      else if (m_keyEvent.key.keysym.scancode == 0xa4) m_VKey = 0xb2; // Stop
      else if (m_keyEvent.key.keysym.scancode == 0xa2) m_VKey = 0xb3; // Play_Pause
      else if (m_keyEvent.key.keysym.scancode == 0xec) m_VKey = 0xb4; // Launch mail
//      else if (m_keyEvent.key.keysym.scancode == ) m_VKey = 0xb5; // Launch media_select
      else if (m_keyEvent.key.keysym.scancode == 198) m_VKey = 0xb6; // Launch App1/PC icon
      else if (m_keyEvent.key.keysym.scancode == 0xa1) m_VKey = 0xb7; // Launch App2/Calculator
      else if (m_keyEvent.key.keysym.scancode == 34) m_VKey = 0xba; // OEM 1: [ on us keyboard
      else if (m_keyEvent.key.keysym.scancode == 51) m_VKey = 0xbf; // OEM 2: additional key on european keyboards between enter and ' on us keyboards
      else if (m_keyEvent.key.keysym.scancode == 47) m_VKey = 0xc0; // OEM 3: ; on us keyboards
      else if (m_keyEvent.key.keysym.scancode == 20) m_VKey = 0xdb; // OEM 4: - on us keyboards (between 0 and =)
      else if (m_keyEvent.key.keysym.scancode == 49) m_VKey = 0xdc; // OEM 5: ` on us keyboards (below ESC)
      else if (m_keyEvent.key.keysym.scancode == 21) m_VKey = 0xdd; // OEM 6: =??? on us keyboards (between - and backspace)
      else if (m_keyEvent.key.keysym.scancode == 48) m_VKey = 0xde; // OEM 7: ' on us keyboards (on right side of ;)
//       else if (m_keyEvent.key.keysym.scancode == ) m_VKey = 0xdf; // OEM 8
      else if (m_keyEvent.key.keysym.scancode == 94) m_VKey = 0xe2; // OEM 102: additional key on european keyboards between left shift and z on us keyboards
//      else if (m_keyEvent.key.keysym.scancode == 0xb2) m_VKey = 0x; // Ubuntu default setting for launch browser
//       else if (m_keyEvent.key.keysym.scancode == 0x76) m_VKey = 0x; // Ubuntu default setting for launch music player
//       else if (m_keyEvent.key.keysym.scancode == 0xcc) m_VKey = 0x; // Ubuntu default setting for eject
*/

  }
  else
  {
    m_cAscii = 0;
    m_bShift = false;
    m_bCtrl = false;
    m_bAlt = false;
    m_bRAlt = false;
    m_VKey = 0;
  }
}