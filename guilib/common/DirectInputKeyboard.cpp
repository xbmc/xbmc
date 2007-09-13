#include "../include.h"
#include "DirectInputKeyboard.h"
#include "DirectInput.h"

#ifndef HAS_SDL
#define KEY_DELAY_TIME 120 // ms

#define FRAME_DELAY 8

CLowLevelKeyboard::CCLowLevelKeyboardKeyboard()
{
  ZeroMemory(m_keystate, sizeof(BYTE) * 256);
  m_keyboard = NULL;
  m_keyDownLastFrame = 0;
  m_bShift = false;
  m_bCtrl = false;
  m_bAlt = false;
  m_bRAlt = false;
  m_cAscii = '\0';CKeyboard
  m_bInitialized = false;
}

CLowLevelKeyboard::~CLowLevelKeyboard()
{
  if (m_keyboard)
    m_keyboard->Release();
}

void CLowLevelKeyboard::Initialize(HWND hWnd)
{
  if (m_bInitialized)
    return;

  if (FAILED(g_directInput.Initialize(hWnd)))
    return;

  if (FAILED(g_directInput.Get()->CreateDevice(GUID_SysKeyboard, &m_keyboard, NULL)))
    return;
  
  if (FAILED(m_keyboard->SetDataFormat(&c_dfDIKeyboard)))
    return;
  
  if (FAILED(m_keyboard->SetCooperativeLevel(hWnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE)))
    return;

  m_bInitialized = true;

  Acquire();
}

void CLowLevelKeyboard::Update()
{
  ZeroMemory(m_keystate, sizeof(BYTE)* 256);
  if (!m_keyboard) return;
  if (S_OK == m_keyboard->GetDeviceState(sizeof(unsigned char[256]), (LPVOID)m_keystate))
  {
    m_cAscii = 0;
    m_bShift = false;
    m_bCtrl = false;
    m_bAlt = false;
    m_bRAlt = false;
    m_VKey = 0;

    // only a press
    if (m_keyDownLastFrame + KEY_DELAY_TIME > timeGetTime())
      return;

    // qualifying keys
    if (KeyDown(DIK_LCONTROL) || KeyDown(DIK_RCONTROL))
      m_bCtrl = true;
    if (KeyDown(DIK_LSHIFT) || KeyDown(DIK_RSHIFT))
      m_bShift = true;
    if (KeyDown(DIK_LALT) || KeyDown(DIK_RALT))
      m_bAlt = true;
    if (KeyDown(DIK_RALT))
      m_bRAlt = true;
    if (KeyDown(DIK_CAPSLOCK))
      m_bShift = !m_bShift;

    // TODO: Should add more keys in this routine
    if (KeyDown(DIK_A)) m_cAscii = 'A';
    else if (KeyDown(DIK_B)) m_cAscii = 'B';
    else if (KeyDown(DIK_C)) m_cAscii = 'C';
    else if (KeyDown(DIK_D)) m_cAscii = 'D';
    else if (KeyDown(DIK_E)) m_cAscii = 'E';
    else if (KeyDown(DIK_F)) m_cAscii = 'F';
    else if (KeyDown(DIK_G)) m_cAscii = 'G';
    else if (KeyDown(DIK_H)) m_cAscii = 'H';
    else if (KeyDown(DIK_I)) m_cAscii = 'I';
    else if (KeyDown(DIK_J)) m_cAscii = 'J';
    else if (KeyDown(DIK_K)) m_cAscii = 'K';
    else if (KeyDown(DIK_L)) m_cAscii = 'L';
    else if (KeyDown(DIK_M)) m_cAscii = 'M';
    else if (KeyDown(DIK_N)) m_cAscii = 'N';
    else if (KeyDown(DIK_O)) m_cAscii = 'O';
    else if (KeyDown(DIK_P)) m_cAscii = 'P';
    else if (KeyDown(DIK_Q)) m_cAscii = 'Q';
    else if (KeyDown(DIK_R)) m_cAscii = 'R';
    else if (KeyDown(DIK_S)) m_cAscii = 'S';
    else if (KeyDown(DIK_T)) m_cAscii = 'T';
    else if (KeyDown(DIK_U)) m_cAscii = 'U';
    else if (KeyDown(DIK_V)) m_cAscii = 'V';
    else if (KeyDown(DIK_W)) m_cAscii = 'W';
    else if (KeyDown(DIK_X)) m_cAscii = 'X';
    else if (KeyDown(DIK_Y)) m_cAscii = 'Y';
    else if (KeyDown(DIK_Z)) m_cAscii = 'Z';
    else if (KeyDown(DIK_1)) { m_VKey = 0x60; m_cAscii = m_bShift ? '!' : '1'; }
    else if (KeyDown(DIK_2)) { m_VKey = 0x61; m_cAscii = m_bShift ? '@' : '2'; }
    else if (KeyDown(DIK_3)) { m_VKey = 0x62; m_cAscii = m_bShift ? '#' : '3'; }
    else if (KeyDown(DIK_4)) { m_VKey = 0x63; m_cAscii = m_bShift ? '$' : '4'; }
    else if (KeyDown(DIK_5)) { m_VKey = 0x64; m_cAscii = m_bShift ? '%' : '5'; }
    else if (KeyDown(DIK_6)) { m_VKey = 0x65; m_cAscii = m_bShift ? '^' : '6'; }
    else if (KeyDown(DIK_7)) { m_VKey = 0x66; m_cAscii = m_bShift ? '&' : '7'; }
    else if (KeyDown(DIK_8)) { m_VKey = 0x67; m_cAscii = m_bShift ? '*' : '8'; }
    else if (KeyDown(DIK_9)) { m_VKey = 0x68; m_cAscii = m_bShift ? '(' : '9'; }
    else if (KeyDown(DIK_0)) { m_VKey = 0x69; m_cAscii = m_bShift ? ')' : '0'; }
    else if (KeyDown(DIK_RETURN)) m_VKey = 0x0D;
    else if (KeyDown(DIK_NUMPADENTER)) m_VKey = 0x6C;
    else if (KeyDown(DIK_ESCAPE)) m_VKey = 0x1B;
    else if (KeyDown(DIK_TAB)) m_VKey = 0x09;
    else if (KeyDown(DIK_SPACE)) m_VKey = 0x20;
    else if (KeyDown(DIK_LEFT)) m_VKey = 0x25; // alias for DIK_LEFTARROW
    else if (KeyDown(DIK_RIGHT)) m_VKey = 0x27; // alias for DIK_RIGHTARROW
    else if (KeyDown(DIK_UP)) m_VKey = 0x26; // alias for DIK_UPARROW
    else if (KeyDown(DIK_DOWN)) m_VKey = 0x28; // alias for DIK_DOWNARROW
    else if (KeyDown(DIK_INSERT)) m_VKey = 0x2D;
    else if (KeyDown(DIK_DELETE)) m_VKey = 0x2E;
    else if (KeyDown(DIK_HOME)) m_VKey = 0x24;
    else if (KeyDown(DIK_END)) m_VKey = 0x23;
    else if (KeyDown(DIK_F1)) m_VKey = 0x70;
    else if (KeyDown(DIK_F2)) m_VKey = 0x71;
    else if (KeyDown(DIK_F3)) m_VKey = 0x72;
    else if (KeyDown(DIK_F4)) m_VKey = 0x73;
    else if (KeyDown(DIK_F5)) m_VKey = 0x74;
    else if (KeyDown(DIK_F6)) m_VKey = 0x75;
    else if (KeyDown(DIK_F7)) m_VKey = 0x76;
    else if (KeyDown(DIK_F8)) m_VKey = 0x77;
    else if (KeyDown(DIK_F9)) m_VKey = 0x78;
    else if (KeyDown(DIK_F10)) m_VKey = 0x79;
    else if (KeyDown(DIK_F11)) m_VKey = 0x7a;
    else if (KeyDown(DIK_F12)) m_VKey = 0x7b;

    else if (KeyDown(DIK_NUMPAD0)) m_VKey = 0x60;
    else if (KeyDown(DIK_NUMPAD1)) m_VKey = 0x61;
    else if (KeyDown(DIK_NUMPAD2)) m_VKey = 0x62;
    else if (KeyDown(DIK_NUMPAD3)) m_VKey = 0x63;
    else if (KeyDown(DIK_NUMPAD4)) m_VKey = 0x64;
    else if (KeyDown(DIK_NUMPAD5)) m_VKey = 0x65;
    else if (KeyDown(DIK_NUMPAD6)) m_VKey = 0x66;
    else if (KeyDown(DIK_NUMPAD7)) m_VKey = 0x67;
    else if (KeyDown(DIK_NUMPAD8)) m_VKey = 0x68;
    else if (KeyDown(DIK_NUMPAD9)) m_VKey = 0x69;
    else if (KeyDown(DIK_NUMPADSTAR)) m_VKey = 0x6a; // alias for DIK_MULTIPLY
    else if (KeyDown(DIK_NUMPADPLUS)) m_VKey = 0x6b; // alias for DIK_ADD
    else if (KeyDown(DIK_NUMPADMINUS)) m_VKey = 0x6d; // alias for DIK_SUBTRACT
    else if (KeyDown(DIK_NUMPADPERIOD)) m_VKey = 0x6e; // alias for DIK_DECIMAL
    else if (KeyDown(DIK_NUMPADSLASH)) m_VKey = 0x6f;
    else if (KeyDown(DIK_PGUP)) m_VKey = 0x21; // alias for DIK_PRIOR
    else if (KeyDown(DIK_PGDN)) m_VKey = 0x22; // alias for DIK_NEXT
    else if (KeyDown(DIK_SYSRQ)) m_VKey = 0x2a;
    else if (KeyDown(DIK_BACKSPACE)) m_VKey = 0x08; // alias for DIK_BACK
    else if (KeyDown(DIK_APPS)) m_VKey = 0x5d;
    else if (KeyDown(DIK_PAUSE)) m_VKey = 0x13;
    else if (KeyDown(DIK_CAPSLOCK)) m_VKey = 0x20; // ???
    else if (KeyDown(DIK_NUMLOCK)) m_VKey = 0x90;
    else if (KeyDown(DIK_SCROLL)) m_VKey = 0x91;
    else if (KeyDown(DIK_SEMICOLON)) { m_VKey = 0xba; m_cAscii = m_bShift ? ':' : ';'; } // DIK_COLON handled, too
    else if (KeyDown(DIK_EQUALS)) { m_VKey = 0xbb; m_cAscii = m_bShift ? '+' : '='; }
    else if (KeyDown(DIK_COMMA)) { m_VKey = 0xbc; m_cAscii = m_bShift ? '<' : ','; }
    else if (KeyDown(DIK_MINUS)) { m_VKey = 0xbd; m_cAscii = m_bShift ? '_' : '-'; } // DIK_UNDERLINE handled, too
    else if (KeyDown(DIK_PERIOD)) { m_VKey = 0xbe; m_cAscii = m_bShift ? '>' : '.'; }
    else if (KeyDown(DIK_DIVIDE)) { m_VKey = 0xbf; m_cAscii = m_bShift ? '?' : '/'; }
    else if (KeyDown(DIK_GRAVE)) { m_VKey = 0xc0; m_cAscii = m_bShift ? '~' : '`'; }
    else if (KeyDown(DIK_LBRACKET)) { m_VKey = 0xeb; m_cAscii = m_bShift ? '{' : '['; }
    else if (KeyDown(DIK_BACKSLASH)) { m_VKey = 0xec; m_cAscii = m_bShift ? '|' : '\\'; }
    else if (KeyDown(DIK_RBRACKET)) { m_VKey = 0xed; m_cAscii = m_bShift ? '}' : ']'; }
    else if (KeyDown(DIK_APOSTROPHE)) { m_VKey = 0xee; m_cAscii = m_bShift ? '"' : '\''; }
    else if (KeyDown(DIK_LSHIFT)) m_VKey = 0xa0;
    else if (KeyDown(DIK_RSHIFT)) m_VKey = 0xa1;
    else if (KeyDown(DIK_LCONTROL)) m_VKey = 0xa2;
    else if (KeyDown(DIK_RCONTROL)) m_VKey = 0xa3;
    else if (KeyDown(DIK_LALT)) m_VKey = 0xa4; // alias for DIK_LMENU
    else if (KeyDown(DIK_RALT)) m_VKey = 0xa5; // alias for DIK_RMENU
    else if (KeyDown(DIK_LWIN)) m_VKey = 0x5b;
    else if (KeyDown(DIK_RWIN)) m_VKey = 0x5c;

    // FIXME TESTME
    else if (KeyDown(DIK_F13)) m_VKey = 0x7c;
    else if (KeyDown(DIK_F14)) m_VKey = 0x7d;
    else if (KeyDown(DIK_F15)) m_VKey = 0x7e;

    else if (KeyDown(DIK_NEXTTRACK)) m_VKey = 0xb0; 
    else if (KeyDown(DIK_PREVTRACK)) m_VKey = 0xb1; 
    else if (KeyDown(DIK_MEDIASTOP)) m_VKey = 0xb2; 
    else if (KeyDown(DIK_PLAYPAUSE)) m_VKey = 0xb3; 
    else if (KeyDown(DIK_MAIL)) m_VKey = 0xb4; 
    else if (KeyDown(DIK_MEDIASELECT)) m_VKey = 0xb5; 
    else if (KeyDown(DIK_MYCOMPUTER)) m_VKey = 0xb6; // App1
    else if (KeyDown(DIK_CALCULATOR)) m_VKey = 0xb7; // App2
    else if (KeyDown(DIK_MUTE)) m_VKey = 0xad; 
    else if (KeyDown(DIK_STOP)) m_VKey = 0xa9; // browser stop
    else if (KeyDown(DIK_VOLUMEDOWN)) m_VKey = 0xae; 
    else if (KeyDown(DIK_VOLUMEUP)) m_VKey = 0xaf; 
    else if (KeyDown(DIK_WEBBACK)) m_VKey = 0xa6; 
    else if (KeyDown(DIK_WEBFAVORITES)) m_VKey = 0xab; 
    else if (KeyDown(DIK_WEBFORWARD)) m_VKey = 0xa7; 
    else if (KeyDown(DIK_WEBHOME)) m_VKey = 0xac; 
    else if (KeyDown(DIK_WEBREFRESH)) m_VKey = 0xa8; 
    else if (KeyDown(DIK_WEBSEARCH)) m_VKey = 0xaa; 
    else if (KeyDown(DIK_WEBSTOP)) m_VKey = 0xa9; 
    else if (KeyDown(DIK_OEM_102)) m_VKey = 0xe2; // 0x56 

    // TODO: Add more keys (eg NEC ones)

//    else if (KeyDown(DIK_UNDERLINE)) { m_VKey = 0xbd; m_cAscii = '_'; }
//    else if (KeyDown(DIK_COLON)) { m_VKey = 0xba; m_cAscii = ':'; }

    // TODO: Not sure if this is correct - can we have vkey's that correspond
    //       with the ascii value?
    if (m_cAscii && !m_VKey)
    {
      m_VKey = m_cAscii;
      if (!m_bShift)
        m_cAscii = tolower(m_cAscii);
    }

    // reset frame count
    if (m_VKey)
      m_keyDownLastFrame = timeGetTime();
  }  
}

void CLowLevelKeyboard::Acquire()
{
  if (m_keyboard)
    m_keyboard->Acquire();
}

#endif
