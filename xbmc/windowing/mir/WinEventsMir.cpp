/*
 *      Copyright (C) 2016 Canonical Ltd.
 *      brandon.schaefer@canonical.com
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


#include "WinEventsMir.h"

#include <unordered_map>
#include <mir_toolkit/mir_client_library.h>
#include <xkbcommon/xkbcommon-keysyms.h>

#include "AppInboundProtocol.h"
#include "ServiceBroker.h"
#include "input/mouse/MouseStat.h"
#include "input/Key.h"

namespace
{

// XkbCommon keysym to xkbmc
std::unordered_map<uint32_t, uint32_t> sym_mapping_xkb =
{
  {XKB_KEY_BackSpace, XBMCK_BACKSPACE}
, {XKB_KEY_Tab,       XBMCK_TAB}
, {XKB_KEY_Clear,     XBMCK_CLEAR}
, {XKB_KEY_Return,    XBMCK_RETURN}
, {XKB_KEY_Pause,     XBMCK_PAUSE}
, {XKB_KEY_Escape,    XBMCK_ESCAPE}
, {XKB_KEY_Delete,    XBMCK_DELETE}
// multi-media keys
, {XKB_KEY_XF86Back,      XBMCK_BROWSER_BACK}
, {XKB_KEY_XF86Forward,   XBMCK_BROWSER_FORWARD}
, {XKB_KEY_XF86Refresh,   XBMCK_BROWSER_REFRESH}
, {XKB_KEY_XF86Stop,      XBMCK_BROWSER_STOP}
, {XKB_KEY_XF86Search,    XBMCK_BROWSER_SEARCH}
, {XKB_KEY_XF86Favorites, XBMCK_BROWSER_FAVORITES}
, {XKB_KEY_XF86HomePage,  XBMCK_BROWSER_HOME}
, {XKB_KEY_XF86AudioMute, XBMCK_VOLUME_MUTE}
, {XKB_KEY_XF86AudioLowerVolume, XBMCK_VOLUME_DOWN}
, {XKB_KEY_XF86AudioRaiseVolume, XBMCK_VOLUME_UP}
, {XKB_KEY_XF86AudioNext,  XBMCK_MEDIA_NEXT_TRACK}
, {XKB_KEY_XF86AudioPrev,  XBMCK_MEDIA_PREV_TRACK}
, {XKB_KEY_XF86AudioStop,  XBMCK_MEDIA_STOP}
, {XKB_KEY_XF86AudioPause, XBMCK_MEDIA_PLAY_PAUSE}
, {XKB_KEY_XF86Mail,       XBMCK_LAUNCH_MAIL}
, {XKB_KEY_XF86Select,     XBMCK_LAUNCH_MEDIA_SELECT}
, {XKB_KEY_XF86Launch0,    XBMCK_LAUNCH_APP1}
, {XKB_KEY_XF86Launch1,    XBMCK_LAUNCH_APP2}
, {XKB_KEY_XF86WWW,        XBMCK_LAUNCH_FILE_BROWSER}
, {XKB_KEY_XF86AudioMedia, XBMCK_LAUNCH_MEDIA_CENTER }
  // Numeric keypad
, {XKB_KEY_KP_0, XBMCK_KP0}
, {XKB_KEY_KP_1, XBMCK_KP1}
, {XKB_KEY_KP_2, XBMCK_KP2}
, {XKB_KEY_KP_3, XBMCK_KP3}
, {XKB_KEY_KP_4, XBMCK_KP4}
, {XKB_KEY_KP_5, XBMCK_KP5}
, {XKB_KEY_KP_6, XBMCK_KP6}
, {XKB_KEY_KP_7, XBMCK_KP7}
, {XKB_KEY_KP_8, XBMCK_KP8}
, {XKB_KEY_KP_9, XBMCK_KP9}
, {XKB_KEY_KP_Decimal,  XBMCK_KP_PERIOD}
, {XKB_KEY_KP_Divide,   XBMCK_KP_DIVIDE}
, {XKB_KEY_KP_Multiply, XBMCK_KP_MULTIPLY}
, {XKB_KEY_KP_Subtract, XBMCK_KP_MINUS}
, {XKB_KEY_KP_Add,   XBMCK_KP_PLUS}
, {XKB_KEY_KP_Enter, XBMCK_KP_ENTER}
, {XKB_KEY_KP_Equal, XBMCK_KP_EQUALS}
  // Arrows + Home/End pad
, {XKB_KEY_Up,     XBMCK_UP}
, {XKB_KEY_Down,   XBMCK_DOWN}
, {XKB_KEY_Right,  XBMCK_RIGHT}
, {XKB_KEY_Left,   XBMCK_LEFT}
, {XKB_KEY_Insert, XBMCK_INSERT}
, {XKB_KEY_Home,   XBMCK_HOME}
, {XKB_KEY_End,    XBMCK_END}
, {XKB_KEY_Page_Up,   XBMCK_PAGEUP}
, {XKB_KEY_Page_Down, XBMCK_PAGEDOWN}
  // Function keys
, {XKB_KEY_F1, XBMCK_F1}
, {XKB_KEY_F2, XBMCK_F2}
, {XKB_KEY_F3, XBMCK_F3}
, {XKB_KEY_F4, XBMCK_F4}
, {XKB_KEY_F5, XBMCK_F5}
, {XKB_KEY_F6, XBMCK_F6}
, {XKB_KEY_F7, XBMCK_F7}
, {XKB_KEY_F8, XBMCK_F8}
, {XKB_KEY_F9, XBMCK_F9}
, {XKB_KEY_F10, XBMCK_F10}
, {XKB_KEY_F11, XBMCK_F11}
, {XKB_KEY_F12, XBMCK_F12}
, {XKB_KEY_F13, XBMCK_F13}
, {XKB_KEY_F14, XBMCK_F14}
, {XKB_KEY_F15, XBMCK_F15}
  // Key state modifier keys
, {XKB_KEY_Num_Lock,    XBMCK_NUMLOCK}
, {XKB_KEY_Caps_Lock,   XBMCK_CAPSLOCK}
, {XKB_KEY_Scroll_Lock, XBMCK_SCROLLOCK}
, {XKB_KEY_Shift_R,   XBMCK_RSHIFT}
, {XKB_KEY_Shift_L,   XBMCK_LSHIFT}
, {XKB_KEY_Control_R, XBMCK_RCTRL}
, {XKB_KEY_Control_L, XBMCK_LCTRL}
, {XKB_KEY_Alt_R,   XBMCK_RALT}
, {XKB_KEY_Alt_L,   XBMCK_LALT}
, {XKB_KEY_Meta_R,  XBMCK_RMETA}
, {XKB_KEY_Meta_L,  XBMCK_LMETA}
, {XKB_KEY_Super_L, XBMCK_LSUPER}
, {XKB_KEY_Super_R, XBMCK_RSUPER}
, {XKB_KEY_Mode_switch, XBMCK_MODE}
, {XKB_KEY_Multi_key,   XBMCK_COMPOSE}
  // Miscellaneous function keys
, {XKB_KEY_Help,  XBMCK_HELP}
, {XKB_KEY_Print, XBMCK_PRINT}
  //, {0, XBMCK_SYSREQ}
, {XKB_KEY_Break, XBMCK_BREAK}
, {XKB_KEY_Menu,  XBMCK_MENU}
, {XKB_KEY_XF86PowerOff, XBMCK_POWER}
, {XKB_KEY_XF86Sleep, XBMCK_SLEEP}
, {XKB_KEY_EcuSign,   XBMCK_EURO}
, {XKB_KEY_Undo,      XBMCK_UNDO}
  // Media keys
, {XKB_KEY_XF86Eject,  XBMCK_EJECT}
, {XKB_KEY_XF86Stop,   XBMCK_STOP}
, {XKB_KEY_XF86AudioRecord, XBMCK_RECORD}
, {XKB_KEY_XF86AudioRewind, XBMCK_REWIND}
, {XKB_KEY_XF86Phone,     XBMCK_PHONE}
, {XKB_KEY_XF86AudioPlay, XBMCK_PLAY}
, {XKB_KEY_XF86AudioRandomPlay, XBMCK_SHUFFLE}
, {XKB_KEY_XF86AudioForward,    XBMCK_FASTFORWARD}
};

void MirHandlePointerButton(MirPointerEvent const* pev, unsigned char type)
{
  auto x = mir_pointer_event_axis_value(pev, mir_pointer_axis_x);
  auto y = mir_pointer_event_axis_value(pev, mir_pointer_axis_y);

  MirPointerButton button_state = mir_pointer_button_primary;
  static uint32_t old_button_states = 0;
  uint32_t new_button_states = mir_pointer_event_buttons(pev);
  unsigned char xbmc_button = XBMC_BUTTON_LEFT;

  button_state = MirPointerButton(new_button_states ^ old_button_states);

  switch (button_state)
  {
    case mir_pointer_button_primary:
      xbmc_button = XBMC_BUTTON_LEFT;
      break;
    case mir_pointer_button_secondary:
      xbmc_button = XBMC_BUTTON_RIGHT;
      break;
    case mir_pointer_button_tertiary:
      xbmc_button = XBMC_BUTTON_MIDDLE;
      break;
    case mir_pointer_button_forward:
      xbmc_button = XBMC_BUTTON_X1;
      break;
    case mir_pointer_button_back:
      xbmc_button = XBMC_BUTTON_X2;
       break;
    default:
      break;
  }

  old_button_states = new_button_states;

  XBMC_Event new_event;
  memset(&new_event, 0, sizeof(new_event));

  new_event.button.button = xbmc_button;
  new_event.button.x = x;
  new_event.button.y = y;

  CWinEvents::MessagePush(&new_event);
}

void MirHandlePointerEvent(MirPointerEvent const* pev)
{
  switch (mir_pointer_event_action(pev))
  {
    case mir_pointer_action_button_down:
      MirHandlePointerButton(pev, XBMC_MOUSEBUTTONDOWN);
      break;
    case mir_pointer_action_button_up:
      MirHandlePointerButton(pev, XBMC_MOUSEBUTTONUP);
      break;
    case mir_pointer_action_motion:
    {
      XBMC_Event new_event;
      memset(&new_event, 0, sizeof(new_event));

      auto x  = mir_pointer_event_axis_value(pev, mir_pointer_axis_x);
      auto y  = mir_pointer_event_axis_value(pev, mir_pointer_axis_y);

      new_event.type = XBMC_MOUSEMOTION;
      new_event.motion.x = x;
      new_event.motion.y = y;

      CWinEvents::MessagePush(&new_event);
      break;
    }
    default:
      break;
  }
}

XBMCMod MirModToXBMCMode(MirInputEventModifiers mir_mod)
{
  int mod = XBMCKMOD_NONE;

  if (mir_mod & mir_input_event_modifier_shift_left)
    mod |= XBMCKMOD_LSHIFT;
  if (mir_mod & mir_input_event_modifier_shift_right)
    mod |= XBMCKMOD_RSHIFT;
  if (mir_mod & mir_input_event_modifier_meta_left)
    mod |= XBMCKMOD_LSUPER;
  if (mir_mod & mir_input_event_modifier_meta_right)
    mod |= XBMCKMOD_RSUPER;
  if (mir_mod & mir_input_event_modifier_ctrl_left)
    mod |= XBMCKMOD_LCTRL;
  if (mir_mod & mir_input_event_modifier_ctrl_right)
    mod |= XBMCKMOD_RCTRL;
  if (mir_mod & mir_input_event_modifier_alt_left)
    mod |= XBMCKMOD_LALT;
  if (mir_mod & mir_input_event_modifier_alt_right)
    mod |= XBMCKMOD_RALT;
  if (mir_mod & mir_input_event_modifier_num_lock)
    mod |= XBMCKMOD_NUM;
  if (mir_mod & mir_input_event_modifier_caps_lock)
    mod |= XBMCKMOD_CAPS;

  return XBMCMod(mod);
}

void MirHandleKeyboardEvent(MirKeyboardEvent const* kev)
{
  auto action = mir_keyboard_event_action(kev);

  XBMC_Event new_event;
  memset(&new_event, 0, sizeof(new_event));

  if (action == mir_keyboard_action_down)
  {
    new_event.type = XBMC_KEYDOWN;
  }
  else
  {
    new_event.type = XBMC_KEYUP;
  }

  auto keysym = mir_keyboard_event_key_code(kev);
  auto xkb_keysym = sym_mapping_xkb.find(keysym);
  if (xkb_keysym != sym_mapping_xkb.end())
  {
    keysym = xkb_keysym->second;
  }

  new_event.key.keysym.sym = XBMCKey(keysym);
  new_event.key.keysym.mod = MirModToXBMCMode(mir_keyboard_event_modifiers(kev));
  new_event.key.keysym.scancode = mir_keyboard_event_scan_code(kev);

  CWinEvents::MessagePush(&new_event);
}

// Lets just handle a tap on an action up
void MirHandleTouchEvent(MirTouchEvent const* tev)
{
  XBMC_Event new_event;
  memset(&new_event, 0, sizeof(new_event));
  new_event.type = XBMC_TOUCH;

  auto pointer_count = mir_touch_event_point_count(tev);
  auto action = mir_touch_event_action(tev, 0);

  if (action == mir_touch_action_up)
  {
    // FIXME Need to test this... since reading
    // the input manager it just turns this tap into a mouse motion?
    // Does this send a up/down?
    new_event.touch.action = ACTION_TOUCH_TAP;

    new_event.touch.x = mir_touch_event_axis_value(tev, 0, mir_touch_axis_x);
    new_event.touch.y = mir_touch_event_axis_value(tev, 0, mir_touch_axis_y);

    new_event.touch.pointers = pointer_count;
    CWinEvents::MessagePush(&new_event);
  }
}

void MirHandleInput(MirInputEvent const* iev)
{
  switch (mir_input_event_get_type(iev))
  {
    case mir_input_event_type_key:
      MirHandleKeyboardEvent(mir_input_event_get_keyboard_event(iev));
      break;
    case mir_input_event_type_pointer:
      MirHandlePointerEvent(mir_input_event_get_pointer_event(iev));
      break;
    case mir_input_event_type_touch:
      MirHandleTouchEvent(mir_input_event_get_touch_event(iev));
      break;
    default:
      break;
  }
}
}

void MirHandleEvent(MirWindow* window, MirEvent const* ev, void* context)
{
  MirEventType event_type = mir_event_get_type(ev);
  switch (event_type)
  {
    case mir_event_type_input:
      MirHandleInput(mir_event_get_input_event(ev));
      break;
    default:
      break;
  }
}

bool CWinEventsMir::MessagePump()
{
  auto ret = GetQueueSize();
  std::shared_ptr<CAppInboundProtocol> appPort = CServiceBroker::GetAppPort();

  while (GetQueueSize())
  {
    XBMC_Event e;
    {
      std::lock_guard<decltype(m_mutex)> event_lock(m_mutex);
      e = m_events.front();
      m_events.pop();
    }

    if (appPort)
      appPort->OnEvent(newEvent);
  }

  return ret;
}

size_t CWinEventsMir::GetQueueSize()
{
  std::lock_guard<decltype(m_mutex)> event_lock(m_mutex);
  return m_events.size();
}

void CWinEventsMir::MessagePush(XBMC_Event* ev)
{
  std::lock_guard<decltype(m_mutex)> event_lock(m_mutex);
  m_events.push(*ev);
}
