/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "LibInputKeyboard.h"

#include "AppInboundProtocol.h"
#include "LibInputSettings.h"
#include "ServiceBroker.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/log.h"

#include <algorithm>
#include <fcntl.h>
#include <linux/input.h>
#include <map>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <xkbcommon/xkbcommon-keysyms.h>
#include <xkbcommon/xkbcommon-names.h>

const int REPEAT_DELAY = 400;
const int REPEAT_RATE = 80;

static const std::map<xkb_keysym_t, XBMCKey> xkbMap =
{
  // Function keys before start of ASCII printable character range
  { XKB_KEY_BackSpace, XBMCK_BACKSPACE },
  { XKB_KEY_Tab, XBMCK_TAB },
  { XKB_KEY_Clear, XBMCK_CLEAR },
  { XKB_KEY_Return, XBMCK_RETURN },
  { XKB_KEY_Pause, XBMCK_PAUSE },
  { XKB_KEY_Escape, XBMCK_ESCAPE },

  // ASCII printable range - not included here

  // Function keys after end of ASCII printable character range
  { XKB_KEY_Delete, XBMCK_DELETE },

  // Multimedia keys
  { XKB_KEY_XF86Back, XBMCK_BROWSER_BACK },
  { XKB_KEY_XF86Forward, XBMCK_BROWSER_FORWARD },
  { XKB_KEY_XF86Refresh, XBMCK_BROWSER_REFRESH },
  { XKB_KEY_XF86Stop, XBMCK_BROWSER_STOP },
  { XKB_KEY_XF86Search, XBMCK_BROWSER_SEARCH },
  // XKB_KEY_XF86Favorites could be XBMCK_BROWSER_FAVORITES or XBMCK_FAVORITES,
  // XBMCK_FAVORITES was chosen here because it is more general
  { XKB_KEY_XF86HomePage, XBMCK_BROWSER_HOME },
  { XKB_KEY_XF86AudioMute, XBMCK_VOLUME_MUTE },
  { XKB_KEY_XF86AudioLowerVolume, XBMCK_VOLUME_DOWN },
  { XKB_KEY_XF86AudioRaiseVolume, XBMCK_VOLUME_UP },
  { XKB_KEY_XF86AudioNext, XBMCK_MEDIA_NEXT_TRACK },
  { XKB_KEY_XF86AudioPrev, XBMCK_MEDIA_PREV_TRACK },
  { XKB_KEY_XF86AudioStop, XBMCK_MEDIA_STOP },
  { XKB_KEY_XF86AudioPause, XBMCK_MEDIA_PLAY_PAUSE },
  { XKB_KEY_XF86Mail, XBMCK_LAUNCH_MAIL },
  { XKB_KEY_XF86Select, XBMCK_LAUNCH_MEDIA_SELECT },
  { XKB_KEY_XF86Launch0, XBMCK_LAUNCH_APP1 },
  { XKB_KEY_XF86Launch1, XBMCK_LAUNCH_APP2 },
  { XKB_KEY_XF86WWW, XBMCK_LAUNCH_FILE_BROWSER },
  { XKB_KEY_XF86AudioMedia, XBMCK_LAUNCH_MEDIA_CENTER },
  { XKB_KEY_XF86AudioRewind, XBMCK_MEDIA_REWIND },
  { XKB_KEY_XF86AudioForward, XBMCK_MEDIA_FASTFORWARD },

  // Numeric keypad
  { XKB_KEY_KP_0, XBMCK_KP0 },
  { XKB_KEY_KP_1, XBMCK_KP1 },
  { XKB_KEY_KP_2, XBMCK_KP2 },
  { XKB_KEY_KP_3, XBMCK_KP3 },
  { XKB_KEY_KP_4, XBMCK_KP4 },
  { XKB_KEY_KP_5, XBMCK_KP5 },
  { XKB_KEY_KP_6, XBMCK_KP6 },
  { XKB_KEY_KP_7, XBMCK_KP7 },
  { XKB_KEY_KP_8, XBMCK_KP8 },
  { XKB_KEY_KP_9, XBMCK_KP9 },
  { XKB_KEY_KP_Decimal, XBMCK_KP_PERIOD },
  { XKB_KEY_KP_Divide, XBMCK_KP_DIVIDE },
  { XKB_KEY_KP_Multiply, XBMCK_KP_MULTIPLY },
  { XKB_KEY_KP_Subtract, XBMCK_KP_MINUS },
  { XKB_KEY_KP_Add, XBMCK_KP_PLUS },
  { XKB_KEY_KP_Enter, XBMCK_KP_ENTER },
  { XKB_KEY_KP_Equal, XBMCK_KP_EQUALS },

  // Arrows + Home/End pad
  { XKB_KEY_Up, XBMCK_UP },
  { XKB_KEY_Down, XBMCK_DOWN },
  { XKB_KEY_Right, XBMCK_RIGHT },
  { XKB_KEY_Left, XBMCK_LEFT },
  { XKB_KEY_Insert, XBMCK_INSERT },
  { XKB_KEY_Home, XBMCK_HOME },
  { XKB_KEY_End, XBMCK_END },
  { XKB_KEY_Page_Up, XBMCK_PAGEUP },
  { XKB_KEY_Page_Down, XBMCK_PAGEDOWN },

  // Key state modifier keys
  { XKB_KEY_Num_Lock, XBMCK_NUMLOCK },
  { XKB_KEY_Caps_Lock, XBMCK_CAPSLOCK },
  { XKB_KEY_Scroll_Lock, XBMCK_SCROLLOCK },
  { XKB_KEY_Shift_R, XBMCK_RSHIFT },
  { XKB_KEY_Shift_L, XBMCK_LSHIFT },
  { XKB_KEY_Control_R, XBMCK_RCTRL },
  { XKB_KEY_Control_L, XBMCK_LCTRL },
  { XKB_KEY_Alt_R, XBMCK_RALT },
  { XKB_KEY_Alt_L, XBMCK_LALT },
  { XKB_KEY_Meta_R, XBMCK_RMETA },
  { XKB_KEY_Meta_L, XBMCK_LMETA },
  { XKB_KEY_Super_R, XBMCK_RSUPER },
  { XKB_KEY_Super_L, XBMCK_LSUPER },
  // XKB does not have XBMCK_MODE/"Alt Gr" - probably equal to XKB_KEY_Alt_R
  { XKB_KEY_Multi_key, XBMCK_COMPOSE },

  // Miscellaneous function keys
  { XKB_KEY_Help, XBMCK_HELP },
  { XKB_KEY_Print, XBMCK_PRINT },
  { XKB_KEY_Sys_Req, XBMCK_SYSREQ},
  { XKB_KEY_Break, XBMCK_BREAK },
  { XKB_KEY_Menu, XBMCK_MENU },
  { XKB_KEY_XF86PowerOff, XBMCK_POWER },
  { XKB_KEY_EcuSign, XBMCK_EURO },
  { XKB_KEY_Undo, XBMCK_UNDO },
  { XKB_KEY_XF86Sleep, XBMCK_SLEEP },
  // Unmapped: XBMCK_GUIDE, XBMCK_SETTINGS, XBMCK_INFO
  { XKB_KEY_XF86Red, XBMCK_RED },
  { XKB_KEY_XF86Green, XBMCK_GREEN },
  { XKB_KEY_XF86Yellow, XBMCK_YELLOW },
  { XKB_KEY_XF86Blue, XBMCK_BLUE },
  // Unmapped: XBMCK_ZOOM, XBMCK_TEXT
  { XKB_KEY_XF86Favorites, XBMCK_FAVORITES },
  { XKB_KEY_XF86HomePage, XBMCK_HOMEPAGE },
  // Unmapped: XBMCK_CONFIG, XBMCK_EPG

  // Media keys
  { XKB_KEY_XF86Eject, XBMCK_EJECT },
  // XBMCK_STOP clashes with XBMCK_MEDIA_STOP
  { XKB_KEY_XF86AudioRecord, XBMCK_RECORD },
  // XBMCK_REWIND clashes with XBMCK_MEDIA_REWIND
  { XKB_KEY_XF86Phone, XBMCK_PHONE },
  { XKB_KEY_XF86AudioPlay, XBMCK_PLAY },
  { XKB_KEY_XF86AudioRandomPlay, XBMCK_SHUFFLE }
  // XBMCK_FASTFORWARD clashes with XBMCK_MEDIA_FASTFORWARD
};

CLibInputKeyboard::CLibInputKeyboard()
  : m_repeatTimer(std::bind(&CLibInputKeyboard::KeyRepeatTimeout, this))
{
  m_ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
  if (!m_ctx)
  {
    CLog::Log(LOGERROR, "CLibInputKeyboard::%s - failed to create xkb context", __FUNCTION__);
    return;
  }

  std::string layout = CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(CLibInputSettings::SETTING_INPUT_LIBINPUTKEYBOARDLAYOUT);

  if (!SetKeymap(layout))
  {
    CLog::Log(LOGERROR, "CLibInputKeyboard::%s - failed set default keymap", __FUNCTION__);
    return;
  }
}

CLibInputKeyboard::~CLibInputKeyboard()
{
  xkb_state_unref(m_state);
  xkb_keymap_unref(m_keymap);
  xkb_context_unref(m_ctx);
}

bool CLibInputKeyboard::SetKeymap(const std::string& layout)
{
  xkb_state_unref(m_state);
  xkb_keymap_unref(m_keymap);

  xkb_rule_names names;

  names.rules = nullptr;
  names.model = nullptr;
  names.layout = layout.c_str();
  names.variant = nullptr;
  names.options = nullptr;

  m_keymap = xkb_keymap_new_from_names(m_ctx, &names, XKB_KEYMAP_COMPILE_NO_FLAGS);
  if (!m_keymap)
  {
    CLog::Log(LOGERROR, "CLibInputKeyboard::%s - failed to compile keymap", __FUNCTION__);
    return false;
  }

  m_state = xkb_state_new(m_keymap);
  if (!m_state)
  {
    CLog::Log(LOGERROR, "CLibInputKeyboard::%s - failed to create xkb state", __FUNCTION__);
    return false;
  }

  m_modindex[0] = xkb_keymap_mod_get_index(m_keymap, XKB_MOD_NAME_CTRL);
  m_modindex[1] = xkb_keymap_mod_get_index(m_keymap, XKB_MOD_NAME_ALT);
  m_modindex[2] = xkb_keymap_mod_get_index(m_keymap, XKB_MOD_NAME_SHIFT);
  m_modindex[3] = xkb_keymap_mod_get_index(m_keymap, XKB_MOD_NAME_LOGO);

  m_ledindex[0] = xkb_keymap_led_get_index(m_keymap, XKB_LED_NAME_NUM);
  m_ledindex[1] = xkb_keymap_led_get_index(m_keymap, XKB_LED_NAME_CAPS);
  m_ledindex[2] = xkb_keymap_led_get_index(m_keymap, XKB_LED_NAME_SCROLL);

  m_leds = 0;

  return true;
}

void CLibInputKeyboard::ProcessKey(libinput_event_keyboard *e)
{
  if (!m_ctx || !m_keymap || !m_state)
    return;

  XBMC_Event event;
  memset(&event, 0, sizeof(event));

  const uint32_t xkbkey = libinput_event_keyboard_get_key(e) + 8;
  const bool pressed = libinput_event_keyboard_get_key_state(e) == LIBINPUT_KEY_STATE_PRESSED;

  event.type = pressed ? XBMC_KEYDOWN : XBMC_KEYUP;
  xkb_state_update_key(m_state, xkbkey, pressed ? XKB_KEY_DOWN : XKB_KEY_UP);

  const xkb_keysym_t keysym = xkb_state_key_get_one_sym(m_state, xkbkey);

  int mod = XBMCKMOD_NONE;

  xkb_state_component modtype = xkb_state_component(XKB_STATE_MODS_DEPRESSED | XKB_STATE_MODS_LATCHED);
  if (xkb_state_mod_index_is_active(m_state, m_modindex[0], modtype) && ((keysym != XBMCK_LCTRL) || !pressed))
    mod |= XBMCKMOD_CTRL;
  if (xkb_state_mod_index_is_active(m_state, m_modindex[0], modtype) && ((keysym != XBMCK_RCTRL) || !pressed))
    mod |= XBMCKMOD_CTRL;
  if (xkb_state_mod_index_is_active(m_state, m_modindex[1], modtype) && ((keysym != XBMCK_LALT) || !pressed))
    mod |= XBMCKMOD_ALT;
  if (xkb_state_mod_index_is_active(m_state, m_modindex[1], modtype) && ((keysym != XBMCK_RALT) || !pressed))
    mod |= XBMCKMOD_ALT;
  if (xkb_state_mod_index_is_active(m_state, m_modindex[2], modtype) && ((keysym != XBMCK_LSHIFT) || !pressed))
    mod |= XBMCKMOD_SHIFT;
  if (xkb_state_mod_index_is_active(m_state, m_modindex[2], modtype) && ((keysym != XBMCK_RSHIFT) || !pressed))
    mod |= XBMCKMOD_SHIFT;
  if (xkb_state_mod_index_is_active(m_state, m_modindex[3], modtype) && ((keysym != XBMCK_LMETA) || !pressed))
    mod |= XBMCKMOD_META;
  if (xkb_state_mod_index_is_active(m_state, m_modindex[3], modtype) && ((keysym != XBMCK_RMETA) || !pressed))
    mod |= XBMCKMOD_META;

  m_leds = 0;

  if (xkb_state_led_index_is_active(m_state, m_ledindex[0]) && ((keysym != XBMCK_NUMLOCK) || !pressed))
  {
    m_leds |= LIBINPUT_LED_NUM_LOCK;
    mod |= XBMCKMOD_NUM;
  }
  if (xkb_state_led_index_is_active(m_state, m_ledindex[1]) && ((keysym != XBMCK_CAPSLOCK) || !pressed))
  {
    m_leds |= LIBINPUT_LED_CAPS_LOCK;
    mod |= XBMCKMOD_CAPS;
  }
  if (xkb_state_led_index_is_active(m_state, m_ledindex[2]) && ((keysym != XBMCK_SCROLLOCK) || !pressed))
  {
    m_leds |= LIBINPUT_LED_SCROLL_LOCK;
    //mod |= XBMCK_SCROLLOCK;
  }

  uint32_t unicode = xkb_state_key_get_utf32(m_state, xkbkey);
  if (unicode > std::numeric_limits<std::uint16_t>::max())
  {
    // Kodi event system only supports UTF16, so ignore the codepoint if it does not fit
    unicode = 0;
  }

  uint32_t scancode = libinput_event_keyboard_get_key(e);
  if (scancode > std::numeric_limits<unsigned char>::max())
  {
    // Kodi scancodes are limited to unsigned char, pretend the scancode is unknown on overflow
    scancode = 0;
  }

  event.key.keysym.mod = XBMCMod(mod);
  event.key.keysym.sym = XBMCKeyForKeysym(keysym, scancode);
  event.key.keysym.scancode = scancode;
  event.key.keysym.unicode = unicode;

  std::shared_ptr<CAppInboundProtocol> appPort = CServiceBroker::GetAppPort();
  if (appPort)
    appPort->OnEvent(event);

  if (pressed && xkb_keymap_key_repeats(m_keymap, xkbkey))
  {
    libinput_event *ev = libinput_event_keyboard_get_base_event(e);
    libinput_device *dev = libinput_event_get_device(ev);
    auto data = m_repeatData.find(dev);
    if (data != m_repeatData.end())
    {
      CLog::Log(LOGDEBUG, "CLibInputKeyboard::%s - using delay: %ims repeat: %ims", __FUNCTION__, data->second.at(0), data->second.at(1));

      m_repeatRate = data->second.at(1);
      m_repeatTimer.Stop(true);
      m_repeatEvent = event;
      m_repeatTimer.Start(data->second.at(0), false);
    }
  }
  else
  {
    m_repeatTimer.Stop();
  }
}

XBMCKey CLibInputKeyboard::XBMCKeyForKeysym(xkb_keysym_t sym, uint32_t scancode)
{
  if (sym >= 'A' && sym <= 'Z')
  {
    return static_cast<XBMCKey> (sym + 'a' - 'A');
  }
  else if (sym >= XKB_KEY_space && sym <= XKB_KEY_asciitilde)
  {
    return static_cast<XBMCKey> (sym);
  }
  else if (sym >= XKB_KEY_F1 && sym <= XKB_KEY_F15)
  {
    return static_cast<XBMCKey> (XBMCK_F1 + ((int)sym - XKB_KEY_F1));
  }

  auto xkbmapping = xkbMap.find(sym);
  if (xkbmapping != xkbMap.end())
    return xkbmapping->second;

  return XBMCK_UNKNOWN;
}

void CLibInputKeyboard::KeyRepeatTimeout()
{
  m_repeatTimer.RestartAsync(m_repeatRate);

  std::shared_ptr<CAppInboundProtocol> appPort = CServiceBroker::GetAppPort();
  if (appPort)
    appPort->OnEvent(m_repeatEvent);
}

void CLibInputKeyboard::UpdateLeds(libinput_device *dev)
{
  libinput_device_led_update(dev, static_cast<libinput_led>(m_leds));
}

void CLibInputKeyboard::GetRepeat(libinput_device *dev)
{
  int kbdrep[2] = { 400, 80 };
  const char *name = libinput_device_get_name(dev);
  const char *sysname = libinput_device_get_sysname(dev);
  std::string path("/dev/input/");
  path.append(sysname);
  auto fd = open(path.c_str(), O_RDONLY);

  if (fd < 0)
  {
    CLog::Log(LOGERROR, "CLibInputKeyboard::%s - failed to open %s (%s)", __FUNCTION__, sysname, strerror(errno));
  }
  else
  {
    auto ret = ioctl(fd, EVIOCGREP, &kbdrep);
    if (ret < 0)
      CLog::Log(LOGDEBUG, "CLibInputKeyboard::%s - could not get key repeat for %s (%s)", __FUNCTION__, sysname, strerror(errno));

    CLog::Log(LOGDEBUG, "CLibInputKeyboard::%s - delay: %ims repeat: %ims for %s (%s)", __FUNCTION__, kbdrep[0], kbdrep[1], name, sysname);
    close(fd);
  }

  std::vector<int> kbdrepvec(std::begin(kbdrep), std::end(kbdrep));

  auto data = m_repeatData.find(dev);
  if (data == m_repeatData.end())
  {
    m_repeatData.insert(std::make_pair(dev, kbdrepvec));
  }
}
