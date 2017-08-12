/*
 *      Copyright (C) 2017 Team XBMC
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

#include "InputProcessorKeyboard.h"

#include <unistd.h>

#include <cassert>
#include <limits>

#include "utils/log.h"
#include "utils/posix/FileHandle.h"

using namespace KODI::WINDOWING::WAYLAND;

namespace
{
// Offset between keyboard codes of Wayland (effectively evdev) and xkb_keycode_t
constexpr int WL_KEYBOARD_XKB_CODE_OFFSET{8};
}

CInputProcessorKeyboard::CInputProcessorKeyboard(wayland::keyboard_t const& keyboard, IInputHandlerKeyboard& handler)
: m_keyboard{keyboard}, m_handler{handler}, m_keyRepeatTimer{std::bind(&CInputProcessorKeyboard::KeyRepeatTimeout, this)}
{
  m_keyboard.on_enter() = [this](std::uint32_t, wayland::surface_t, wayland::array_t)
  {
    m_handler.OnKeyboardEnter();
  };
  m_keyboard.on_leave() = [this](std::uint32_t, wayland::surface_t)
  {
    m_keyRepeatTimer.Stop();
    m_handler.OnKeyboardLeave();
  };
  m_keyboard.on_repeat_info() = [this](std::int32_t rate, std::int32_t delay)
  {
    CLog::Log(LOGDEBUG, "Key repeat rate: %d cps, delay %d ms", rate, delay);
    // rate is in characters per second, so convert to msec interval
    m_keyRepeatInterval = (rate != 0) ? static_cast<int> (1000.0f / rate) : 0;
    m_keyRepeatDelay = delay;
  };
  m_keyboard.on_keymap() = [this](wayland::keyboard_keymap_format format, int fd, std::uint32_t size)
  {
    KODI::UTILS::POSIX::CFileHandle fdGuard{fd};

    if (format != wayland::keyboard_keymap_format::xkb_v1)
    {
      CLog::Log(LOGWARNING, "Wayland compositor sent keymap in format %u, but we only understand xkbv1 - keyboard input will not work", format);
      return;
    }

    m_keyRepeatTimer.Stop();

    try
    {
      if (!m_xkbContext)
      {
        // Lazily initialize XkbcommonContext
        m_xkbContext.reset(new CXkbcommonContext);
      }

      m_keymap = m_xkbContext->KeymapFromSharedMemory(fd, size);
    }
    catch(std::exception const& e)
    {
      CLog::Log(LOGERROR, "Could not parse keymap from compositor: %s - continuing without keymap", e.what());
    }
  };
  m_keyboard.on_key() = [this](std::uint32_t, std::uint32_t, std::uint32_t key, wayland::keyboard_key_state state)
  {
    if (!m_keymap)
    {
      CLog::Log(LOGWARNING, "Key event for code %u without valid keymap, ignoring", key);
      return;
    }

    ConvertAndSendKey(key, state == wayland::keyboard_key_state::pressed);
  };
  m_keyboard.on_modifiers() = [this](std::uint32_t, std::uint32_t modsDepressed, std::uint32_t modsLatched, std::uint32_t modsLocked, std::uint32_t group)
  {
    if (!m_keymap)
    {
      CLog::Log(LOGWARNING, "Modifier event without valid keymap, ignoring");
      return;
    }

    m_keyRepeatTimer.Stop();
    m_keymap->UpdateMask(modsDepressed, modsLatched, modsLocked, group);
  };
}

void CInputProcessorKeyboard::ConvertAndSendKey(std::uint32_t scancode, bool pressed)
{
  std::uint32_t xkbCode{scancode + WL_KEYBOARD_XKB_CODE_OFFSET};
  XBMCKey xbmcKey{m_keymap->XBMCKeyForKeycode(xkbCode)};
  std::uint32_t utf32{m_keymap->UnicodeCodepointForKeycode(xkbCode)};

  if (utf32 > std::numeric_limits<std::uint16_t>::max())
  {
    // Kodi event system only supports UTF16, so ignore the codepoint if
    // it does not fit
    utf32 = 0;
  }
  if (scancode > std::numeric_limits<unsigned char>::max())
  {
    // Kodi scancodes are limited to unsigned char, pretend the scancode is unknown
    // on overflow
    scancode = 0;
  }

  XBMC_Event event{SendKey(scancode, xbmcKey, static_cast<std::uint16_t> (utf32), pressed)};

  if (pressed && m_keymap->ShouldKeycodeRepeat(xkbCode) && m_keyRepeatInterval > 0)
  {
    // Can't modify keyToRepeat until we're sure the thread isn't accessing it
    m_keyRepeatTimer.Stop(true);
    // Update/Set key
    m_keyToRepeat = event;
    // Start timer with initial delay
    m_keyRepeatTimer.Start(m_keyRepeatDelay, false);
  }
  else
  {
    m_keyRepeatTimer.Stop();
  }
}

XBMC_Event CInputProcessorKeyboard::SendKey(unsigned char scancode, XBMCKey key, std::uint16_t unicodeCodepoint, bool pressed)
{
  assert(m_keymap);

  XBMC_Event event{static_cast<unsigned char> (pressed ? XBMC_KEYDOWN : XBMC_KEYUP)};
  event.key.keysym =
  {
    .scancode = scancode,
    .sym = key,
    .mod = m_keymap->ActiveXBMCModifiers(),
    .unicode = unicodeCodepoint
  };
  m_handler.OnKeyboardEvent(event);
  // Return created event for convenience (key repeat)
  return event;
}

void CInputProcessorKeyboard::KeyRepeatTimeout()
{
  // Reset ourselves
  m_keyRepeatTimer.RestartAsync(m_keyRepeatInterval);
  // Simulate repeat: Key up and down
  XBMC_Event event = m_keyToRepeat;
  event.type = XBMC_KEYUP;
  m_handler.OnKeyboardEvent(event);
  event.type = XBMC_KEYDOWN;
  m_handler.OnKeyboardEvent(event);
}
