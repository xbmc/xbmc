#include "../include.h"
#include "../Key.h"
#include "SDLKeyboard.h"

#ifdef HAS_SDL

#define FRAME_DELAY 5

CLowLevelKeyboard::CLowLevelKeyboard()
{
  Reset();
}

void CLowLevelKeyboard::Initialize(HWND hWnd)
{
  SDL_EnableUNICODE(1);
  SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
}

void CLowLevelKeyboard::Reset()
{
  m_bShift = false;
  m_bCtrl = false;
  m_bAlt = false;
  m_bRAlt = false;
  m_cAscii = '\0';
  m_wUnicode = '\0';
  m_VKey = 0;
}

void CLowLevelKeyboard::Update(SDL_Event& m_keyEvent)
{
  if (m_keyEvent.type == SDL_KEYDOWN)
  {
    m_cAscii = 0;
    m_VKey = 0;

    m_wUnicode = m_keyEvent.key.keysym.unicode;

    m_bCtrl = (m_keyEvent.key.keysym.mod & KMOD_CTRL) != 0;
    m_bShift = (m_keyEvent.key.keysym.mod & KMOD_SHIFT) != 0;
    m_bAlt = (m_keyEvent.key.keysym.mod & KMOD_ALT) != 0;
    m_bRAlt = (m_keyEvent.key.keysym.mod & KMOD_RALT) != 0;

    if ((m_keyEvent.key.keysym.unicode >= 'A' && m_keyEvent.key.keysym.unicode <= 'Z') ||
        (m_keyEvent.key.keysym.unicode >= 'a' && m_keyEvent.key.keysym.unicode <= 'z'))
    {
      m_cAscii = (char)m_keyEvent.key.keysym.unicode;
      m_VKey = toupper(m_cAscii);
    }
    else if (m_keyEvent.key.keysym.unicode >= '0' && m_keyEvent.key.keysym.unicode <= '9')
    {
      m_cAscii = (char)m_keyEvent.key.keysym.unicode;
      m_VKey = 0x60 + m_cAscii - '0'; // xbox keyboard routine appears to return 0x60->69 (unverified). Ideally this "fixing"
                                      // should be done in xbox routine, not in the sdl/directinput routines.
                                      // we should just be using the unicode/ascii value in all routines (perhaps with some
                                      // headroom for modifier keys?)
    }
    else
    {
      // see comment above about the weird use of m_VKey here...
      if (m_keyEvent.key.keysym.unicode == ')') { m_VKey = 0x60; m_cAscii = ')'; }
      else if (m_keyEvent.key.keysym.unicode == '!') { m_VKey = 0x61; m_cAscii = '!'; }
      else if (m_keyEvent.key.keysym.unicode == '@') { m_VKey = 0x62; m_cAscii = '@'; }
      else if (m_keyEvent.key.keysym.unicode == '#') { m_VKey = 0x63; m_cAscii = '#'; }
      else if (m_keyEvent.key.keysym.unicode == '$') { m_VKey = 0x64; m_cAscii = '$'; }
      else if (m_keyEvent.key.keysym.unicode == '%') { m_VKey = 0x65; m_cAscii = '%'; }
      else if (m_keyEvent.key.keysym.unicode == '^') { m_VKey = 0x66; m_cAscii = '^'; }
      else if (m_keyEvent.key.keysym.unicode == '&') { m_VKey = 0x67; m_cAscii = '&'; }
      else if (m_keyEvent.key.keysym.unicode == '*') { m_VKey = 0x68; m_cAscii = '*'; }
      else if (m_keyEvent.key.keysym.unicode == '(') { m_VKey = 0x69; m_cAscii = '('; }
      else if (m_keyEvent.key.keysym.unicode == ':') { m_VKey = 0xba; m_cAscii = ':'; }
      else if (m_keyEvent.key.keysym.unicode == ';') { m_VKey = 0xba; m_cAscii = ';'; }
      else if (m_keyEvent.key.keysym.unicode == '=') { m_VKey = 0xbb; m_cAscii = '='; }
      else if (m_keyEvent.key.keysym.unicode == '+') { m_VKey = 0xbb; m_cAscii = '+'; }
      else if (m_keyEvent.key.keysym.unicode == '<') { m_VKey = 0xbc; m_cAscii = '<'; }
      else if (m_keyEvent.key.keysym.unicode == ',') { m_VKey = 0xbc; m_cAscii = ','; }
      else if (m_keyEvent.key.keysym.unicode == '-') { m_VKey = 0xbd; m_cAscii = '-'; }
      else if (m_keyEvent.key.keysym.unicode == '_') { m_VKey = 0xbd; m_cAscii = '_'; }
      else if (m_keyEvent.key.keysym.unicode == '>') { m_VKey = 0xbe; m_cAscii = '>'; }
      else if (m_keyEvent.key.keysym.unicode == '.') { m_VKey = 0xbe; m_cAscii = '.'; }
      else if (m_keyEvent.key.keysym.unicode == '?') { m_VKey = 0xbf; m_cAscii = '?'; } // 0xbf is OEM 2 Why is it assigned here?
      else if (m_keyEvent.key.keysym.unicode == '/') { m_VKey = 0xbf; m_cAscii = '/'; }
      else if (m_keyEvent.key.keysym.unicode == '~') { m_VKey = 0xc0; m_cAscii = '~'; }
      else if (m_keyEvent.key.keysym.unicode == '`') { m_VKey = 0xc0; m_cAscii = '`'; }
      else if (m_keyEvent.key.keysym.unicode == '{') { m_VKey = 0xeb; m_cAscii = '{'; }
      else if (m_keyEvent.key.keysym.unicode == '[') { m_VKey = 0xeb; m_cAscii = '['; } // 0xeb is not defined by MS. Why is it assigned here?
      else if (m_keyEvent.key.keysym.unicode == '|') { m_VKey = 0xec; m_cAscii = '|'; }
      else if (m_keyEvent.key.keysym.unicode == '\\') { m_VKey = 0xec; m_cAscii = '\\'; }
      else if (m_keyEvent.key.keysym.unicode == '}') { m_VKey = 0xed; m_cAscii = '}'; }
      else if (m_keyEvent.key.keysym.unicode == ']') { m_VKey = 0xed; m_cAscii = ']'; } // 0xed is not defined by MS. Why is it assigned here?
      else if (m_keyEvent.key.keysym.unicode == '"') { m_VKey = 0xee; m_cAscii = '"'; }
      else if (m_keyEvent.key.keysym.unicode == '\'') { m_VKey = 0xee; m_cAscii = '\''; }
      else if (m_keyEvent.key.keysym.sym == SDLK_BACKSPACE) m_VKey = 0x08;
      else if (m_keyEvent.key.keysym.sym == SDLK_TAB) m_VKey = 0x09;
      else if (m_keyEvent.key.keysym.sym == SDLK_RETURN) m_VKey = 0x0d;
      else if (m_keyEvent.key.keysym.sym == SDLK_ESCAPE) m_VKey = 0x1b;
      else if (m_keyEvent.key.keysym.sym == SDLK_SPACE) m_VKey = 0x20;
      else if (m_keyEvent.key.keysym.sym == SDLK_KP0) m_VKey = 0x60;
      else if (m_keyEvent.key.keysym.sym == SDLK_KP1) m_VKey = 0x61;
      else if (m_keyEvent.key.keysym.sym == SDLK_KP2) m_VKey = 0x62;
      else if (m_keyEvent.key.keysym.sym == SDLK_KP3) m_VKey = 0x63;
      else if (m_keyEvent.key.keysym.sym == SDLK_KP4) m_VKey = 0x64;
      else if (m_keyEvent.key.keysym.sym == SDLK_KP5) m_VKey = 0x65;
      else if (m_keyEvent.key.keysym.sym == SDLK_KP6) m_VKey = 0x66;
      else if (m_keyEvent.key.keysym.sym == SDLK_KP7) m_VKey = 0x67;
      else if (m_keyEvent.key.keysym.sym == SDLK_KP8) m_VKey = 0x68;
      else if (m_keyEvent.key.keysym.sym == SDLK_KP9) m_VKey = 0x69;
      else if (m_keyEvent.key.keysym.sym == SDLK_KP_ENTER) m_VKey = 0x6C;
      else if (m_keyEvent.key.keysym.sym == SDLK_UP) m_VKey = 0x26;
      else if (m_keyEvent.key.keysym.sym == SDLK_DOWN) m_VKey = 0x28;
      else if (m_keyEvent.key.keysym.sym == SDLK_LEFT) m_VKey = 0x25;
      else if (m_keyEvent.key.keysym.sym == SDLK_RIGHT) m_VKey = 0x27;
      else if (m_keyEvent.key.keysym.sym == SDLK_INSERT) m_VKey = 0x2D;
      else if (m_keyEvent.key.keysym.sym == SDLK_DELETE) m_VKey = 0x2E;
      else if (m_keyEvent.key.keysym.sym == SDLK_HOME) m_VKey = 0x24;
      else if (m_keyEvent.key.keysym.sym == SDLK_END) m_VKey = 0x23;
      else if (m_keyEvent.key.keysym.sym == SDLK_F1) m_VKey = 0x70;
      else if (m_keyEvent.key.keysym.sym == SDLK_F2) m_VKey = 0x71;
      else if (m_keyEvent.key.keysym.sym == SDLK_F3) m_VKey = 0x72;
      else if (m_keyEvent.key.keysym.sym == SDLK_F4) m_VKey = 0x73;
      else if (m_keyEvent.key.keysym.sym == SDLK_F5) m_VKey = 0x74;
      else if (m_keyEvent.key.keysym.sym == SDLK_F6) m_VKey = 0x75;
      else if (m_keyEvent.key.keysym.sym == SDLK_F7) m_VKey = 0x76;
      else if (m_keyEvent.key.keysym.sym == SDLK_F8) m_VKey = 0x77;
      else if (m_keyEvent.key.keysym.sym == SDLK_F9) m_VKey = 0x78;
      else if (m_keyEvent.key.keysym.sym == SDLK_F10) m_VKey = 0x79;
      else if (m_keyEvent.key.keysym.sym == SDLK_F11) m_VKey = 0x7a;
      else if (m_keyEvent.key.keysym.sym == SDLK_F12) m_VKey = 0x7b;
      else if (m_keyEvent.key.keysym.sym == SDLK_KP_PERIOD) m_VKey = 0x6e;
      else if (m_keyEvent.key.keysym.sym == SDLK_KP_MULTIPLY) m_VKey = 0x6a;
      else if (m_keyEvent.key.keysym.sym == SDLK_KP_MINUS) m_VKey = 0x6d;
      else if (m_keyEvent.key.keysym.sym == SDLK_KP_PLUS) m_VKey = 0x6b;
      else if (m_keyEvent.key.keysym.sym == SDLK_KP_DIVIDE) m_VKey = 0x6f;
      else if (m_keyEvent.key.keysym.sym == SDLK_PAGEUP) m_VKey = 0x21;
      else if (m_keyEvent.key.keysym.sym == SDLK_PAGEDOWN) m_VKey = 0x22;
      else if ((m_keyEvent.key.keysym.sym == SDLK_PRINT) || (m_keyEvent.key.keysym.scancode==111) ) m_VKey = 0x2a;
      else if (m_keyEvent.key.keysym.sym == SDLK_LSUPER) m_VKey = 0x5b;
      else if (m_keyEvent.key.keysym.sym == SDLK_RSUPER) m_VKey = 0x5c;
      else if (m_keyEvent.key.keysym.scancode==117) m_VKey = 0x5d; // right click

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

      else if (m_keyEvent.key.keysym.mod & KMOD_LSHIFT) m_VKey = 0xa0;
      else if (m_keyEvent.key.keysym.mod & KMOD_RSHIFT) m_VKey = 0xa1;
      else if (m_keyEvent.key.keysym.mod & KMOD_LALT) m_VKey = 0xa4;
      else if (m_keyEvent.key.keysym.mod & KMOD_RALT) m_VKey = 0xa5;
      else if (m_keyEvent.key.keysym.mod & KMOD_LCTRL) m_VKey = 0xa2;
      else if (m_keyEvent.key.keysym.mod & KMOD_RCTRL) m_VKey = 0xa3;
      else if (m_keyEvent.key.keysym.unicode > 32 && m_keyEvent.key.keysym.unicode < 128)
        // only TRUE ASCII! (Otherwise XBMC crashes! No unicode not even latin 1!)
        m_cAscii = (char)(m_keyEvent.key.keysym.unicode & 0xff);
      else CLog::Log(LOGDEBUG, "SDLKeyboard found something unknown (unicode <> printable ASCII): scancode: %d, sym: %d, unicode: %d, modifier: %d ", m_keyEvent.key.keysym.scancode, m_keyEvent.key.keysym.sym, m_keyEvent.key.keysym.unicode, m_keyEvent.key.keysym.mod);
    }
  }
/*
  Initial support for the
  PS3 Controller using the SDL Joystick API

  /\ - 12
  O  - 13
  X  - 14
  [] - 15
  
  top - 4
  right - 5
  bottom - 6
  left - 7
  
  select - 0
  start - 3

  left analog click - 1
  right analog click - 2

  left shoulder - 10
  right shoulder - 11

  left analog shoulder - 8
  right analog shoulder - 9

  Controller reports 28 axes, one for each pressure sensitive button with values ranging 
  from [-32k, 32k].

*/
  else
  {
    Reset();
  }
}

#endif
