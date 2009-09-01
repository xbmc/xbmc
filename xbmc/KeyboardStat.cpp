//
// C++ Implementation: CKeyboard
//
// Description:
//
//
// Author: Team XBMC <>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//

// Comment OUT, if not really debugging!!!:
//#define DEBUG_KEYBOARD_GETCHAR

#include "stdafx.h"
#include "KeyboardStat.h"
#include "KeyboardLayoutConfiguration.h"
#include "XBMC_events.h"

#if defined(HAS_GLX)
#include <X11/XKBlib.h>
#endif

CKeyboardStat g_Keyboard;

const unsigned int CKeyboardStat::key_hold_time = 500;  // time in ms before we declare it held

#define XBMC_NLK_CAPS 0x01
#define XBMC_NLK_NUM  0x02

/* Global keystate information */
static Uint8  XBMC_KeyState[XBMCK_LAST];
static XBMCMod XBMC_ModState;
static const char *keynames[XBMCK_LAST];	/* Array of keycode names */

static Uint8 XBMC_NoLockKeys;

struct {
  int firsttime;    /* if we check against the delay or repeat value */
  int delay;        /* the delay before we start repeating */
  int interval;     /* the delay between key repeat events */
  Uint32 timestamp; /* the time the first keydown event occurred */

  XBMC_Event evt;    /* the event we are supposed to repeat */
} XBMC_KeyRepeat;

int XBMC_EnableKeyRepeat(int delay, int interval)
{
  if ( (delay < 0) || (interval < 0) ) {
    return(-1);
  }
  XBMC_KeyRepeat.firsttime = 0;
  XBMC_KeyRepeat.delay = delay;
  XBMC_KeyRepeat.interval = interval;
  XBMC_KeyRepeat.timestamp = 0;
  return(0);
}

CKeyboardStat::CKeyboardStat()
{
  /* Initialize the tables */
  XBMC_ModState = XBMCKMOD_NONE;
  memset((void*)keynames, 0, sizeof(keynames));
  memset(XBMC_KeyState, 0, sizeof(XBMC_KeyState));

  XBMC_EnableKeyRepeat(0, 0);

  XBMC_NoLockKeys = 0;

  /* Fill in the blanks in keynames */
  keynames[XBMCK_BACKSPACE] = "backspace";
  keynames[XBMCK_TAB] = "tab";
  keynames[XBMCK_CLEAR] = "clear";
  keynames[XBMCK_RETURN] = "return";
  keynames[XBMCK_PAUSE] = "pause";
  keynames[XBMCK_ESCAPE] = "escape";
  keynames[XBMCK_SPACE] = "space";
  keynames[XBMCK_EXCLAIM]  = "!";
  keynames[XBMCK_QUOTEDBL]  = "\"";
  keynames[XBMCK_HASH]  = "#";
  keynames[XBMCK_DOLLAR]  = "$";
  keynames[XBMCK_AMPERSAND]  = "&";
  keynames[XBMCK_QUOTE] = "'";
  keynames[XBMCK_LEFTPAREN] = "(";
  keynames[XBMCK_RIGHTPAREN] = ")";
  keynames[XBMCK_ASTERISK] = "*";
  keynames[XBMCK_PLUS] = "+";
  keynames[XBMCK_COMMA] = ",";
  keynames[XBMCK_MINUS] = "-";
  keynames[XBMCK_PERIOD] = ".";
  keynames[XBMCK_SLASH] = "/";
  keynames[XBMCK_0] = "0";
  keynames[XBMCK_1] = "1";
  keynames[XBMCK_2] = "2";
  keynames[XBMCK_3] = "3";
  keynames[XBMCK_4] = "4";
  keynames[XBMCK_5] = "5";
  keynames[XBMCK_6] = "6";
  keynames[XBMCK_7] = "7";
  keynames[XBMCK_8] = "8";
  keynames[XBMCK_9] = "9";
  keynames[XBMCK_COLON] = ":";
  keynames[XBMCK_SEMICOLON] = ";";
  keynames[XBMCK_LESS] = "<";
  keynames[XBMCK_EQUALS] = "=";
  keynames[XBMCK_GREATER] = ">";
  keynames[XBMCK_QUESTION] = "?";
  keynames[XBMCK_AT] = "@";
  keynames[XBMCK_LEFTBRACKET] = "[";
  keynames[XBMCK_BACKSLASH] = "\\";
  keynames[XBMCK_RIGHTBRACKET] = "]";
  keynames[XBMCK_CARET] = "^";
  keynames[XBMCK_UNDERSCORE] = "_";
  keynames[XBMCK_BACKQUOTE] = "`";
  keynames[XBMCK_a] = "a";
  keynames[XBMCK_b] = "b";
  keynames[XBMCK_c] = "c";
  keynames[XBMCK_d] = "d";
  keynames[XBMCK_e] = "e";
  keynames[XBMCK_f] = "f";
  keynames[XBMCK_g] = "g";
  keynames[XBMCK_h] = "h";
  keynames[XBMCK_i] = "i";
  keynames[XBMCK_j] = "j";
  keynames[XBMCK_k] = "k";
  keynames[XBMCK_l] = "l";
  keynames[XBMCK_m] = "m";
  keynames[XBMCK_n] = "n";
  keynames[XBMCK_o] = "o";
  keynames[XBMCK_p] = "p";
  keynames[XBMCK_q] = "q";
  keynames[XBMCK_r] = "r";
  keynames[XBMCK_s] = "s";
  keynames[XBMCK_t] = "t";
  keynames[XBMCK_u] = "u";
  keynames[XBMCK_v] = "v";
  keynames[XBMCK_w] = "w";
  keynames[XBMCK_x] = "x";
  keynames[XBMCK_y] = "y";
  keynames[XBMCK_z] = "z";
  keynames[XBMCK_DELETE] = "delete";

  keynames[XBMCK_WORLD_0] = "world 0";
  keynames[XBMCK_WORLD_1] = "world 1";
  keynames[XBMCK_WORLD_2] = "world 2";
  keynames[XBMCK_WORLD_3] = "world 3";
  keynames[XBMCK_WORLD_4] = "world 4";
  keynames[XBMCK_WORLD_5] = "world 5";
  keynames[XBMCK_WORLD_6] = "world 6";
  keynames[XBMCK_WORLD_7] = "world 7";
  keynames[XBMCK_WORLD_8] = "world 8";
  keynames[XBMCK_WORLD_9] = "world 9";
  keynames[XBMCK_WORLD_10] = "world 10";
  keynames[XBMCK_WORLD_11] = "world 11";
  keynames[XBMCK_WORLD_12] = "world 12";
  keynames[XBMCK_WORLD_13] = "world 13";
  keynames[XBMCK_WORLD_14] = "world 14";
  keynames[XBMCK_WORLD_15] = "world 15";
  keynames[XBMCK_WORLD_16] = "world 16";
  keynames[XBMCK_WORLD_17] = "world 17";
  keynames[XBMCK_WORLD_18] = "world 18";
  keynames[XBMCK_WORLD_19] = "world 19";
  keynames[XBMCK_WORLD_20] = "world 20";
  keynames[XBMCK_WORLD_21] = "world 21";
  keynames[XBMCK_WORLD_22] = "world 22";
  keynames[XBMCK_WORLD_23] = "world 23";
  keynames[XBMCK_WORLD_24] = "world 24";
  keynames[XBMCK_WORLD_25] = "world 25";
  keynames[XBMCK_WORLD_26] = "world 26";
  keynames[XBMCK_WORLD_27] = "world 27";
  keynames[XBMCK_WORLD_28] = "world 28";
  keynames[XBMCK_WORLD_29] = "world 29";
  keynames[XBMCK_WORLD_30] = "world 30";
  keynames[XBMCK_WORLD_31] = "world 31";
  keynames[XBMCK_WORLD_32] = "world 32";
  keynames[XBMCK_WORLD_33] = "world 33";
  keynames[XBMCK_WORLD_34] = "world 34";
  keynames[XBMCK_WORLD_35] = "world 35";
  keynames[XBMCK_WORLD_36] = "world 36";
  keynames[XBMCK_WORLD_37] = "world 37";
  keynames[XBMCK_WORLD_38] = "world 38";
  keynames[XBMCK_WORLD_39] = "world 39";
  keynames[XBMCK_WORLD_40] = "world 40";
  keynames[XBMCK_WORLD_41] = "world 41";
  keynames[XBMCK_WORLD_42] = "world 42";
  keynames[XBMCK_WORLD_43] = "world 43";
  keynames[XBMCK_WORLD_44] = "world 44";
  keynames[XBMCK_WORLD_45] = "world 45";
  keynames[XBMCK_WORLD_46] = "world 46";
  keynames[XBMCK_WORLD_47] = "world 47";
  keynames[XBMCK_WORLD_48] = "world 48";
  keynames[XBMCK_WORLD_49] = "world 49";
  keynames[XBMCK_WORLD_50] = "world 50";
  keynames[XBMCK_WORLD_51] = "world 51";
  keynames[XBMCK_WORLD_52] = "world 52";
  keynames[XBMCK_WORLD_53] = "world 53";
  keynames[XBMCK_WORLD_54] = "world 54";
  keynames[XBMCK_WORLD_55] = "world 55";
  keynames[XBMCK_WORLD_56] = "world 56";
    keynames[XBMCK_WORLD_57] = "world 57";
    keynames[XBMCK_WORLD_58] = "world 58";
    keynames[XBMCK_WORLD_59] = "world 59";
    keynames[XBMCK_WORLD_60] = "world 60";
    keynames[XBMCK_WORLD_61] = "world 61";
    keynames[XBMCK_WORLD_62] = "world 62";
    keynames[XBMCK_WORLD_63] = "world 63";
    keynames[XBMCK_WORLD_64] = "world 64";
    keynames[XBMCK_WORLD_65] = "world 65";
    keynames[XBMCK_WORLD_66] = "world 66";
    keynames[XBMCK_WORLD_67] = "world 67";
    keynames[XBMCK_WORLD_68] = "world 68";
    keynames[XBMCK_WORLD_69] = "world 69";
    keynames[XBMCK_WORLD_70] = "world 70";
    keynames[XBMCK_WORLD_71] = "world 71";
    keynames[XBMCK_WORLD_72] = "world 72";
    keynames[XBMCK_WORLD_73] = "world 73";
    keynames[XBMCK_WORLD_74] = "world 74";
    keynames[XBMCK_WORLD_75] = "world 75";
    keynames[XBMCK_WORLD_76] = "world 76";
    keynames[XBMCK_WORLD_77] = "world 77";
    keynames[XBMCK_WORLD_78] = "world 78";
    keynames[XBMCK_WORLD_79] = "world 79";
    keynames[XBMCK_WORLD_80] = "world 80";
    keynames[XBMCK_WORLD_81] = "world 81";
    keynames[XBMCK_WORLD_82] = "world 82";
    keynames[XBMCK_WORLD_83] = "world 83";
    keynames[XBMCK_WORLD_84] = "world 84";
    keynames[XBMCK_WORLD_85] = "world 85";
    keynames[XBMCK_WORLD_86] = "world 86";
    keynames[XBMCK_WORLD_87] = "world 87";
    keynames[XBMCK_WORLD_88] = "world 88";
    keynames[XBMCK_WORLD_89] = "world 89";
    keynames[XBMCK_WORLD_90] = "world 90";
    keynames[XBMCK_WORLD_91] = "world 91";
    keynames[XBMCK_WORLD_92] = "world 92";
    keynames[XBMCK_WORLD_93] = "world 93";
    keynames[XBMCK_WORLD_94] = "world 94";
    keynames[XBMCK_WORLD_95] = "world 95";

    keynames[XBMCK_KP0] = "[0]";
    keynames[XBMCK_KP1] = "[1]";
    keynames[XBMCK_KP2] = "[2]";
    keynames[XBMCK_KP3] = "[3]";
    keynames[XBMCK_KP4] = "[4]";
    keynames[XBMCK_KP5] = "[5]";
    keynames[XBMCK_KP6] = "[6]";
    keynames[XBMCK_KP7] = "[7]";
    keynames[XBMCK_KP8] = "[8]";
    keynames[XBMCK_KP9] = "[9]";
    keynames[XBMCK_KP_PERIOD] = "[.]";
    keynames[XBMCK_KP_DIVIDE] = "[/]";
    keynames[XBMCK_KP_MULTIPLY] = "[*]";
    keynames[XBMCK_KP_MINUS] = "[-]";
    keynames[XBMCK_KP_PLUS] = "[+]";
    keynames[XBMCK_KP_ENTER] = "enter";
    keynames[XBMCK_KP_EQUALS] = "equals";

    keynames[XBMCK_UP] = "up";
    keynames[XBMCK_DOWN] = "down";
    keynames[XBMCK_RIGHT] = "right";
    keynames[XBMCK_LEFT] = "left";
    keynames[XBMCK_DOWN] = "down";
    keynames[XBMCK_INSERT] = "insert";
    keynames[XBMCK_HOME] = "home";
    keynames[XBMCK_END] = "end";
    keynames[XBMCK_PAGEUP] = "page up";
    keynames[XBMCK_PAGEDOWN] = "page down";

    keynames[XBMCK_F1] = "f1";
    keynames[XBMCK_F2] = "f2";
    keynames[XBMCK_F3] = "f3";
    keynames[XBMCK_F4] = "f4";
    keynames[XBMCK_F5] = "f5";
    keynames[XBMCK_F6] = "f6";
    keynames[XBMCK_F7] = "f7";
    keynames[XBMCK_F8] = "f8";
    keynames[XBMCK_F9] = "f9";
    keynames[XBMCK_F10] = "f10";
    keynames[XBMCK_F11] = "f11";
    keynames[XBMCK_F12] = "f12";
    keynames[XBMCK_F13] = "f13";
    keynames[XBMCK_F14] = "f14";
    keynames[XBMCK_F15] = "f15";

    keynames[XBMCK_NUMLOCK] = "numlock";
    keynames[XBMCK_CAPSLOCK] = "caps lock";
    keynames[XBMCK_SCROLLOCK] = "scroll lock";
    keynames[XBMCK_RSHIFT] = "right shift";
    keynames[XBMCK_LSHIFT] = "left shift";
    keynames[XBMCK_RCTRL] = "right ctrl";
    keynames[XBMCK_LCTRL] = "left ctrl";
    keynames[XBMCK_RALT] = "right alt";
    keynames[XBMCK_LALT] = "left alt";
    keynames[XBMCK_RMETA] = "right meta";
    keynames[XBMCK_LMETA] = "left meta";
    keynames[XBMCK_LSUPER] = "left super";	/* "Windows" keys */
    keynames[XBMCK_RSUPER] = "right super";	
    keynames[XBMCK_MODE] = "alt gr";
    keynames[XBMCK_COMPOSE] = "compose";

    keynames[XBMCK_HELP] = "help";
    keynames[XBMCK_PRINT] = "print screen";
    keynames[XBMCK_SYSREQ] = "sys req";
    keynames[XBMCK_BREAK] = "break";
    keynames[XBMCK_MENU] = "menu";
    keynames[XBMCK_POWER] = "power";
    keynames[XBMCK_EURO] = "euro";
    keynames[XBMCK_UNDO] = "undo";

  Reset();
}

CKeyboardStat::~CKeyboardStat()
{
}

void CKeyboardStat::Initialize()
{
#if defined(HAS_GLX)
  Display* dpy = XOpenDisplay(NULL);
  if (!dpy)
    return;

  XkbDescPtr desc;
  char* symbols;

  desc = XkbGetKeyboard(dpy, XkbAllComponentsMask, XkbUseCoreKbd);
  if(!desc)
  {
    XCloseDisplay(dpy);
    return;
  }

  symbols = XGetAtomName(dpy, desc->names->symbols);
  if(symbols)
  {
    CLog::Log(LOGDEBUG, "CLowLevelKeyboard::Initialize - XKb symbols %s", symbols);
    if (strstr(symbols, "(evdev)"))
      m_bEvdev = true;
  }

  XFree(symbols);
  XkbFreeKeyboard(desc, XkbAllComponentsMask, True);
  XCloseDisplay(dpy);
#endif
}

void CKeyboardStat::Reset()
{
  m_bShift = false;
  m_bCtrl = false;
  m_bAlt = false;
  m_bRAlt = false;
  m_cAscii = '\0';
  m_wUnicode = '\0';
  m_VKey = 0;

  ZeroMemory(&XBMC_KeyState, sizeof(XBMC_KeyState));
}

void CKeyboardStat::ResetState()
{
  Reset();
}

unsigned int CKeyboardStat::KeyHeld() const
{
  if (m_keyHoldTime > key_hold_time)
    return m_keyHoldTime - key_hold_time;
  return 0;
}




int CKeyboardStat::HandleEvent(XBMC_Event& newEvent)
{
  int repeatable;
  Uint16 modstate;

  /* Set up the keysym */
  XBMC_keysym *keysym = &newEvent.key.keysym;
  modstate = (Uint16)XBMC_ModState;

  repeatable = 0;

  int state;
  if(newEvent.type == XBMC_KEYDOWN)
    state = XBMC_PRESSED;
  else if(newEvent.type == XBMC_KEYUP)
    state = XBMC_RELEASED;
  else
    return 0;

  if ( state == XBMC_PRESSED ) 
  {
    keysym->mod = (XBMCMod)modstate;
    switch (keysym->sym) 
    {
      case XBMCK_UNKNOWN:
        break;
      case XBMCK_NUMLOCK:
        modstate ^= XBMCKMOD_NUM;
        if ( XBMC_NoLockKeys & XBMC_NLK_NUM )
          break;
        if ( ! (modstate&XBMCKMOD_NUM) )
          state = XBMC_RELEASED;
        keysym->mod = (XBMCMod)modstate;
        break;
      case XBMCK_CAPSLOCK:
        modstate ^= XBMCKMOD_CAPS;
        if ( XBMC_NoLockKeys & XBMC_NLK_CAPS )
          break;
        if ( ! (modstate&XBMCKMOD_CAPS) )
          state = XBMC_RELEASED;
        keysym->mod = (XBMCMod)modstate;
        break;
      case XBMCK_LCTRL:
        modstate |= XBMCKMOD_LCTRL;
        break;
      case XBMCK_RCTRL:
        modstate |= XBMCKMOD_RCTRL;
        break;
      case XBMCK_LSHIFT:
        modstate |= XBMCKMOD_LSHIFT;
        break;
      case XBMCK_RSHIFT:
        modstate |= XBMCKMOD_RSHIFT;
        break;
      case XBMCK_LALT:
        modstate |= XBMCKMOD_LALT;
        break;
      case XBMCK_RALT:
        modstate |= XBMCKMOD_RALT;
        break;
      case XBMCK_LMETA:
        modstate |= XBMCKMOD_LMETA;
        break;
      case XBMCK_RMETA:
        modstate |= XBMCKMOD_RMETA;
        break;
      case XBMCK_MODE:
        modstate |= XBMCKMOD_MODE;
        break;
      default:
        repeatable = 1;
        break;
    }
  } 
  else 
  {
    switch (keysym->sym) 
    {
      case XBMCK_UNKNOWN:
        break;
      case XBMCK_NUMLOCK:
        if ( XBMC_NoLockKeys & XBMC_NLK_NUM )
          break;
        /* Only send keydown events */
        return(0);
      case XBMCK_CAPSLOCK:
        if ( XBMC_NoLockKeys & XBMC_NLK_CAPS )
          break;
        /* Only send keydown events */
        return(0);
      case XBMCK_LCTRL:
        modstate &= ~XBMCKMOD_LCTRL;
        break;
      case XBMCK_RCTRL:
        modstate &= ~XBMCKMOD_RCTRL;
        break;
      case XBMCK_LSHIFT:
        modstate &= ~XBMCKMOD_LSHIFT;
        break;
      case XBMCK_RSHIFT:
        modstate &= ~XBMCKMOD_RSHIFT;
        break;
      case XBMCK_LALT:
        modstate &= ~XBMCKMOD_LALT;
        break;
      case XBMCK_RALT:
        modstate &= ~XBMCKMOD_RALT;
        break;
      case XBMCK_LMETA:
        modstate &= ~XBMCKMOD_LMETA;
        break;
      case XBMCK_RMETA:
        modstate &= ~XBMCKMOD_RMETA;
        break;
      case XBMCK_MODE:
        modstate &= ~XBMCKMOD_MODE;
        break;
      default:
        break;
    }
    keysym->mod = (XBMCMod)modstate;
  }

  /*
  * jk 991215 - Added
  */
  if(state == XBMC_RELEASED)
  if ( XBMC_KeyRepeat.timestamp &&
    XBMC_KeyRepeat.evt.key.keysym.sym == keysym->sym ) 
  {
    XBMC_KeyRepeat.timestamp = 0;
  }

 /*
  switch (state) {
    case XBMC_PRESSED:
      event.type = XBMC_KEYDOWN;
      break;
    case XBMC_RELEASED:
      event.type = XBMC_KEYUP;
      
      if ( XBMC_KeyRepeat.timestamp &&
        XBMC_KeyRepeat.evt.key.keysym.sym == keysym->sym ) 
      {
          XBMC_KeyRepeat.timestamp = 0;
      }
      break;
    default:
      
      return(0);
  }
  */

  if ( keysym->sym != XBMCK_UNKNOWN ) 
  {
    /* Drop events that don't change state */
    if ( XBMC_KeyState[keysym->sym] == state ) 
    {
      return(0);
    }

    /* Update internal keyboard state */
    XBMC_ModState = (XBMCMod)modstate;
    XBMC_KeyState[keysym->sym] = state;
  }
  
  newEvent.key.state = state;
  Update(newEvent);
  
  return 0;
}

void CKeyboardStat::Update(XBMC_Event& m_keyEvent)
{
  if (m_keyEvent.type == XBMC_KEYDOWN)
  {
    DWORD now = timeGetTime();
    if (memcmp(&m_lastKey, &m_keyEvent, sizeof(XBMC_Event)) == 0)
      m_keyHoldTime += now - m_lastKeyTime;
    m_lastKey = m_keyEvent;
    m_lastKeyTime = now;

    m_cAscii = 0;
    m_VKey = 0;

    m_wUnicode = m_keyEvent.key.keysym.unicode;

    m_bCtrl = (m_keyEvent.key.keysym.mod & XBMCKMOD_CTRL) != 0;
    m_bShift = (m_keyEvent.key.keysym.mod & XBMCKMOD_SHIFT) != 0;
    m_bAlt = (m_keyEvent.key.keysym.mod & XBMCKMOD_ALT) != 0;
    m_bRAlt = (m_keyEvent.key.keysym.mod & XBMCKMOD_RALT) != 0;

    CLog::Log(LOGDEBUG, "SDLKeyboard: scancode: %d, sym: %d, unicode: %d, modifier: %x", m_keyEvent.key.keysym.scancode, m_keyEvent.key.keysym.sym, m_keyEvent.key.keysym.unicode, m_keyEvent.key.keysym.mod);

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
      if (!m_VKey && !m_cAscii) // split block due to ms compiler complaints about nested code depth
      {
        // OSX defines unicode values for non-printing keys which breaks the key parser, set m_wUnicode
        if (m_keyEvent.key.keysym.sym == XBMCK_BACKSPACE) { m_VKey = 0x08; m_wUnicode=0x08; }
        else if (m_keyEvent.key.keysym.sym == XBMCK_TAB) m_VKey = 0x09;
        else if (m_keyEvent.key.keysym.sym == XBMCK_RETURN) m_VKey = 0x0d;
        else if (m_keyEvent.key.keysym.sym == XBMCK_ESCAPE) m_VKey = 0x1b;
        else if (m_keyEvent.key.keysym.sym == XBMCK_SPACE) m_VKey = 0x20;
        else if (m_keyEvent.key.keysym.sym == XBMCK_MENU) m_VKey = 0x5d;
        else if (m_keyEvent.key.keysym.sym == XBMCK_KP0) m_VKey = 0x60;
        else if (m_keyEvent.key.keysym.sym == XBMCK_KP1) m_VKey = 0x61;
        else if (m_keyEvent.key.keysym.sym == XBMCK_KP2) m_VKey = 0x62;
        else if (m_keyEvent.key.keysym.sym == XBMCK_KP3) m_VKey = 0x63;
        else if (m_keyEvent.key.keysym.sym == XBMCK_KP4) m_VKey = 0x64;
        else if (m_keyEvent.key.keysym.sym == XBMCK_KP5) m_VKey = 0x65;
        else if (m_keyEvent.key.keysym.sym == XBMCK_KP6) m_VKey = 0x66;
        else if (m_keyEvent.key.keysym.sym == XBMCK_KP7) m_VKey = 0x67;
        else if (m_keyEvent.key.keysym.sym == XBMCK_KP8) m_VKey = 0x68;
        else if (m_keyEvent.key.keysym.sym == XBMCK_KP9) m_VKey = 0x69;
        else if (m_keyEvent.key.keysym.sym == XBMCK_KP_ENTER) m_VKey = 0x6C;
        else if (m_keyEvent.key.keysym.sym == XBMCK_UP)  { m_VKey = 0x26; m_wUnicode = 0; }
        else if (m_keyEvent.key.keysym.sym == XBMCK_DOWN) { m_VKey = 0x28; m_wUnicode = 0; }
        else if (m_keyEvent.key.keysym.sym == XBMCK_LEFT) { m_VKey = 0x25; m_wUnicode = 0; }
        else if (m_keyEvent.key.keysym.sym == XBMCK_RIGHT) { m_VKey = 0x27; m_wUnicode = 0; }
        else if (m_keyEvent.key.keysym.sym == XBMCK_INSERT) m_VKey = 0x2D;
        else if (m_keyEvent.key.keysym.sym == XBMCK_DELETE) { m_VKey = 0x2E; m_wUnicode = 0; }
        else if (m_keyEvent.key.keysym.sym == XBMCK_HOME) m_VKey = 0x24;
        else if (m_keyEvent.key.keysym.sym == XBMCK_END) m_VKey = 0x23;
        else if (m_keyEvent.key.keysym.sym == XBMCK_F1) m_VKey = 0x70;
        else if (m_keyEvent.key.keysym.sym == XBMCK_F2) m_VKey = 0x71;
        else if (m_keyEvent.key.keysym.sym == XBMCK_F3) m_VKey = 0x72;
        else if (m_keyEvent.key.keysym.sym == XBMCK_F4) m_VKey = 0x73;
        else if (m_keyEvent.key.keysym.sym == XBMCK_F5) m_VKey = 0x74;
        else if (m_keyEvent.key.keysym.sym == XBMCK_F6) m_VKey = 0x75;
        else if (m_keyEvent.key.keysym.sym == XBMCK_F7) m_VKey = 0x76;
        else if (m_keyEvent.key.keysym.sym == XBMCK_F8) m_VKey = 0x77;
        else if (m_keyEvent.key.keysym.sym == XBMCK_F9) m_VKey = 0x78;
        else if (m_keyEvent.key.keysym.sym == XBMCK_F10) m_VKey = 0x79;
        else if (m_keyEvent.key.keysym.sym == XBMCK_F11) m_VKey = 0x7a;
        else if (m_keyEvent.key.keysym.sym == XBMCK_F12) m_VKey = 0x7b;
        else if (m_keyEvent.key.keysym.sym == XBMCK_KP_PERIOD) m_VKey = 0x6e;
        else if (m_keyEvent.key.keysym.sym == XBMCK_KP_MULTIPLY) m_VKey = 0x6a;
        else if (m_keyEvent.key.keysym.sym == XBMCK_KP_MINUS) m_VKey = 0x6d;
        else if (m_keyEvent.key.keysym.sym == XBMCK_KP_PLUS) m_VKey = 0x6b;
        else if (m_keyEvent.key.keysym.sym == XBMCK_KP_DIVIDE) m_VKey = 0x6f;
        else if (m_keyEvent.key.keysym.sym == XBMCK_PAGEUP) m_VKey = 0x21;
        else if (m_keyEvent.key.keysym.sym == XBMCK_PAGEDOWN) m_VKey = 0x22;
        else if ((m_keyEvent.key.keysym.sym == XBMCK_PRINT) || (m_keyEvent.key.keysym.scancode==111) || (m_keyEvent.key.keysym.scancode==105) ) m_VKey = 0x2a;
        else if (m_keyEvent.key.keysym.sym == XBMCK_LSHIFT) m_VKey = 0xa0;
        else if (m_keyEvent.key.keysym.sym == XBMCK_RSHIFT) m_VKey = 0xa1;
      }

      if (!m_VKey && !m_cAscii && m_bEvdev)
      {
        // based on the evdev mapped scancodes in /user/share/X11/xkb/keycodes
        if (m_keyEvent.key.keysym.scancode == 121) m_VKey = 0xad; // Volume mute
        else if (m_keyEvent.key.keysym.scancode == 122) m_VKey = 0xae; // Volume down
        else if (m_keyEvent.key.keysym.scancode == 123) m_VKey = 0xaf; // Volume up
        else if (m_keyEvent.key.keysym.scancode == 135) m_VKey = 0x5d; // Right click
        else if (m_keyEvent.key.keysym.scancode == 136) m_VKey = 0xb2; // Stop
        else if (m_keyEvent.key.keysym.scancode == 138) m_VKey = 0x49; // Info
        else if (m_keyEvent.key.keysym.scancode == 166) m_VKey = 0xa6; // Browser back
        else if (m_keyEvent.key.keysym.scancode == 167) m_VKey = 0xa7; // Browser forward
        else if (m_keyEvent.key.keysym.scancode == 171) m_VKey = 0xb0; // Next track
        else if (m_keyEvent.key.keysym.scancode == 172) m_VKey = 0xb3; // Play_Pause
        else if (m_keyEvent.key.keysym.scancode == 173) m_VKey = 0xb1; // Prev track
        else if (m_keyEvent.key.keysym.scancode == 174) m_VKey = 0xb2; // Stop
        else if (m_keyEvent.key.keysym.scancode == 176) m_VKey = 0x52; // Rewind
        else if (m_keyEvent.key.keysym.scancode == 180) m_VKey = 0xac; // Browser home
        else if (m_keyEvent.key.keysym.scancode == 181) m_VKey = 0xa8; // Browser refresh
        else if (m_keyEvent.key.keysym.scancode == 214) m_VKey = 0x1B; // Close
        else if (m_keyEvent.key.keysym.scancode == 215) m_VKey = 0xb3; // Play_Pause
        else if (m_keyEvent.key.keysym.scancode == 216) m_VKey = 0x46; // Forward
        //else if (m_keyEvent.key.keysym.scancode == 167) m_VKey = 0xb3; // Record
      }
      if (!m_VKey && !m_cAscii && !m_bEvdev)
      {
        // following scancode infos are
        // 1. from ubuntu keyboard shortcut (hex) -> predefined
        // 2. from unix tool xev and my keyboards (decimal)
        // m_VKey infos from CharProbe tool
        // Can we do the same for XBoxKeyboard and DirectInputKeyboard? Can we access the scancode of them? By the way how does SDL do it? I can't find it. (Automagically? But how exactly?)
        // Some pairs of scancode and virtual keys are only known half

        // special "keys" above F1 till F12 on my MS natural keyboard mapped to virtual keys "F13" till "F24"):
        if (m_keyEvent.key.keysym.scancode == 0xf5) m_VKey = 0x7c; // F13 Launch help browser
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
        else if (m_keyEvent.key.keysym.scancode == 231) m_VKey = 0xa8; // Browser refresh
        //      else if (m_keyEvent.key.keysym.scancode == ) m_VKey = 0xa9; // Browser stop
        else if (m_keyEvent.key.keysym.scancode == 122) m_VKey = 0xaa; // Browser search
        else if (m_keyEvent.key.keysym.scancode == 0xe5) m_VKey = 0xaa; // Browser search
        else if (m_keyEvent.key.keysym.scancode == 230) m_VKey = 0xab; // Browser favorites
        else if (m_keyEvent.key.keysym.scancode == 130) m_VKey = 0xac; // Browser home
        else if (m_keyEvent.key.keysym.scancode == 0xa0) m_VKey = 0xad; // Volume mute
        else if (m_keyEvent.key.keysym.scancode == 0xae) m_VKey = 0xae; // Volume down
        else if (m_keyEvent.key.keysym.scancode == 0xb0) m_VKey = 0xaf; // Volume up
        else if (m_keyEvent.key.keysym.scancode == 0x99) m_VKey = 0xb0; // Next track
        else if (m_keyEvent.key.keysym.scancode == 0x90) m_VKey = 0xb1; // Prev track
        else if (m_keyEvent.key.keysym.scancode == 0xa4) m_VKey = 0xb2; // Stop
        else if (m_keyEvent.key.keysym.scancode == 0xa2) m_VKey = 0xb3; // Play_Pause
        else if (m_keyEvent.key.keysym.scancode == 0xec) m_VKey = 0xb4; // Launch mail
        else if (m_keyEvent.key.keysym.scancode == 129) m_VKey = 0xb5; // Launch media_select
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
        else if (m_keyEvent.key.keysym.scancode==117) m_VKey = 0x5d; // right click
      }
      if (!m_VKey && !m_cAscii)
      {
        if (m_keyEvent.key.keysym.mod & XBMCKMOD_LSHIFT) m_VKey = 0xa0;
        else if (m_keyEvent.key.keysym.mod & XBMCKMOD_RSHIFT) m_VKey = 0xa1;
        else if (m_keyEvent.key.keysym.mod & XBMCKMOD_LALT) m_VKey = 0xa4;
        else if (m_keyEvent.key.keysym.mod & XBMCKMOD_RALT) m_VKey = 0xa5;
        else if (m_keyEvent.key.keysym.mod & XBMCKMOD_LCTRL) m_VKey = 0xa2;
        else if (m_keyEvent.key.keysym.mod & XBMCKMOD_RCTRL) m_VKey = 0xa3;
        else if (m_keyEvent.key.keysym.unicode > 32 && m_keyEvent.key.keysym.unicode < 128)
          // only TRUE ASCII! (Otherwise XBMC crashes! No unicode not even latin 1!)
          m_cAscii = (char)(m_keyEvent.key.keysym.unicode & 0xff);
      }
    }
  }
  else
  { // key up event
    Reset();
    memset(&m_lastKey, 0, sizeof(m_lastKey));
    m_keyHoldTime = 0;
  }
}

char CKeyboardStat::GetAscii()
{
  char lowLevelAscii = m_cAscii;
  int translatedAscii = GetUnicode();

#ifdef DEBUG_KEYBOARD_GETCHAR
  CLog::Log(LOGDEBUG, "low level ascii: %c ", lowLevelAscii);
  CLog::Log(LOGDEBUG, "low level ascii code: %d ", lowLevelAscii);
  CLog::Log(LOGDEBUG, "result char: %c ", translatedAscii);
  CLog::Log(LOGDEBUG, "result char code: %d ", translatedAscii);
  CLog::Log(LOGDEBUG, "ralt is pressed bool: %d ", GetRAlt());
  CLog::Log(LOGDEBUG, "shift is pressed bool: %d ", GetShift());
#endif

  if (translatedAscii >= 0 && translatedAscii < 128) // only TRUE ASCII! Otherwise XBMC crashes! No unicode not even latin 1!
    return translatedAscii; // mapping to ASCII is supported only if the result is TRUE ASCII
  else
    return lowLevelAscii; // old style
}

WCHAR CKeyboardStat::GetUnicode()
{
  // More specific mappings, i.e. with scancodes and/or with one or even more modifiers,
  // must be handled first/prioritized over less specific mappings! Why?
  // Example: an us keyboard has: "]" on one key, the german keyboard has "+" on the same key,
  // additionally the german keyboard has "~" on the same key, but the "~"
  // can only be reached with the special modifier "AltGr" (right alt).
  // See http://en.wikipedia.org/wiki/Keyboard_layout.
  // If "+" is handled first, the key is already consumed and "~" can never be reached.
  // The least specific mappings, e.g. "regardless modifiers" should be done at last/least prioritized.

  WCHAR lowLevelUnicode = m_wUnicode;
  BYTE key = m_VKey;

#ifdef DEBUG_KEYBOARD_GETCHAR
  CLog::Log(LOGDEBUG, "low level unicode char: %c ", lowLevelUnicode);
  CLog::Log(LOGDEBUG, "low level unicode code: %d ", lowLevelUnicode);
  CLog::Log(LOGDEBUG, "low level vkey: %d ", key);
  CLog::Log(LOGDEBUG, "ralt is pressed bool: %d ", GetRAlt());
  CLog::Log(LOGDEBUG, "shift is pressed bool: %d ", GetShift());
#endif

  if (GetRAlt())
  {
    if (g_keyboardLayoutConfiguration.containsDeriveXbmcCharFromVkeyWithRalt(key))
    {
      WCHAR resultUnicode = g_keyboardLayoutConfiguration.valueOfDeriveXbmcCharFromVkeyWithRalt(key);
#ifdef DEBUG_KEYBOARD_GETCHAR
      CLog::Log(LOGDEBUG, "derived with ralt to code: %d ", resultUnicode);
#endif
      return resultUnicode;
    }
  }

  if (GetShift())
  {
    if (g_keyboardLayoutConfiguration.containsDeriveXbmcCharFromVkeyWithShift(key))
    {
      WCHAR resultUnicode = g_keyboardLayoutConfiguration.valueOfDeriveXbmcCharFromVkeyWithShift(key);
#ifdef DEBUG_KEYBOARD_GETCHAR
      CLog::Log(LOGDEBUG, "derived with shift to code: %d ", resultUnicode);
#endif
      return resultUnicode;
    }
  }

  if (g_keyboardLayoutConfiguration.containsDeriveXbmcCharFromVkeyRegardlessModifiers(key))
  {
    WCHAR resultUnicode = g_keyboardLayoutConfiguration.valueOfDeriveXbmcCharFromVkeyRegardlessModifiers(key);
#ifdef DEBUG_KEYBOARD_GETCHAR
    CLog::Log(LOGDEBUG, "derived to code: %d ", resultUnicode);
#endif
    return resultUnicode;
  }

  if (GetRAlt())
  {
    if (g_keyboardLayoutConfiguration.containsChangeXbmcCharWithRalt(lowLevelUnicode))
    {
      WCHAR resultUnicode = g_keyboardLayoutConfiguration.valueOfChangeXbmcCharWithRalt(lowLevelUnicode);
#ifdef DEBUG_KEYBOARD_GETCHAR
      CLog::Log(LOGDEBUG, "changed char with ralt to code: %d ", resultUnicode);
#endif
      return resultUnicode;
    };
  }

  if (g_keyboardLayoutConfiguration.containsChangeXbmcCharRegardlessModifiers(lowLevelUnicode))
  {
    WCHAR resultUnicode = g_keyboardLayoutConfiguration.valueOfChangeXbmcCharRegardlessModifiers(lowLevelUnicode);
#ifdef DEBUG_KEYBOARD_GETCHAR
    CLog::Log(LOGDEBUG, "changed char to code: %d ", resultUnicode);
#endif
    return resultUnicode;
  };

  return lowLevelUnicode;
}
