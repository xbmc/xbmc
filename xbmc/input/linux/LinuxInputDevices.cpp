/*
 *  Portions copied from DirectFB:
 *      Copyright (C) 2001-2009  The world wide DirectFB Open Source Community (directfb.org)
 *      Copyright (C) 2000-2004  Convergence (integrated media) GmbH
 *      All rights reserved.
 *      Written by Denis Oliver Kropp <dok@directfb.org>,
 *      Andreas Hundt <andi@fischlustig.de>,
 *      Sven Neumann <neo@directfb.org>,
 *      Ville Syrjälä <syrjala@sci.fi> and
 *      Claudio Ciccani <klan@users.sf.net>.
 *
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#include "system.h"
#if defined(HAS_LINUX_EVENTS)

#if defined(HAS_LIBAMCODEC)
#include "utils/AMLUtils.h"
#endif

#include <linux/version.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
typedef unsigned long kernel_ulong_t;
#define BITS_PER_LONG    (sizeof(long)*8)
#endif

#include <linux/input.h>

#ifndef EV_CNT
#define EV_CNT (EV_MAX+1)
#define KEY_CNT (KEY_MAX+1)
#define REL_CNT (REL_MAX+1)
#define ABS_CNT (ABS_MAX+1)
#define LED_CNT (LED_MAX+1)
#endif

/* compat defines for older kernel like 2.4.x */
#ifndef EV_SYN
#define EV_SYN			0x00
#define SYN_REPORT              0
#define SYN_CONFIG              1
#define ABS_TOOL_WIDTH		0x1c
#define BTN_TOOL_DOUBLETAP	0x14d
#define BTN_TOOL_TRIPLETAP	0x14e
#endif

#ifndef EVIOCGLED
#define EVIOCGLED(len) _IOC(_IOC_READ, 'E', 0x19, len)
#endif

#ifndef EVIOCGRAB
#define EVIOCGRAB _IOW('E', 0x90, int)
#endif

#ifdef STANDALONE
#define XBMC_BUTTON_LEFT    1
#define XBMC_BUTTON_MIDDLE  2
#define XBMC_BUTTON_RIGHT 3
#define XBMC_BUTTON_WHEELUP 4
#define XBMC_BUTTON_WHEELDOWN 5
#endif

#include <linux/keyboard.h>
#include <linux/kd.h>

#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <stdio.h>

#include "guilib/GraphicContext.h"
#include "input/XBMC_keysym.h"
#include "LinuxInputDevices.h"
#include "input/MouseStat.h"
#include "utils/log.h"

#ifndef BITS_PER_LONG
#define BITS_PER_LONG        (sizeof(long) * 8)
#endif
#define NBITS(x)             ((((x)-1)/BITS_PER_LONG)+1)
#define OFF(x)               ((x)%BITS_PER_LONG)
#define BIT(x)               (1UL<<OFF(x))
#define LONG(x)              ((x)/BITS_PER_LONG)
#undef test_bit
#define test_bit(bit, array) ((array[LONG(bit)] >> OFF(bit)) & 1)

#define MAX_LINUX_INPUT_DEVICES 16

typedef struct {
  unsigned short Key;
  XBMCKey xbmcKey;
} KeyMap;

static const
KeyMap keyMap[] = {
  { KEY_ESC           , XBMCK_ESCAPE      },
  { KEY_1             , XBMCK_1           },
  { KEY_2             , XBMCK_2           },
  { KEY_3             , XBMCK_3           },
  { KEY_4             , XBMCK_4           },
  { KEY_5             , XBMCK_5           },
  { KEY_6             , XBMCK_6           },
  { KEY_7             , XBMCK_7           },
  { KEY_8             , XBMCK_8           },
  { KEY_9             , XBMCK_9           },
  { KEY_0             , XBMCK_0           },
  { KEY_MINUS         , XBMCK_MINUS       },
  { KEY_EQUAL         , XBMCK_EQUALS      },
  { KEY_BACKSPACE     , XBMCK_BACKSPACE   },
  { KEY_TAB           , XBMCK_TAB         },
  { KEY_Q             , XBMCK_q           },
  { KEY_W             , XBMCK_w           },
  { KEY_E             , XBMCK_e           },
  { KEY_R             , XBMCK_r           },
  { KEY_T             , XBMCK_t           },
  { KEY_Y             , XBMCK_y           },
  { KEY_U             , XBMCK_u           },
  { KEY_I             , XBMCK_i           },
  { KEY_O             , XBMCK_o           },
  { KEY_P             , XBMCK_p           },
  { KEY_LEFTBRACE     , XBMCK_LEFTBRACKET },
  { KEY_RIGHTBRACE    , XBMCK_RIGHTBRACKET},
  { KEY_ENTER         , XBMCK_RETURN      },
  { KEY_LEFTCTRL      , XBMCK_LCTRL       },
  { KEY_A             , XBMCK_a           },
  { KEY_S             , XBMCK_s           },
  { KEY_D             , XBMCK_d           },
  { KEY_F             , XBMCK_f           },
  { KEY_G             , XBMCK_g           },
  { KEY_H             , XBMCK_h           },
  { KEY_J             , XBMCK_j           },
  { KEY_K             , XBMCK_k           },
  { KEY_L             , XBMCK_l           },
  { KEY_SEMICOLON     , XBMCK_SEMICOLON   },
  { KEY_APOSTROPHE    , XBMCK_QUOTE       },
  { KEY_GRAVE         , XBMCK_BACKQUOTE   },
  { KEY_LEFTSHIFT     , XBMCK_LSHIFT      },
  { KEY_BACKSLASH     , XBMCK_BACKSLASH   },
  { KEY_Z             , XBMCK_z           },
  { KEY_X             , XBMCK_x           },
  { KEY_C             , XBMCK_c           },
  { KEY_V             , XBMCK_v           },
  { KEY_B             , XBMCK_b           },
  { KEY_N             , XBMCK_n           },
  { KEY_M             , XBMCK_m           },
  { KEY_COMMA         , XBMCK_COMMA       },
  { KEY_DOT           , XBMCK_PERIOD      },
  { KEY_SLASH         , XBMCK_SLASH       },
  { KEY_RIGHTSHIFT    , XBMCK_RSHIFT      },
  { KEY_KPASTERISK    , XBMCK_KP_MULTIPLY },
  { KEY_LEFTALT       , XBMCK_LALT        },
  { KEY_SPACE         , XBMCK_SPACE       },
  { KEY_CAPSLOCK      , XBMCK_CAPSLOCK    },
  { KEY_F1            , XBMCK_F1          },
  { KEY_F2            , XBMCK_F2          },
  { KEY_F3            , XBMCK_F3          },
  { KEY_F4            , XBMCK_F4          },
  { KEY_F5            , XBMCK_F5          },
  { KEY_F6            , XBMCK_F6          },
  { KEY_F7            , XBMCK_F7          },
  { KEY_F8            , XBMCK_F8          },
  { KEY_F9            , XBMCK_F9          },
  { KEY_F10           , XBMCK_F10         },
  { KEY_NUMLOCK       , XBMCK_NUMLOCK     },
  { KEY_SCROLLLOCK    , XBMCK_SCROLLOCK   },
  { KEY_KP7           , XBMCK_KP7         },
  { KEY_KP8           , XBMCK_KP8         },
  { KEY_KP9           , XBMCK_KP9         },
  { KEY_KPMINUS       , XBMCK_KP_MINUS    },
  { KEY_KP4           , XBMCK_KP4         },
  { KEY_KP5           , XBMCK_KP5         },
  { KEY_KP6           , XBMCK_KP6         },
  { KEY_KPPLUS        , XBMCK_KP_PLUS     },
  { KEY_KP1           , XBMCK_KP1         },
  { KEY_KP2           , XBMCK_KP2         },
  { KEY_KP3           , XBMCK_KP3         },
  { KEY_KP0           , XBMCK_KP0         },
  { KEY_KPDOT         , XBMCK_KP_PERIOD   },
  { 84                , XBMCK_BACKSLASH   },
  { 85                , XBMCK_F13         },
  { 86                , XBMCK_LESS        },
  { KEY_F11           , XBMCK_F11         },
  { KEY_F12           , XBMCK_F12         },
  { 89                , XBMCK_F14         },
  { 90                , XBMCK_F15         },
  { KEY_KPENTER       , XBMCK_KP_ENTER    },
  { KEY_RIGHTCTRL     , XBMCK_RCTRL       },
  { KEY_KPSLASH       , XBMCK_KP_DIVIDE   },
  { KEY_SYSRQ         , XBMCK_PRINT       },
  { KEY_RIGHTALT      , XBMCK_MODE        },
  { KEY_HOME          , XBMCK_HOME        },
  { KEY_UP            , XBMCK_UP          },
  { KEY_PAGEUP        , XBMCK_PAGEUP      },
  { KEY_LEFT          , XBMCK_LEFT        },
  { KEY_RIGHT         , XBMCK_RIGHT       },
  { KEY_END           , XBMCK_END         },
  { KEY_DOWN          , XBMCK_DOWN        },
  { KEY_PAGEDOWN      , XBMCK_PAGEDOWN    },
  { KEY_INSERT        , XBMCK_INSERT      },
  { KEY_DELETE        , XBMCK_DELETE      },
  { KEY_MUTE          , XBMCK_VOLUME_MUTE },
  { KEY_VOLUMEDOWN    , XBMCK_VOLUME_DOWN },
  { KEY_VOLUMEUP      , XBMCK_VOLUME_UP   },
  { KEY_POWER         , XBMCK_POWER       },
  { KEY_KPEQUAL       , XBMCK_KP_EQUALS   },
  { KEY_PAUSE         , XBMCK_PAUSE       },
  { KEY_PAUSECD       , XBMCK_PAUSE       },
  { KEY_LEFTMETA      , XBMCK_LMETA       },
  { KEY_RIGHTMETA     , XBMCK_RMETA       },
  { KEY_COMPOSE       , XBMCK_LSUPER      },
  { KEY_STOP          , XBMCK_MEDIA_STOP  },
  { KEY_HELP          , XBMCK_HELP        },
  { KEY_CLOSECD       , XBMCK_EJECT       },
  { KEY_EJECTCD       , XBMCK_EJECT       },
  { KEY_EJECTCLOSECD  , XBMCK_EJECT       },
  { KEY_NEXTSONG      , XBMCK_MEDIA_NEXT_TRACK},
  { KEY_PLAYPAUSE     , XBMCK_MEDIA_PLAY_PAUSE},
  { KEY_PREVIOUSSONG  , XBMCK_MEDIA_PREV_TRACK},
  { KEY_STOPCD        , XBMCK_MEDIA_STOP  },
  { KEY_RECORD        , XBMCK_RECORD      },
  { KEY_REWIND        , XBMCK_REWIND      },
  { KEY_PHONE         , XBMCK_PHONE       },
  { KEY_REFRESH       , XBMCK_SHUFFLE     },
  { KEY_SCROLLUP      , XBMCK_PAGEUP      },
  { KEY_SCROLLDOWN    , XBMCK_PAGEDOWN    },
  { KEY_PLAY          , XBMCK_PLAY        },
  { KEY_PLAYCD        , XBMCK_PLAY        },
  { KEY_FASTFORWARD   , XBMCK_FASTFORWARD },
  { KEY_PRINT         , XBMCK_PRINT       },
  { KEY_QUESTION      , XBMCK_HELP        },
  { KEY_BACK          , XBMCK_BACKSPACE   },
  // The Little Black Box Remote Additions
  { 384               , XBMCK_LEFT        }, // Red
  { 378               , XBMCK_RIGHT       }, // Green
  { 381               , XBMCK_UP          }, // Yellow
  { 366               , XBMCK_DOWN        }, // Blue
  // Rii i7 Home button / wetek openelec remote (code 172)
  { KEY_HOMEPAGE      , XBMCK_HOME        },
};

typedef enum
{
  LI_DEVICE_NONE     = 0,
  LI_DEVICE_MOUSE    = 1,
  LI_DEVICE_JOYSTICK = 2,
  LI_DEVICE_KEYBOARD = 4,
  LI_DEVICE_REMOTE   = 8
} LinuxInputDeviceType;

typedef enum
{
  LI_CAPS_KEYS    = 1,
  LI_CAPS_BUTTONS = 2,
  LI_CAPS_AXES    = 4,
} LinuxInputCapsType;

static char remoteStatus = 0xFF; // paired, battery OK

CLinuxInputDevice::CLinuxInputDevice(const std::string& fileName, int index):
  m_fileName(fileName)
{
  m_fd = -1;
  m_vt_fd = -1;
  m_hasLeds = false;
  m_ledState[0] = false;
  m_ledState[1] = false;
  m_ledState[2] = false;
  m_mouseX = 0;
  m_mouseY = 0;
  m_deviceIndex = index;
  m_keyMods = XBMCKMOD_NONE;
  m_lastKeyMods = XBMCKMOD_NONE;
  strcpy(m_deviceName, "");
  m_deviceType = 0;
  m_devicePreferredId = 0;
  m_deviceCaps = 0;
  m_deviceMinKeyCode = 0;
  m_deviceMaxKeyCode = 0;
  m_deviceMaxAxis = 0;
  m_bUnplugged = false;

  Open();
}

CLinuxInputDevice::~CLinuxInputDevice()
{
  Close();
}

/*
 * Translates a Linux input keycode into an XBMC keycode.
 */
XBMCKey CLinuxInputDevice::TranslateKey(unsigned short code)
{
  for (size_t index = 0; index < sizeof(keyMap) / sizeof(KeyMap); index++)
  {
    if (code == keyMap[index].Key)
      return keyMap[index].xbmcKey;
  }

  return XBMCK_UNKNOWN;
}

int CLinuxInputDevice::KeyboardGetSymbol(unsigned short value)
{
  unsigned char type = KTYP(value);
  unsigned char index = KVAL(value);

  switch (type)
  {
  case KT_FN:
    if (index < 15)
      return XBMCK_F1 + index;
    break;
  case KT_LETTER:
  case KT_LATIN:
    switch (index)
    {
    case 0x1c:
      return XBMCK_PRINT;
    case 0x7f:
      return XBMCK_BACKSPACE;
    case 0xa4:
      return XBMCK_EURO; /* euro currency sign */
    default:
      return index;
    }
    break;

/*
  case KT_DEAD:
    switch (value)
    {
    case K_DGRAVE:
      return DIKS_DEAD_GRAVE;

    case K_DACUTE:
      return DIKS_DEAD_ACUTE;

    case K_DCIRCM:
      return DIKS_DEAD_CIRCUMFLEX;

    case K_DTILDE:
      return DIKS_DEAD_TILDE;

    case K_DDIERE:
      return DIKS_DEAD_DIAERESIS;

    case K_DCEDIL:
      return DIKS_DEAD_CEDILLA;

    default:
      break;
    }
    break;

  case KT_PAD:
    if (index <= 9 && level != DIKSI_BASE)
      return (DFBInputDeviceKeySymbol) (DIKS_0 + index);
    break;
*/
  }

  return XBMCK_UNKNOWN;
}

unsigned short CLinuxInputDevice::KeyboardReadValue(unsigned char table, unsigned char index)
{
  struct kbentry entry;

  entry.kb_table = table;
  entry.kb_index = index;
  entry.kb_value = 0;

  if (ioctl(m_vt_fd, KDGKBENT, &entry))
  {
    CLog::Log(LOGWARNING, "CLinuxInputDevice::KeyboardReadValue: KDGKBENT (table: %d, index: %d) "
        "failed!\n", table, index);
    return 0;
  }

  return entry.kb_value;
}

XBMCMod CLinuxInputDevice::UpdateModifiers(XBMC_Event& devt)
{
  XBMCMod modifier = XBMCKMOD_NONE;
  switch (devt.key.keysym.sym)
  {
    case XBMCK_LSHIFT: modifier = XBMCKMOD_LSHIFT; break;
    case XBMCK_RSHIFT: modifier = XBMCKMOD_RSHIFT; break;
    case XBMCK_LCTRL: modifier = XBMCKMOD_LCTRL; break;
    case XBMCK_RCTRL: modifier = XBMCKMOD_RCTRL; break;
    case XBMCK_LALT: modifier = XBMCKMOD_LALT; break;
    case XBMCK_RALT: modifier = XBMCKMOD_RALT; break;
    case XBMCK_LMETA: modifier = XBMCKMOD_LMETA; break;
    case XBMCK_RMETA: modifier = XBMCKMOD_RMETA; break;
    default: break;
  }

  if (devt.key.type == XBMC_KEYDOWN)
  {
    m_keyMods |= modifier;
  }
  else
  {
    m_keyMods &= ~modifier;
  }

  if (devt.key.type == XBMC_KEYDOWN)
  {
    modifier = XBMCKMOD_NONE;
    switch (devt.key.keysym.sym)
    {
      case XBMCK_NUMLOCK: modifier = XBMCKMOD_NUM; break;
      case XBMCK_CAPSLOCK: modifier = XBMCKMOD_CAPS; break;
      default: break;
    }

    if (m_keyMods & modifier)
    {
      m_keyMods &= ~modifier;
    }
    else
    {
      m_keyMods |= modifier;
    }
  }

  return (XBMCMod) m_keyMods;
}

/*
 * Translates key and button events.
 */
bool CLinuxInputDevice::KeyEvent(const struct input_event& levt, XBMC_Event& devt)
{
  int code = levt.code;

  /* map touchscreen and smartpad events to button mouse */
  if (code == BTN_TOUCH || code == BTN_TOOL_FINGER)
    code = BTN_MOUSE;

  if ((code >= BTN_MOUSE && code < BTN_JOYSTICK) || code == BTN_TOUCH)
  {
    /* ignore repeat events for buttons */
    if (levt.value == 2)
      return false;

    devt.type = levt.value ? XBMC_MOUSEBUTTONDOWN : XBMC_MOUSEBUTTONUP;
    devt.button.state = levt.value ? XBMC_PRESSED : XBMC_RELEASED;
    devt.button.type = devt.type;
    devt.button.x = m_mouseX;
    devt.button.y = m_mouseY;

    switch (levt.code)
    {
      case BTN_RIGHT:
        devt.button.button = XBMC_BUTTON_RIGHT;
        break;

      case BTN_LEFT:
        devt.button.button = XBMC_BUTTON_LEFT;
        break;

      case BTN_MIDDLE:
        devt.button.button = XBMC_BUTTON_RIGHT;
        break;

      case BTN_FORWARD:
        devt.button.button = XBMC_BUTTON_WHEELDOWN;
        break;

      case BTN_BACK:
        devt.button.button = XBMC_BUTTON_WHEELUP;
        break;

      case BTN_TOUCH:
        devt.button.button = XBMC_BUTTON_LEFT;
        break;

      case BTN_TOOL_DOUBLETAP:
        devt.button.button = XBMC_BUTTON_RIGHT;
        break;

      default:
        CLog::Log(LOGWARNING, "CLinuxInputDevice::KeyEvent: Unknown mouse button code: %d\n", levt.code);
        return false;
    }
  }
  else
  {
    XBMCKey key = TranslateKey(code);

    if (key == XBMCK_UNKNOWN)
    {
      CLog::Log(LOGDEBUG, "CLinuxInputDevice::KeyEvent: TranslateKey returned XBMCK_UNKNOWN from code(%d)", code);
      return false;
    }

    devt.type = levt.value ? XBMC_KEYDOWN : XBMC_KEYUP;
    devt.key.type = devt.type;
    // warning, key.keysym.scancode is unsigned char so 0 - 255 only
    devt.key.keysym.scancode = code;
    devt.key.keysym.sym = key;
    devt.key.keysym.mod = UpdateModifiers(devt);
    devt.key.keysym.unicode = 0;

    KeymapEntry entry;
    entry.code = code;
    if (GetKeymapEntry(entry))
    {
      int keyMapValue;
      if (devt.key.keysym.mod & (XBMCKMOD_SHIFT | XBMCKMOD_CAPS)) keyMapValue = entry.shift;
      else if (devt.key.keysym.mod & XBMCKMOD_ALT) keyMapValue = entry.alt;
      else if (devt.key.keysym.mod & XBMCKMOD_META) keyMapValue = entry.altShift;
      else keyMapValue = entry.base;

      if (keyMapValue != XBMCK_UNKNOWN)
      {
        devt.key.keysym.sym = (XBMCKey) keyMapValue;
        if (keyMapValue > 0 && keyMapValue < 127)
        {
          devt.key.keysym.unicode = devt.key.keysym.sym;
        }
      }
    }
  }

  return true;
}

/*
 * Translates relative axis events.
 */
bool CLinuxInputDevice::RelEvent(const struct input_event& levt, XBMC_Event& devt)
{
  switch (levt.code)
  {
  case REL_X:
    m_mouseX += levt.value;
    devt.motion.xrel = levt.value;
    devt.motion.yrel = 0;
    break;

  case REL_Y:
    m_mouseY += levt.value;
    devt.motion.xrel = 0;
    devt.motion.yrel = levt.value;
    break;

  case REL_Z:
  case REL_WHEEL:
  default:
    CLog::Log(LOGWARNING, "CLinuxInputDevice::RelEvent: Unknown rel event code: %d\n", levt.code);
    return false;
  }

  // limit the mouse to the screen width
  m_mouseX = std::min(g_graphicsContext.GetWidth(), m_mouseX);
  m_mouseX = std::max(0, m_mouseX);

  // limit the mouse to the screen height
  m_mouseY = std::min(g_graphicsContext.GetHeight(), m_mouseY);
  m_mouseY = std::max(0, m_mouseY);


  devt.type = XBMC_MOUSEMOTION;
  devt.motion.type = XBMC_MOUSEMOTION;
  devt.motion.x = m_mouseX;
  devt.motion.y = m_mouseY;
  devt.motion.state = 0;
  devt.motion.which = m_deviceIndex;


  return true;
}

/*
 * Translates absolute axis events.
 */
bool CLinuxInputDevice::AbsEvent(const struct input_event& levt, XBMC_Event& devt)
{
  switch (levt.code)
  {
  case ABS_X:
    m_mouseX = levt.value;
    break;

  case ABS_Y:
    m_mouseY = levt.value;
    break;
  
  case ABS_MISC:
    remoteStatus = levt.value & 0xFF;
    break;

  case ABS_Z:
  default:
    return false;
  }

  devt.type = XBMC_MOUSEMOTION;
  devt.motion.type = XBMC_MOUSEMOTION;
  devt.motion.x = m_mouseX;
  devt.motion.y = m_mouseY;
  devt.motion.state = 0;
  devt.motion.xrel = 0;
  devt.motion.yrel = 0;
  devt.motion.which = m_deviceIndex;

  return true;
}

/*
 * Translates a Linux input event into a DirectFB input event.
 */
bool CLinuxInputDevice::TranslateEvent(const struct input_event& levt,
    XBMC_Event& devt)
{
  switch (levt.type)
  {
  case EV_KEY:
    return KeyEvent(levt, devt);

  case EV_REL:
    if (m_bSkipNonKeyEvents)
    {
      CLog::Log(LOGINFO, "read a relative event which will be ignored (device name %s) (file name %s)", m_deviceName, m_fileName.c_str());
      return false;
    }

    return RelEvent(levt, devt);

  case EV_ABS:
    if (m_bSkipNonKeyEvents)
    {
      CLog::Log(LOGINFO, "read an absolute event which will be ignored (device name %s) (file name %s)", m_deviceName, m_fileName.c_str());
      return false;
    }

    return AbsEvent(levt, devt);

  default:
    ;
  }

  return false;
}

void CLinuxInputDevice::SetLed(int led, int state)
{
  struct input_event levt;

  levt.type = EV_LED;
  levt.code = led;
  levt.value = !!state;

  write(m_fd, &levt, sizeof(levt));
}

/*
 * Input thread reading from device.
 * Generates events on incoming data.
 */
XBMC_Event CLinuxInputDevice::ReadEvent()
{
  int readlen;
  struct input_event levt;

  XBMC_Event devt;

  while (1)
  {
    bzero(&levt, sizeof(levt));

    bzero(&devt, sizeof(devt));
    devt.type = XBMC_NOEVENT;

    if(m_devicePreferredId == LI_DEVICE_NONE)
      return devt;

    readlen = read(m_fd, &levt, sizeof(levt));

    if (readlen <= 0)
    {
      if (errno == ENODEV)
      {
        CLog::Log(LOGINFO,"input device was unplugged %s",m_deviceName);
        m_bUnplugged = true;
      }

      break;
    }

    //printf("read event readlen = %d device name %s m_fileName %s\n", readlen, m_deviceName, m_fileName.c_str());

    // sanity check if we realy read the event
    if(readlen != sizeof(levt))
    {
      printf("CLinuxInputDevice: read error : %s\n", strerror(errno));
      break;
    }

    if (!TranslateEvent(levt, devt))
      continue;

    /* Flush previous event with DIEF_FOLLOW? */
    if (devt.type != XBMC_NOEVENT)
    {
      //printf("new event! type = %d\n", devt.type);
      //printf("key: %d %d %d %c\n", devt.key.keysym.scancode, devt.key.keysym.sym, devt.key.keysym.mod, devt.key.keysym.unicode);

      if (m_hasLeds && (m_keyMods != m_lastKeyMods))
      {
        SetLed(LED_NUML, m_keyMods & XBMCKMOD_NUM);
        SetLed(LED_CAPSL, m_keyMods & XBMCKMOD_CAPS);
        m_lastKeyMods = m_keyMods;
      }

      break;
    }
  }

  return devt;
}

void CLinuxInputDevice::SetupKeyboardAutoRepeat(int fd)
{
  bool enable = true;

#if defined(HAS_LIBAMCODEC)
  if (aml_get_device_type() == AML_DEVICE_TYPE_M1 || aml_get_device_type() == AML_DEVICE_TYPE_M3)
  {
    // ignore the native aml driver named 'key_input',
    //  it is the dedicated power key handler (am_key_input)
    if (strncmp(m_deviceName, "key_input", strlen("key_input")) == 0)
      return;
    // ignore the native aml driver named 'aml_keypad',
    //  it is the dedicated IR remote handler (amremote)
    else if (strncmp(m_deviceName, "aml_keypad", strlen("aml_keypad")) == 0)
      return;

    // turn off any keyboard autorepeat, there is a kernel bug
    // where if the cpu is max'ed then key up is missed and
    // we get a flood of EV_REP that never stop until next
    // key down/up. Very nasty when seeking during video playback.
    enable = false;
  }
#endif

  if (enable)
  {
    int kbdrep[2] = { 400, 80 };
    ioctl(fd, EVIOCSREP, kbdrep);
  }
  else
  {
    struct input_event event;
    memset(&event, 0, sizeof(event));

    gettimeofday(&event.time, NULL);
    event.type  = EV_REP;
    event.code  = REP_DELAY;
    event.value = 0;
    write(fd, &event, sizeof(event));

    gettimeofday(&event.time, NULL);
    event.type  = EV_REP;
    event.code  = REP_PERIOD;
    event.value = 0;
    write(fd, &event, sizeof(event));

    CLog::Log(LOGINFO, "CLinuxInputDevice: auto key repeat disabled on device '%s'\n", m_deviceName);
  }
}

/*
 * Fill device information.
 * Queries the input device and tries to classify it.
 */
void CLinuxInputDevice::GetInfo(int fd)
{
  unsigned int num_keys = 0;
  unsigned int num_ext_keys = 0;
  unsigned int num_buttons = 0;
  unsigned int num_rels = 0;
  unsigned int num_abs = 0;

  unsigned long evbit[NBITS(EV_CNT)];
  unsigned long keybit[NBITS(KEY_CNT)];

  /* get device name */
  bzero(m_deviceName, sizeof(m_deviceName));
  ioctl(fd, EVIOCGNAME(sizeof(m_deviceName)-1), m_deviceName);

  if (strncmp(m_deviceName, "D-Link Boxee D-Link Boxee Receiver", strlen("D-Link Boxee D-Link Boxee Receiver")) == 0)
  {
    m_bSkipNonKeyEvents = true;
  }
  else
  {
    m_bSkipNonKeyEvents = false;
  }
  CLog::Log(LOGINFO, "opened device '%s' (file name %s), m_bSkipNonKeyEvents %d\n", m_deviceName, m_fileName.c_str(), m_bSkipNonKeyEvents);

  /* get event type bits */
  ioctl(fd, EVIOCGBIT(0, sizeof(evbit)), evbit);

  if (test_bit( EV_KEY, evbit ))
  {
    int i;

    /* get keyboard bits */
    ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(keybit)), keybit);

    /**  count typical keyboard keys only */
    for (i = KEY_Q; i <= KEY_M; i++)
      if (test_bit( i, keybit ))
        num_keys++;

    for (i = KEY_OK; i < KEY_CNT; i++)
      if (test_bit( i, keybit ))
        num_ext_keys++;

    for (i = BTN_MOUSE; i < BTN_JOYSTICK; i++)
      if (test_bit( i, keybit ))
        num_buttons++;
  }

#ifndef HAS_INTELCE
  unsigned long relbit[NBITS(REL_CNT)];
  unsigned long absbit[NBITS(ABS_CNT)];

  if (test_bit( EV_REL, evbit ))
  {
    int i;

    /* get bits for relative axes */
    ioctl(fd, EVIOCGBIT(EV_REL, sizeof(relbit)), relbit);

    for (i = 0; i < REL_CNT; i++)
      if (test_bit( i, relbit ))
        num_rels++;
  }

  if (test_bit( EV_ABS, evbit ))
  {
    int i;

    /* get bits for absolute axes */
    ioctl(fd, EVIOCGBIT(EV_ABS, sizeof(absbit)), absbit);

    for (i = 0; i < ABS_PRESSURE; i++)
      if (test_bit( i, absbit ))
        num_abs++;
  }

  /* Mouse, Touchscreen or Smartpad ? */
  if ((test_bit( EV_KEY, evbit ) && (test_bit( BTN_TOUCH, keybit )
      || test_bit( BTN_TOOL_FINGER, keybit ))) || ((num_rels >= 2
      && num_buttons) || (num_abs == 2 && (num_buttons == 1))))
    m_deviceType |= LI_DEVICE_MOUSE;
  else if (num_abs && num_buttons) /* Or a Joystick? */
    m_deviceType |= LI_DEVICE_JOYSTICK;
#endif

  /* A Keyboard, do we have at least some letters? */
  if (num_keys > 20)
  {
    m_deviceType |= LI_DEVICE_KEYBOARD;
    m_deviceCaps |= LI_CAPS_KEYS;

    m_deviceMinKeyCode = 0;
    m_deviceMaxKeyCode = 127;
  }

  /* A Remote Control? */
  if (num_ext_keys)
  {
    m_deviceType |= LI_DEVICE_REMOTE;
    m_deviceCaps |= LI_CAPS_KEYS;
  }

  /* Buttons */
  if (num_buttons)
  {
    m_deviceCaps |= LI_CAPS_BUTTONS;
    m_deviceMaxKeyCode = num_buttons - 1;
  }

  /* Axes */
  if (num_rels || num_abs)
  {
    m_deviceCaps |= LI_CAPS_AXES;
    m_deviceMaxAxis = std::max(num_rels, num_abs) - 1;
  }

  /* Decide which primary input device to be. */
  if (m_deviceType & LI_DEVICE_KEYBOARD)
    m_devicePreferredId = LI_DEVICE_KEYBOARD;
  else if (m_deviceType & LI_DEVICE_REMOTE)
    m_devicePreferredId = LI_DEVICE_REMOTE;
  else if (m_deviceType & LI_DEVICE_JOYSTICK)
    m_devicePreferredId = LI_DEVICE_JOYSTICK;
  else if (m_deviceType & LI_DEVICE_MOUSE)
    m_devicePreferredId = LI_DEVICE_MOUSE;
  else
    m_devicePreferredId = LI_DEVICE_NONE;

  //printf("type: %d\n", m_deviceType);
  //printf("caps: %d\n", m_deviceCaps);
  //printf("pref: %d\n", m_devicePreferredId);
}

const std::string& CLinuxInputDevice::GetFileName()
{
  return m_fileName;
}

bool CLinuxInputDevice::IsUnplugged()
{
  return m_bUnplugged;
}

bool CLinuxInputDevices::CheckDevice(const char *device)
{
  int fd;

  // Does the device exists?
  struct stat buffer;
  if (stat(device, &buffer) != 0)
    return false;

  /* Check if we are able to open the device */
  fd = open(device, O_RDWR);
  if (fd < 0)
    return false;

  if (ioctl(fd, EVIOCGRAB, 1) && errno != EINVAL)
  {
    close(fd);
    return false;
  }

  ioctl(fd, EVIOCGRAB, 0);

  close(fd);

  return true;
}

/* exported symbols */

/*
 * Return the number of available devices.
 * Called once during initialization of DirectFB.
 */
void CLinuxInputDevices::InitAvailable()
{
  CSingleLock lock(m_devicesListLock);

  /* Close any devices that may have been initialized previously */
  for (size_t i = 0; i < m_devices.size(); i++)
  {
    delete m_devices[i];
  }
  m_devices.clear();

  int deviceId = 0;

  /* No devices specified. Try to guess some. */
  for (int i = 0; i < MAX_LINUX_INPUT_DEVICES; i++)
  {
    char buf[32];

    snprintf(buf, 32, "/dev/input/event%d", i);
    if (CheckDevice(buf))
    {
      CLog::Log(LOGINFO, "Found input device %s", buf);
      m_devices.push_back(new CLinuxInputDevice(buf, deviceId));
      ++deviceId;
    }
  }
}

/*
 * Check for hot plugged devices.
 */
void CLinuxInputDevices::CheckHotplugged()
{
  CSingleLock lock(m_devicesListLock);

  int deviceId = m_devices.size();

  /* No devices specified. Try to guess some. */
  for (int i = 0; i < MAX_LINUX_INPUT_DEVICES; i++)
  {
    char buf[32];
    bool ispresent = false;

    snprintf(buf, 32, "/dev/input/event%d", i);

    for (size_t j = 0; j < m_devices.size(); j++)
    {
      if (m_devices[j]->GetFileName().compare(buf) == 0)
      {
        ispresent = true;
        break;
      }
    }

    if (!ispresent && CheckDevice(buf))
    {
      CLog::Log(LOGINFO, "Found input device %s", buf);
      m_devices.push_back(new CLinuxInputDevice(buf, deviceId));
      ++deviceId;
    }
  }
}

/*
 * Open the device, fill out information about it,
 * allocate and fill private data, start input thread.
 */
bool CLinuxInputDevice::Open()
{
  int fd, ret;
  unsigned long ledbit[NBITS(LED_CNT)];

  /* open device */
  fd = open(m_fileName.c_str(), O_RDWR | O_NONBLOCK);
  if (fd < 0)
  {
    CLog::Log(LOGERROR, "CLinuxInputDevice: could not open device: %s\n", m_fileName.c_str());
    return false;
  }

  /* grab device */
  ret = ioctl(fd, EVIOCGRAB, 1);
  if (ret && errno != EINVAL)
  {
    CLog::Log(LOGERROR, "CLinuxInputDevice: could not grab device: %s\n", m_fileName.c_str());
    close(fd);
    return false;
  }

  // Set the socket to non-blocking
  int opts = 0;
  if ((opts = fcntl(fd, F_GETFL)) < 0)
  {
    CLog::Log(LOGERROR, "CLinuxInputDevice %s: fcntl(F_GETFL) failed: %s", __FUNCTION__ , strerror(errno));
    close(fd);
    return false;
  }

  opts = (opts | O_NONBLOCK);
  if (fcntl(fd, F_SETFL, opts) < 0)
  {
    CLog::Log(LOGERROR, "CLinuxInputDevice %s: fcntl(F_SETFL) failed: %s", __FUNCTION__, strerror(errno));
    close(fd);
    return false;
  }

  /* fill device info structure */
  GetInfo(fd);

  if (m_deviceType & LI_DEVICE_KEYBOARD)
    SetupKeyboardAutoRepeat(fd);

  m_fd = fd;
  m_vt_fd = -1;

  if (m_deviceMinKeyCode >= 0 && m_deviceMaxKeyCode >= m_deviceMinKeyCode)
  {
    if (m_vt_fd < 0)
      m_vt_fd = open("/dev/tty0", O_RDWR | O_NOCTTY);
 
    if (m_vt_fd < 0)
      m_vt_fd = open("/dev/tty1", O_RDWR | O_NOCTTY);

    if (m_vt_fd < 0)
      CLog::Log(LOGWARNING, "no keymap support (requires /dev/tty0 - CONFIG_VT)");
  }

  /* check if the device has LEDs */
  ret = ioctl(fd, EVIOCGBIT(EV_LED, sizeof(ledbit)), ledbit);
  if (ret < 0)
	  CLog::Log(LOGWARNING, "DirectFB/linux_input: could not get LED bits" );
  else
    m_hasLeds = test_bit( LED_SCROLLL, ledbit ) || test_bit( LED_NUML, ledbit )
        || test_bit( LED_CAPSL, ledbit );

  if (m_hasLeds)
  {
    /* get LED state */
    ret = ioctl(fd, EVIOCGLED(sizeof(m_ledState)), m_ledState);
    if (ret < 0)
    {
      CLog::Log(LOGERROR, "DirectFB/linux_input: could not get LED state");
      goto driver_open_device_error;
    }

    /* turn off LEDs */
    SetLed(LED_SCROLLL, 0);
    SetLed(LED_NUML, 0);
    SetLed(LED_CAPSL, 0);
  }

  return true;

driver_open_device_error:

  ioctl(fd, EVIOCGRAB, 0);
  if (m_vt_fd >= 0)
  {
    close(m_vt_fd);
    m_vt_fd = -1;
  }
  close(fd);
  m_fd = -1;

  return false;
}

/*
 * Fetch one entry from the kernel keymap.
 */
bool CLinuxInputDevice::GetKeymapEntry(KeymapEntry& entry)
{
  int code = entry.code;
  unsigned short value;
  //DFBInputDeviceKeyIdentifier identifier;

  if (m_vt_fd < 0)
    return false;

  // to support '+'  and '/' with Boxee's remote control we do something ugly like this for now
  if (KVAL(code) == 98)
  {
    code = K(KTYP(code),53);
  }

  /* fetch the base level */
  value = KeyboardGetSymbol(KeyboardReadValue(K_NORMTAB, code));
  //printf("base=%d typ=%d code %d\n", KVAL(value), KTYP(value), code);

  /* write base level symbol to entry */
  entry.base = value; //KeyboardGetSymbol(code, value, LI_KEYLEVEL_BASE);

  /* fetch the shifted base level */
  value = KeyboardGetSymbol(KeyboardReadValue(K_SHIFTTAB, entry.code));
  //printf("shift=%d\n", value);

  /* write shifted base level symbol to entry */
  entry.shift = value; //KeyboardGetSymbol(code, value, LI_KEYLEVEL_SHIFT);

  // to support '+'  and '/' with Boxee's remote control we could do ugly something like this for now
  if (KVAL(code) == 78)
  {
    //code = K(KTYP(code),13);
    //entry.code = K(KTYP(code),13);
    entry.base = K(KTYP(code),43);
  }

  /* fetch the alternative level */
  value = KeyboardGetSymbol(KeyboardReadValue(K_ALTTAB, entry.code));
  //printf("alt=%d\n", value);

  /* write alternative level symbol to entry */
  entry.alt = value; //KeyboardGetSymbol(code, value, LI_KEYLEVEL_ALT);

  /* fetch the shifted alternative level */
  value = KeyboardGetSymbol(KeyboardReadValue(K_ALTSHIFTTAB, entry.code));
  //printf("altshift=%d\n", value);

  /* write shifted alternative level symbol to entry */
  entry.altShift = value; //KeyboardGetSymbol(code, value, LI_KEYLEVEL_ALT_SHIFT);

  return true;
}

/*
 * End thread, close device and free private data.
 */
void CLinuxInputDevice::Close()
{
  /* release device */
  ioctl(m_fd, EVIOCGRAB, 0);

  if (m_vt_fd >= 0)
    close(m_vt_fd);

  /* close file */
  close(m_fd);
}

XBMC_Event CLinuxInputDevices::ReadEvent()
{
  if (m_bReInitialize)
  {
    InitAvailable();
    m_bReInitialize = false;
  }
  else
  {
    time_t now;
    time(&now);

    if ((now - m_lastHotplugCheck) >= 10)
    {
      CheckHotplugged();
      m_lastHotplugCheck = now;
    }
  }

  CSingleLock lock(m_devicesListLock);

  XBMC_Event event;
  event.type = XBMC_NOEVENT;

  for (size_t i = 0; i < m_devices.size(); i++)
  {
    event = m_devices[i]->ReadEvent();
    if (event.type != XBMC_NOEVENT)
    {
      break;
    }

    if (m_devices[i]->IsUnplugged())
    {
      m_bReInitialize = true;
      break;
    }
  }

  return event;
}

/*
   - 0x7F -> if not paired, battery OK
   - 0xFF -> if paired, battery OK
   - 0x00 -> if not paired, battery low
   - 0x80 -> if paired, battery low
*/
bool CLinuxInputDevices::IsRemoteLowBattery()
{
  bool bLowBattery = !(remoteStatus & 0xF);
  return bLowBattery;
}

bool CLinuxInputDevices::IsRemoteNotPaired()
{
  bool bRemoteNotPaired = !(remoteStatus & 0x70) || !(remoteStatus & 0x80);
  return bRemoteNotPaired;
}

/*
int main()
{
  CLinuxInputDevices devices;
  devices.InitAvailable();
  while (1)
  {
    XBMC_Event event = devices.ReadEvent();
    if (event.type != XBMC_NOEVENT)
    {
      printf("%d\n", event.type);
    }
    usleep(1000);
  }

}
*/
#endif
