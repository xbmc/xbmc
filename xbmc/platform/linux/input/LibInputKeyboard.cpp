/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "LibInputKeyboard.h"

#include "LangInfo.h"
#include "LibInputSettings.h"
#include "ServiceBroker.h"
#include "application/AppInboundProtocol.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/Map.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <algorithm>
#include <optional>
#include <string.h>

#include <fcntl.h>
#include <linux/input.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <xkbcommon/xkbcommon-keysyms.h>
#include <xkbcommon/xkbcommon-names.h>

namespace
{
constexpr unsigned int REPEAT_DELAY = 400;
constexpr unsigned int REPEAT_RATE = 80;

constexpr auto xkbMap = make_map<xkb_keysym_t, XBMCKey>({
    // Function keys before start of ASCII printable character range
    {XKB_KEY_BackSpace, XBMCK_BACKSPACE},
    {XKB_KEY_Tab, XBMCK_TAB},
    {XKB_KEY_Clear, XBMCK_CLEAR},
    {XKB_KEY_Return, XBMCK_RETURN},
    {XKB_KEY_Pause, XBMCK_PAUSE},
    {XKB_KEY_Escape, XBMCK_ESCAPE},

    // ASCII printable range - not included here

    // Function keys after end of ASCII printable character range
    {XKB_KEY_Delete, XBMCK_DELETE},

    // Multimedia keys
    {XKB_KEY_XF86Back, XBMCK_BROWSER_BACK},
    {XKB_KEY_XF86Forward, XBMCK_BROWSER_FORWARD},
    {XKB_KEY_XF86Refresh, XBMCK_BROWSER_REFRESH},
    {XKB_KEY_XF86Stop, XBMCK_BROWSER_STOP},
    {XKB_KEY_XF86Search, XBMCK_BROWSER_SEARCH},
    // XKB_KEY_XF86Favorites could be XBMCK_BROWSER_FAVORITES or XBMCK_FAVORITES,
    // XBMCK_FAVORITES was chosen here because it is more general
    {XKB_KEY_XF86HomePage, XBMCK_BROWSER_HOME},
    {XKB_KEY_XF86AudioMute, XBMCK_VOLUME_MUTE},
    {XKB_KEY_XF86AudioLowerVolume, XBMCK_VOLUME_DOWN},
    {XKB_KEY_XF86AudioRaiseVolume, XBMCK_VOLUME_UP},
    {XKB_KEY_XF86AudioNext, XBMCK_MEDIA_NEXT_TRACK},
    {XKB_KEY_XF86AudioPrev, XBMCK_MEDIA_PREV_TRACK},
    {XKB_KEY_XF86AudioStop, XBMCK_MEDIA_STOP},
    {XKB_KEY_XF86AudioPause, XBMCK_MEDIA_PLAY_PAUSE},
    {XKB_KEY_XF86Mail, XBMCK_LAUNCH_MAIL},
    {XKB_KEY_XF86Select, XBMCK_LAUNCH_MEDIA_SELECT},
    {XKB_KEY_XF86Launch0, XBMCK_LAUNCH_APP1},
    {XKB_KEY_XF86Launch1, XBMCK_LAUNCH_APP2},
    {XKB_KEY_XF86WWW, XBMCK_LAUNCH_FILE_BROWSER},
    {XKB_KEY_XF86AudioMedia, XBMCK_LAUNCH_MEDIA_CENTER},
    {XKB_KEY_XF86AudioRewind, XBMCK_MEDIA_REWIND},
    {XKB_KEY_XF86AudioForward, XBMCK_MEDIA_FASTFORWARD},

    // Numeric keypad
    {XKB_KEY_KP_0, XBMCK_KP0},
    {XKB_KEY_KP_1, XBMCK_KP1},
    {XKB_KEY_KP_2, XBMCK_KP2},
    {XKB_KEY_KP_3, XBMCK_KP3},
    {XKB_KEY_KP_4, XBMCK_KP4},
    {XKB_KEY_KP_5, XBMCK_KP5},
    {XKB_KEY_KP_6, XBMCK_KP6},
    {XKB_KEY_KP_7, XBMCK_KP7},
    {XKB_KEY_KP_8, XBMCK_KP8},
    {XKB_KEY_KP_9, XBMCK_KP9},
    {XKB_KEY_KP_Decimal, XBMCK_KP_PERIOD},
    {XKB_KEY_KP_Divide, XBMCK_KP_DIVIDE},
    {XKB_KEY_KP_Multiply, XBMCK_KP_MULTIPLY},
    {XKB_KEY_KP_Subtract, XBMCK_KP_MINUS},
    {XKB_KEY_KP_Add, XBMCK_KP_PLUS},
    {XKB_KEY_KP_Enter, XBMCK_KP_ENTER},
    {XKB_KEY_KP_Equal, XBMCK_KP_EQUALS},

    // Arrows + Home/End pad
    {XKB_KEY_Up, XBMCK_UP},
    {XKB_KEY_Down, XBMCK_DOWN},
    {XKB_KEY_Right, XBMCK_RIGHT},
    {XKB_KEY_Left, XBMCK_LEFT},
    {XKB_KEY_Insert, XBMCK_INSERT},
    {XKB_KEY_Home, XBMCK_HOME},
    {XKB_KEY_End, XBMCK_END},
    {XKB_KEY_Page_Up, XBMCK_PAGEUP},
    {XKB_KEY_Page_Down, XBMCK_PAGEDOWN},

    // Key state modifier keys
    {XKB_KEY_Num_Lock, XBMCK_NUMLOCK},
    {XKB_KEY_Caps_Lock, XBMCK_CAPSLOCK},
    {XKB_KEY_Scroll_Lock, XBMCK_SCROLLOCK},
    {XKB_KEY_Shift_R, XBMCK_RSHIFT},
    {XKB_KEY_Shift_L, XBMCK_LSHIFT},
    {XKB_KEY_Control_R, XBMCK_RCTRL},
    {XKB_KEY_Control_L, XBMCK_LCTRL},
    {XKB_KEY_Alt_R, XBMCK_RALT},
    {XKB_KEY_Alt_L, XBMCK_LALT},
    {XKB_KEY_Meta_R, XBMCK_RMETA},
    {XKB_KEY_Meta_L, XBMCK_LMETA},
    {XKB_KEY_Super_R, XBMCK_RSUPER},
    {XKB_KEY_Super_L, XBMCK_LSUPER},
    // XKB does not have XBMCK_MODE/"Alt Gr" - probably equal to XKB_KEY_Alt_R
    {XKB_KEY_Multi_key, XBMCK_COMPOSE},

    // Miscellaneous function keys
    {XKB_KEY_Help, XBMCK_HELP},
    {XKB_KEY_Print, XBMCK_PRINT},
    {XKB_KEY_Sys_Req, XBMCK_SYSREQ},
    {XKB_KEY_Break, XBMCK_BREAK},
    {XKB_KEY_Menu, XBMCK_MENU},
    {XKB_KEY_XF86PowerOff, XBMCK_POWER},
    {XKB_KEY_EcuSign, XBMCK_EURO},
    {XKB_KEY_Undo, XBMCK_UNDO},
    {XKB_KEY_XF86Sleep, XBMCK_SLEEP},
    // Unmapped: XBMCK_GUIDE, XBMCK_SETTINGS, XBMCK_INFO
    {XKB_KEY_XF86Red, XBMCK_RED},
    {XKB_KEY_XF86Green, XBMCK_GREEN},
    {XKB_KEY_XF86Yellow, XBMCK_YELLOW},
    {XKB_KEY_XF86Blue, XBMCK_BLUE},
    // Unmapped: XBMCK_ZOOM, XBMCK_TEXT
    {XKB_KEY_XF86Favorites, XBMCK_FAVORITES},
    {XKB_KEY_XF86HomePage, XBMCK_HOMEPAGE},
    // Unmapped: XBMCK_CONFIG, XBMCK_EPG

    // Media keys
    {XKB_KEY_XF86Eject, XBMCK_EJECT},
    {XKB_KEY_Cancel, XBMCK_STOP},
    {XKB_KEY_XF86AudioRecord, XBMCK_RECORD},
    // XBMCK_REWIND clashes with XBMCK_MEDIA_REWIND
    {XKB_KEY_XF86Phone, XBMCK_PHONE},
    {XKB_KEY_XF86AudioPlay, XBMCK_PLAY},
    {XKB_KEY_XF86AudioRandomPlay, XBMCK_SHUFFLE}
    // XBMCK_FASTFORWARD clashes with XBMCK_MEDIA_FASTFORWARD
});

constexpr auto logLevelMap = make_map<xkb_log_level, int>({{XKB_LOG_LEVEL_CRITICAL, LOGERROR},
                                                           {XKB_LOG_LEVEL_ERROR, LOGERROR},
                                                           {XKB_LOG_LEVEL_WARNING, LOGWARNING},
                                                           {XKB_LOG_LEVEL_INFO, LOGINFO},
                                                           {XKB_LOG_LEVEL_DEBUG, LOGDEBUG}});

constexpr auto XkbDeadKeyXBMCMapping =
    make_map<xkb_keycode_t, XBMCKey>({{XKB_KEY_dead_grave, XBMCK_GRAVE},
                                      {XKB_KEY_dead_tilde, XBMCK_TILDE},
                                      {XKB_KEY_dead_acute, XBMCK_ACUTE},
                                      {XKB_KEY_dead_circumflex, XBMCK_CIRCUMFLEX},
                                      {XKB_KEY_dead_perispomeni, XBMCK_PERISPOMENI},
                                      {XKB_KEY_dead_macron, XBMCK_MACRON},
                                      {XKB_KEY_dead_breve, XBMCK_BREVE},
                                      {XKB_KEY_dead_abovedot, XBMCK_ABOVEDOT},
                                      {XKB_KEY_dead_diaeresis, XBMCK_DIAERESIS},
                                      {XKB_KEY_dead_abovering, XBMCK_ABOVERING},
                                      {XKB_KEY_dead_doubleacute, XBMCK_DOUBLEACUTE},
                                      {XKB_KEY_dead_caron, XBMCK_CARON},
                                      {XKB_KEY_dead_cedilla, XBMCK_CEDILLA},
                                      {XKB_KEY_dead_ogonek, XBMCK_OGONEK},
                                      {XKB_KEY_dead_iota, XBMCK_IOTA},
                                      {XKB_KEY_dead_voiced_sound, XBMCK_VOICESOUND},
                                      {XKB_KEY_dead_semivoiced_sound, XBMCK_SEMIVOICESOUND},
                                      {XKB_KEY_dead_belowdot, XBMCK_BELOWDOT},
                                      {XKB_KEY_dead_hook, XBMCK_HOOK},
                                      {XKB_KEY_dead_horn, XBMCK_HORN},
                                      {XKB_KEY_dead_stroke, XBMCK_STROKE},
                                      {XKB_KEY_dead_abovecomma, XBMCK_ABOVECOMMA},
                                      {XKB_KEY_dead_psili, XBMCK_ABOVECOMMA},
                                      {XKB_KEY_dead_abovereversedcomma, XBMCK_ABOVEREVERSEDCOMMA},
                                      {XKB_KEY_dead_dasia, XBMCK_OGONEK},
                                      {XKB_KEY_dead_doublegrave, XBMCK_DOUBLEGRAVE},
                                      {XKB_KEY_dead_belowring, XBMCK_BELOWRING},
                                      {XKB_KEY_dead_belowmacron, XBMCK_BELOWMACRON},
                                      {XKB_KEY_dead_belowcircumflex, XBMCK_BELOWCIRCUMFLEX},
                                      {XKB_KEY_dead_belowtilde, XBMCK_BELOWTILDE},
                                      {XKB_KEY_dead_belowbreve, XBMCK_BELOWBREVE},
                                      {XKB_KEY_dead_belowdiaeresis, XBMCK_BELOWDIAERESIS},
                                      {XKB_KEY_dead_invertedbreve, XBMCK_INVERTEDBREVE},
                                      {XKB_KEY_dead_belowcomma, XBMCK_BELOWCOMMA},
                                      {XKB_KEY_dead_a, XBMCK_DEAD_A},
                                      {XKB_KEY_dead_A, XBMCK_DEAD_A},
                                      {XKB_KEY_dead_e, XBMCK_DEAD_E},
                                      {XKB_KEY_dead_E, XBMCK_DEAD_E},
                                      {XKB_KEY_dead_i, XBMCK_DEAD_I},
                                      {XKB_KEY_dead_I, XBMCK_DEAD_I},
                                      {XKB_KEY_dead_o, XBMCK_DEAD_O},
                                      {XKB_KEY_dead_O, XBMCK_DEAD_O},
                                      {XKB_KEY_dead_u, XBMCK_DEAD_U},
                                      {XKB_KEY_dead_U, XBMCK_DEAD_U},
                                      {XKB_KEY_dead_small_schwa, XBMCK_SCHWA},
                                      {XKB_KEY_dead_capital_schwa, XBMCK_SCHWA}});

std::optional<XBMCKey> TranslateDeadKey(uint32_t keySym)
{
  auto mapping = XkbDeadKeyXBMCMapping.find(keySym);
  return mapping != XkbDeadKeyXBMCMapping.cend() ? std::optional<XBMCKey>(mapping->second)
                                                 : std::nullopt;
}

static void xkbLogger(xkb_context* context,
                      xkb_log_level priority,
                      const char* format,
                      va_list args)
{
  const std::string message = StringUtils::FormatV(format, args);
  auto logLevel = logLevelMap.find(priority);
  CLog::Log(logLevel != logLevelMap.cend() ? logLevel->second : LOGDEBUG, "[xkb] {}", message);
}
} // namespace

CLibInputKeyboard::CLibInputKeyboard()
  : m_ctx(std::unique_ptr<xkb_context, XkbContextDeleter>{xkb_context_new(XKB_CONTEXT_NO_FLAGS)}),
    m_repeatTimer(std::bind(&CLibInputKeyboard::KeyRepeatTimeout, this))
{
  if (!m_ctx)
  {
    CLog::Log(LOGERROR, "CLibInputKeyboard::{} - failed to create xkb context", __FUNCTION__);
    return;
  }

  xkb_context_set_log_level(m_ctx.get(), XKB_LOG_LEVEL_DEBUG);
  xkb_context_set_log_fn(m_ctx.get(), &xkbLogger);

  std::string layout = CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(
      CLibInputSettings::SETTING_INPUT_LIBINPUTKEYBOARDLAYOUT);
  if (!SetKeymap(layout))
  {
    CLog::Log(LOGERROR, "CLibInputKeyboard::{} - failed set default keymap", __FUNCTION__);
    return;
  }

  m_composeTable =
      std::unique_ptr<xkb_compose_table, XkbComposeTableDeleter>{xkb_compose_table_new_from_locale(
          m_ctx.get(), g_langInfo.GetSystemLocale().name().c_str(), XKB_COMPOSE_COMPILE_NO_FLAGS)};
  if (!m_composeTable)
  {
    CLog::LogF(LOGWARNING,
               "Failed to compile localized compose table, composed key support will be disabled");
    return;
  }

  m_composedState = std::unique_ptr<xkb_compose_state, XkbComposeStateDeleter>{
      xkb_compose_state_new(m_composeTable.get(), XKB_COMPOSE_STATE_NO_FLAGS)};

  if (!m_composedState)
  {
    throw std::runtime_error("Failed to create keyboard composer");
  }
}

void CLibInputKeyboard::XkbContextDeleter::operator()(xkb_context* ctx) const
{
  xkb_context_unref(ctx);
}

void CLibInputKeyboard::XkbKeymapDeleter::operator()(xkb_keymap* keymap) const
{
  xkb_keymap_unref(keymap);
}

void CLibInputKeyboard::XkbStateDeleter::operator()(xkb_state* state) const
{
  xkb_state_unref(state);
}

void CLibInputKeyboard::XkbComposeTableDeleter::operator()(xkb_compose_table* table) const
{
  xkb_compose_table_unref(table);
}

void CLibInputKeyboard::XkbComposeStateDeleter::operator()(xkb_compose_state* state) const
{
  xkb_compose_state_unref(state);
}

bool CLibInputKeyboard::SetKeymap(const std::string& layout)
{
  xkb_rule_names names;

  names.rules = nullptr;
  names.model = nullptr;
  names.layout = layout.c_str();
  names.variant = nullptr;
  names.options = nullptr;

  m_keymap = std::unique_ptr<xkb_keymap, XkbKeymapDeleter>{
      xkb_keymap_new_from_names(m_ctx.get(), &names, XKB_KEYMAP_COMPILE_NO_FLAGS)};
  if (!m_keymap)
  {
    CLog::Log(LOGERROR, "CLibInputKeyboard::{} - failed to compile keymap", __FUNCTION__);
    return false;
  }

  m_state = std::unique_ptr<xkb_state, XkbStateDeleter>{xkb_state_new(m_keymap.get())};
  if (!m_state)
  {
    CLog::Log(LOGERROR, "CLibInputKeyboard::{} - failed to create xkb state", __FUNCTION__);
    return false;
  }

  m_modindex[0] = xkb_keymap_mod_get_index(m_keymap.get(), XKB_MOD_NAME_CTRL);
  m_modindex[1] = xkb_keymap_mod_get_index(m_keymap.get(), XKB_MOD_NAME_ALT);
  m_modindex[2] = xkb_keymap_mod_get_index(m_keymap.get(), XKB_MOD_NAME_SHIFT);
  m_modindex[3] = xkb_keymap_mod_get_index(m_keymap.get(), XKB_MOD_NAME_LOGO);

  m_ledindex[0] = xkb_keymap_led_get_index(m_keymap.get(), XKB_LED_NAME_NUM);
  m_ledindex[1] = xkb_keymap_led_get_index(m_keymap.get(), XKB_LED_NAME_CAPS);
  m_ledindex[2] = xkb_keymap_led_get_index(m_keymap.get(), XKB_LED_NAME_SCROLL);

  m_leds = 0;

  return true;
}

void CLibInputKeyboard::ProcessKey(libinput_event_keyboard *e)
{
  if (!m_ctx || !m_keymap || !m_state)
    return;

  const uint32_t xkbkey = libinput_event_keyboard_get_key(e) + 8;
  const xkb_keysym_t keysym = xkb_state_key_get_one_sym(m_state.get(), xkbkey);
  const bool pressed = libinput_event_keyboard_get_key_state(e) == LIBINPUT_KEY_STATE_PRESSED;
  xkb_state_update_key(m_state.get(), xkbkey, pressed ? XKB_KEY_DOWN : XKB_KEY_UP);

  bool flushComposer{false};
  if (pressed && SupportsKeyComposition())
  {
    xkb_compose_state_feed(m_composedState.get(), keysym);
    const xkb_compose_status composeStatus = xkb_compose_state_get_status(m_composedState.get());
    switch (composeStatus)
    {
      case XKB_COMPOSE_COMPOSING:
      {
        const std::optional<XBMCKey> xbmcKey = TranslateDeadKey(keysym);
        if (xbmcKey)
        {
          NotifyKeyComposingEvent(XBMC_KEYCOMPOSING_COMPOSING, xbmcKey.value());
        }
        return;
      }
      case XKB_COMPOSE_COMPOSED:
      {
        flushComposer = true;
        NotifyKeyComposingEvent(XBMC_KEYCOMPOSING_FINISHED, XBMCK_UNKNOWN);
        break;
      }
      case XKB_COMPOSE_CANCELLED:
      {
        const std::uint32_t unicodeCodePointCancellationKey{UnicodeCodepointForKeycode(xkbkey)};
        NotifyKeyComposingEvent(XBMC_KEYCOMPOSING_CANCELLED, unicodeCodePointCancellationKey);
        xkb_compose_state_reset(m_composedState.get());
        // do not allow key fallthrough if we are simply cancelling with a backspace (we are cancelling the
        // composition behavior not really removing any character)
        if (unicodeCodePointCancellationKey == XBMCK_BACKSPACE)
        {
          return;
        }
        break;
      }
      default:
        break;
    }
  }

  int mod = XBMCKMOD_NONE;

  xkb_state_component modtype = xkb_state_component(XKB_STATE_MODS_DEPRESSED | XKB_STATE_MODS_LATCHED);
  if (xkb_state_mod_index_is_active(m_state.get(), m_modindex[0], modtype) &&
      ((keysym != XBMCK_LCTRL) || !pressed))
    mod |= XBMCKMOD_CTRL;
  if (xkb_state_mod_index_is_active(m_state.get(), m_modindex[0], modtype) &&
      ((keysym != XBMCK_RCTRL) || !pressed))
    mod |= XBMCKMOD_CTRL;
  if (xkb_state_mod_index_is_active(m_state.get(), m_modindex[1], modtype) &&
      ((keysym != XBMCK_LALT) || !pressed))
    mod |= XBMCKMOD_ALT;
  if (xkb_state_mod_index_is_active(m_state.get(), m_modindex[1], modtype) &&
      ((keysym != XBMCK_RALT) || !pressed))
    mod |= XBMCKMOD_ALT;
  if (xkb_state_mod_index_is_active(m_state.get(), m_modindex[2], modtype) &&
      ((keysym != XBMCK_LSHIFT) || !pressed))
    mod |= XBMCKMOD_SHIFT;
  if (xkb_state_mod_index_is_active(m_state.get(), m_modindex[2], modtype) &&
      ((keysym != XBMCK_RSHIFT) || !pressed))
    mod |= XBMCKMOD_SHIFT;
  if (xkb_state_mod_index_is_active(m_state.get(), m_modindex[3], modtype) &&
      ((keysym != XBMCK_LMETA) || !pressed))
    mod |= XBMCKMOD_META;
  if (xkb_state_mod_index_is_active(m_state.get(), m_modindex[3], modtype) &&
      ((keysym != XBMCK_RMETA) || !pressed))
    mod |= XBMCKMOD_META;

  m_leds = 0;

  if (xkb_state_led_index_is_active(m_state.get(), m_ledindex[0]) &&
      ((keysym != XBMCK_NUMLOCK) || !pressed))
  {
    m_leds |= LIBINPUT_LED_NUM_LOCK;
    mod |= XBMCKMOD_NUM;
  }
  if (xkb_state_led_index_is_active(m_state.get(), m_ledindex[1]) &&
      ((keysym != XBMCK_CAPSLOCK) || !pressed))
  {
    m_leds |= LIBINPUT_LED_CAPS_LOCK;
    mod |= XBMCKMOD_CAPS;
  }
  if (xkb_state_led_index_is_active(m_state.get(), m_ledindex[2]) &&
      ((keysym != XBMCK_SCROLLOCK) || !pressed))
  {
    m_leds |= LIBINPUT_LED_SCROLL_LOCK;
    //mod |= XBMCK_SCROLLOCK;
  }

  uint32_t unicode = UnicodeCodepointForKeycode(xkbkey);
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

  // flush composer if set (after a finished sequence)
  if (flushComposer)
  {
    xkb_compose_state_reset(m_composedState.get());
  }

  XBMC_Event event = {};
  event.type = pressed ? XBMC_KEYDOWN : XBMC_KEYUP;
  event.key.keysym.mod = XBMCMod(mod);
  event.key.keysym.sym = XBMCKeyForKeysym(keysym, scancode);
  event.key.keysym.scancode = scancode;
  event.key.keysym.unicode = unicode;

  std::shared_ptr<CAppInboundProtocol> appPort = CServiceBroker::GetAppPort();
  if (appPort)
    appPort->OnEvent(event);

  if (pressed && xkb_keymap_key_repeats(m_keymap.get(), xkbkey))
  {
    libinput_event *ev = libinput_event_keyboard_get_base_event(e);
    libinput_device *dev = libinput_event_get_device(ev);
    auto data = m_repeatData.find(dev);
    if (data != m_repeatData.end())
    {
      CLog::Log(LOGDEBUG, "CLibInputKeyboard::{} - using delay: {}ms repeat: {}ms", __FUNCTION__,
                data->second.at(0), data->second.at(1));

      m_repeatRate = data->second.at(1);
      m_repeatTimer.Stop(true);
      m_repeatEvent = event;
      m_repeatTimer.Start(std::chrono::milliseconds(data->second.at(0)), false);
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
  if (xkbmapping != xkbMap.cend())
    return xkbmapping->second;

  return XBMCK_UNKNOWN;
}

void CLibInputKeyboard::KeyRepeatTimeout()
{
  m_repeatTimer.RestartAsync(std::chrono::milliseconds(m_repeatRate));

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
  int kbdrep[2] = {REPEAT_DELAY, REPEAT_RATE};
  const char *name = libinput_device_get_name(dev);
  const char *sysname = libinput_device_get_sysname(dev);
  std::string path("/dev/input/");
  path.append(sysname);
  auto fd = open(path.c_str(), O_RDONLY);

  if (fd < 0)
  {
    CLog::Log(LOGERROR, "CLibInputKeyboard::{} - failed to open {} ({})", __FUNCTION__, sysname,
              strerror(errno));
  }
  else
  {
    auto ret = ioctl(fd, EVIOCGREP, &kbdrep);
    if (ret < 0)
      CLog::Log(LOGDEBUG, "CLibInputKeyboard::{} - could not get key repeat for {} ({})",
                __FUNCTION__, sysname, strerror(errno));

    CLog::Log(LOGDEBUG, "CLibInputKeyboard::{} - delay: {}ms repeat: {}ms for {} ({})",
              __FUNCTION__, kbdrep[0], kbdrep[1], name, sysname);
    close(fd);
  }

  std::vector<int> kbdrepvec(std::begin(kbdrep), std::end(kbdrep));

  auto data = m_repeatData.find(dev);
  if (data == m_repeatData.end())
  {
    m_repeatData.insert(std::make_pair(dev, kbdrepvec));
  }
}

bool CLibInputKeyboard::SupportsKeyComposition() const
{
  return m_composedState != nullptr;
}

void CLibInputKeyboard::NotifyKeyComposingEvent(uint8_t eventType, std::uint16_t unicodeCodepoint)
{
  XBMC_Event event{};
  event.type = eventType;
  event.key.keysym.unicode = unicodeCodepoint;
  std::shared_ptr<CAppInboundProtocol> appPort = CServiceBroker::GetAppPort();
  if (appPort)
  {
    appPort->OnEvent(event);
  }
}

std::uint32_t CLibInputKeyboard::UnicodeCodepointForKeycode(xkb_keycode_t code) const
{
  uint32_t unicode;

  if (SupportsKeyComposition())
  {
    const xkb_compose_status composerStatus = xkb_compose_state_get_status(m_composedState.get());
    if (composerStatus == XKB_COMPOSE_COMPOSED)
    {
      const uint32_t keysym = xkb_compose_state_get_one_sym(m_composedState.get());
      unicode = xkb_keysym_to_utf32(keysym);
    }
    else
    {
      unicode = xkb_state_key_get_utf32(m_state.get(), code);
    }
  }
  else
  {
    unicode = xkb_state_key_get_utf32(m_state.get(), code);
  }

  // check if it is a dead key and try to translate
  if (unicode == XBMCK_UNKNOWN)
  {
    const uint32_t keysym = xkb_state_key_get_one_sym(m_state.get(), code);
    const std::optional<XBMCKey> xbmcKey = TranslateDeadKey(keysym);
    if (xbmcKey)
    {
      unicode = xbmcKey.value();
    }
  }
  return unicode;
}
