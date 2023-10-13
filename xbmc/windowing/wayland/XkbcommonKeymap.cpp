/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "XkbcommonKeymap.h"

#include "Util.h"
#include "utils/log.h"

#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <vector>

using namespace KODI::WINDOWING::WAYLAND;

namespace
{

struct ModifierNameXBMCMapping
{
  const char* name;
  XBMCMod xbmc;
};

static const std::vector<ModifierNameXBMCMapping> ModifierNameXBMCMappings = {
    {XKB_MOD_NAME_CTRL, XBMCKMOD_LCTRL},
    {XKB_MOD_NAME_SHIFT, XBMCKMOD_LSHIFT},
    {XKB_MOD_NAME_LOGO, XBMCKMOD_LSUPER},
    {XKB_MOD_NAME_ALT, XBMCKMOD_LALT},
    {"Meta", XBMCKMOD_LMETA},
    {"RControl", XBMCKMOD_RCTRL},
    {"RShift", XBMCKMOD_RSHIFT},
    {"Hyper", XBMCKMOD_RSUPER},
    {"AltGr", XBMCKMOD_RALT},
    {XKB_LED_NAME_CAPS, XBMCKMOD_CAPS},
    {XKB_LED_NAME_NUM, XBMCKMOD_NUM},
    {XKB_LED_NAME_SCROLL, XBMCKMOD_MODE}};

static const std::map<xkb_keycode_t, XBMCKey> XkbKeycodeXBMCMappings = {
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

    // Function keys
    {XKB_KEY_F1, XBMCK_F1},
    {XKB_KEY_F2, XBMCK_F2},
    {XKB_KEY_F3, XBMCK_F3},
    {XKB_KEY_F4, XBMCK_F4},
    {XKB_KEY_F5, XBMCK_F5},
    {XKB_KEY_F6, XBMCK_F6},
    {XKB_KEY_F7, XBMCK_F7},
    {XKB_KEY_F8, XBMCK_F8},
    {XKB_KEY_F9, XBMCK_F9},
    {XKB_KEY_F10, XBMCK_F10},
    {XKB_KEY_F11, XBMCK_F11},
    {XKB_KEY_F12, XBMCK_F12},
    {XKB_KEY_F13, XBMCK_F13},
    {XKB_KEY_F14, XBMCK_F14},
    {XKB_KEY_F15, XBMCK_F15},

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
    // Unmapped: XBMCK_SYSREQ
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
    // XBMCK_STOP clashes with XBMCK_MEDIA_STOP
    {XKB_KEY_XF86AudioRecord, XBMCK_RECORD},
    // XBMCK_REWIND clashes with XBMCK_MEDIA_REWIND
    {XKB_KEY_XF86Phone, XBMCK_PHONE},
    {XKB_KEY_XF86AudioPlay, XBMCK_PLAY},
    {XKB_KEY_XF86AudioRandomPlay, XBMCK_SHUFFLE}
    // XBMCK_FASTFORWARD clashes with XBMCK_MEDIA_FASTFORWARD

#if defined(TARGET_WEBOS)
    // WebOS remote
    ,
    {XKB_KEY_WEBOS_BLUE, XBMCK_BLUE},
    {XKB_KEY_WEBOS_GREEN, XBMCK_GREEN},
    {XKB_KEY_WEBOS_YELLOW, XBMCK_YELLOW},
    {XKB_KEY_WEBOS_RED, XBMCK_RED},
    {XKB_KEY_WEBOS_BACK, XBMCK_BROWSER_BACK},
    {XKB_KEY_WEBOS_MEDIA_PLAY, XBMCK_PLAY},
    {XKB_KEY_WEBOS_MEDIA_PAUSE, XBMCK_PAUSE},
    {XKB_KEY_WEBOS_MEDIA_STOP, XBMCK_STOP},
    {XKB_KEY_WEBOS_MEDIA_RECORD, XBMCK_RECORD},
    {XKB_KEY_WEBOS_MEDIA_PREVIOUS, XBMCK_MEDIA_PREV_TRACK},
    {XKB_KEY_WEBOS_MEDIA_NEXT, XBMCK_MEDIA_NEXT_TRACK},
    {XKB_KEY_WEBOS_CHANNEL_DOWN, XBMCK_PAGEDOWN},
    {XKB_KEY_WEBOS_CHANNEL_UP, XBMCK_PAGEUP},
    {XKB_KEY_WEBOS_CURSOR_HIDE, XBMCK_UNKNOWN},
    {XKB_KEY_WEBOS_CURSOR_SHOW, XBMCK_UNKNOWN},
    {XKB_KEY_WEBOS_INVALID, XBMCK_UNKNOWN},
#endif
};
}

CXkbcommonContext::CXkbcommonContext(xkb_context_flags flags)
: m_context{xkb_context_new(flags), XkbContextDeleter()}
{
  if (!m_context)
  {
    throw std::runtime_error("Failed to create xkb context");
  }
}

void CXkbcommonContext::XkbContextDeleter::operator()(xkb_context* ctx) const
{
  xkb_context_unref(ctx);
}

std::unique_ptr<CXkbcommonKeymap> CXkbcommonContext::KeymapFromString(std::string const& keymap)
{
  std::unique_ptr<xkb_keymap, CXkbcommonKeymap::XkbKeymapDeleter> xkbKeymap{xkb_keymap_new_from_string(m_context.get(), keymap.c_str(), XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS), CXkbcommonKeymap::XkbKeymapDeleter()};

  if (!xkbKeymap)
  {
    throw std::runtime_error("Failed to compile keymap");
  }

  return std::make_unique<CXkbcommonKeymap>(std::move(xkbKeymap));
}

std::unique_ptr<CXkbcommonKeymap> CXkbcommonContext::KeymapFromNames(const std::string& rules, const std::string& model, const std::string& layout, const std::string& variant, const std::string& options)
{
  xkb_rule_names names = {
    rules.c_str(),
    model.c_str(),
    layout.c_str(),
    variant.c_str(),
    options.c_str()
  };

  std::unique_ptr<xkb_keymap, CXkbcommonKeymap::XkbKeymapDeleter> keymap{xkb_keymap_new_from_names(m_context.get(), &names, XKB_KEYMAP_COMPILE_NO_FLAGS), CXkbcommonKeymap::XkbKeymapDeleter()};

  if (!keymap)
  {
    throw std::runtime_error("Failed to compile keymap");
  }

  return std::make_unique<CXkbcommonKeymap>(std::move(keymap));
}

std::unique_ptr<xkb_state, CXkbcommonKeymap::XkbStateDeleter> CXkbcommonKeymap::CreateXkbStateFromKeymap(xkb_keymap* keymap)
{
  std::unique_ptr<xkb_state, XkbStateDeleter> state{xkb_state_new(keymap), XkbStateDeleter()};

  if (!state)
  {
    throw std::runtime_error("Failed to create keyboard state");
  }

  return state;
}

CXkbcommonKeymap::CXkbcommonKeymap(std::unique_ptr<xkb_keymap, XkbKeymapDeleter> keymap)
: m_keymap{std::move(keymap)}, m_state{CreateXkbStateFromKeymap(m_keymap.get())}
{
  // Lookup modifier indices and create new map - this is more efficient
  // than looking the modifiers up by name each time
  for (auto const& nameMapping : ModifierNameXBMCMappings)
  {
    xkb_mod_index_t index = xkb_keymap_mod_get_index(m_keymap.get(), nameMapping.name);
    if (index != XKB_MOD_INVALID)
    {
      m_modifierMappings.emplace_back(index, nameMapping.xbmc);
    }
  }
}

void CXkbcommonKeymap::XkbStateDeleter::operator()(xkb_state* state) const
{
  xkb_state_unref(state);
}

void CXkbcommonKeymap::XkbKeymapDeleter::operator()(xkb_keymap* keymap) const
{
  xkb_keymap_unref(keymap);
}

xkb_keysym_t CXkbcommonKeymap::KeysymForKeycode(xkb_keycode_t code) const
{
  return xkb_state_key_get_one_sym(m_state.get(), code);
}

xkb_mod_mask_t CXkbcommonKeymap::CurrentModifiers() const
{
  return xkb_state_serialize_mods(m_state.get(), XKB_STATE_MODS_EFFECTIVE);
}

void CXkbcommonKeymap::UpdateMask(xkb_mod_mask_t depressed, xkb_mod_mask_t latched, xkb_mod_mask_t locked, xkb_mod_mask_t group)
{
  xkb_state_update_mask(m_state.get(), depressed, latched, locked, 0, 0, group);
}

XBMCMod CXkbcommonKeymap::ActiveXBMCModifiers() const
{
  xkb_mod_mask_t mask(CurrentModifiers());
  XBMCMod xbmcModifiers = XBMCKMOD_NONE;

  for (auto const& mapping : m_modifierMappings)
  {
    if (mask & (1 << mapping.xkb))
    {
      xbmcModifiers = static_cast<XBMCMod> (xbmcModifiers | mapping.xbmc);
    }
  }

  return xbmcModifiers;
}

XBMCKey CXkbcommonKeymap::XBMCKeyForKeysym(xkb_keysym_t sym)
{
  if (sym >= 'A' && sym <= 'Z')
  {
    // Uppercase ASCII characters must be lowercased as XBMCKey is modifier-invariant
    return static_cast<XBMCKey> (sym + 'a' - 'A');
  }
  else if (sym >= 0x20 /* ASCII space */ && sym <= 0x7E /* ASCII tilde */)
  {
    // Rest of ASCII printable character range is code-compatible
    return static_cast<XBMCKey> (sym);
  }

  // Try mapping
  auto mapping = XkbKeycodeXBMCMappings.find(sym);
  if (mapping != XkbKeycodeXBMCMappings.end())
  {
    return mapping->second;
  }
  else
  {
    return XBMCK_UNKNOWN;
  }
}

XBMCKey CXkbcommonKeymap::XBMCKeyForKeycode(xkb_keycode_t code) const
{
  return XBMCKeyForKeysym(KeysymForKeycode(code));
}

std::uint32_t CXkbcommonKeymap::UnicodeCodepointForKeycode(xkb_keycode_t code) const
{
  return xkb_state_key_get_utf32(m_state.get(), code);
}

bool CXkbcommonKeymap::ShouldKeycodeRepeat(xkb_keycode_t code) const
{
  return xkb_keymap_key_repeats(m_keymap.get(), code);
}
