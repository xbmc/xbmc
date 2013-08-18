/*
*      Copyright (C) 2005-2013 Team XBMC
*      http://www.xbmc.org
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
#include <algorithm>
#include <sstream>
#include <vector>

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/scope_exit.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

#include <wayland-client.h>
#include <xkbcommon/xkbcommon.h>

#include "utils/log.h"

#include "windowing/DllXKBCommon.h"

#include "EventListener.h"
#include "Keyboard.h"
#include "KeyboardProcessor.h"
#include "TimeoutManager.h"

#include "input/linux/XKBCommonKeymap.h"
#include "input/linux/Keymap.h"

xbmc::KeyboardProcessor::KeyboardProcessor(IDllXKBCommon &xkbCommonLibrary,
                                           IEventListener &listener,
                                           ITimeoutManager &timeouts) :
  m_xkbCommonLibrary(xkbCommonLibrary),
  m_listener(listener),
  m_timeouts(timeouts),
  m_xbmcWindow(NULL),
  m_repeatSym(0),
  m_context(NULL)
{
  enum xkb_context_flags flags =
    static_cast<enum xkb_context_flags>(0);

  m_context = m_xkbCommonLibrary.xkb_context_new(flags);
  
/* KeyboardProcessor and not XKBKeymap owns the xkb_context. The
   * xkb_context is merely just a detail for construction of the
   * more interesting xkb_state and xkb_keymap objects.
   * 
   * Failure to create the context effectively means that we will
   * be unable to create a keymap or serve any useful purpose in
   * processing key events. As such, it makes this an incomplete
   * object and a runtime_error will be thrown */
  if (!m_context)
    throw std::runtime_error("Failed to create xkb context");
}

xbmc::KeyboardProcessor::~KeyboardProcessor()
{
  m_xkbCommonLibrary.xkb_context_unref(m_context);
}

void
xbmc::KeyboardProcessor::SetXBMCSurface(struct wl_surface *s)
{
  m_xbmcWindow = s;
}

/* Creates a new internal keymap representation for a serialized
 * keymap as represented in shared memory as referred to by fd.
 * 
 * Since the fd is sent to us via sendmsg(), the currently running
 * process has ownership over it. As such, it MUST close the file
 * descriptor after it has decided what to do with it in order to
 * avoid a leak.
 */
void
xbmc::KeyboardProcessor::UpdateKeymap(uint32_t format,
                                      int fd,
                                      uint32_t size)
{
  /* The file descriptor must always be closed */
  BOOST_SCOPE_EXIT((fd))
  {
    close(fd);
  } BOOST_SCOPE_EXIT_END

  /* We don't understand anything other than xkbv1. If we get some
   * other keyboard, then we can't process keyboard events reliably
   * and that's a runtime error. */
  if (format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1)
    throw std::runtime_error("Server gave us a keymap we don't understand");

  bool successfullyCreatedKeyboard = false;
  
  /* Either throws or returns a valid xkb_keymap * */
  struct xkb_keymap *keymap =
    CXKBKeymap::ReceiveXKBKeymapFromSharedMemory(m_xkbCommonLibrary, m_context, fd, size);

  BOOST_SCOPE_EXIT((&m_xkbCommonLibrary)(&successfullyCreatedKeyboard)(keymap))
  {
    if (!successfullyCreatedKeyboard)
      m_xkbCommonLibrary.xkb_keymap_unref(keymap);
  } BOOST_SCOPE_EXIT_END

  struct xkb_state *state =
    CXKBKeymap::CreateXKBStateFromKeymap(m_xkbCommonLibrary, keymap);

  m_keymap.reset(new CXKBKeymap(m_xkbCommonLibrary, keymap, state));
  
  successfullyCreatedKeyboard = true;
}

void
xbmc::KeyboardProcessor::Enter(uint32_t serial,
                               struct wl_surface *surface,
                               struct wl_array *keys)
{
  if (surface == m_xbmcWindow)
  {
    m_listener.OnFocused();
  }
}

void
xbmc::KeyboardProcessor::Leave(uint32_t serial,
                               struct wl_surface *surface)
{
  if (surface == m_xbmcWindow)
  {
    m_listener.OnUnfocused();
  }
}

void
xbmc::KeyboardProcessor::SendKeyToXBMC(uint32_t key,
                                       uint32_t sym,
                                       uint32_t eventType)
{
  XBMC_Event event;
  event.type = eventType;
  event.key.keysym.scancode = key;
  event.key.keysym.sym = static_cast<XBMCKey>(sym);
  event.key.keysym.unicode = static_cast<XBMCKey>(sym);
  event.key.keysym.mod =
    static_cast<XBMCMod>(m_keymap->ActiveXBMCModifiers());
  event.key.state = 0;
  event.key.type = event.type;
  event.key.which = '0';

  m_listener.OnEvent(event);
}

void
xbmc::KeyboardProcessor::RepeatCallback(uint32_t key,
                                        uint32_t sym)
{
  /* Release and press the key again */
  SendKeyToXBMC(key, sym, XBMC_KEYUP);
  SendKeyToXBMC(key, sym, XBMC_KEYDOWN);
}

/* If this function is called before a keymap is set, then that
 * is a precondition violation and a logic_error results */
void
xbmc::KeyboardProcessor::Key(uint32_t serial,
                             uint32_t time,
                             uint32_t key,
                             enum wl_keyboard_key_state state)
{
  if (!m_keymap.get())
    throw std::logic_error("a keymap must be set before processing key events");

  uint32_t sym = XKB_KEY_NoSymbol;

  /* If we're unable to process a single key, then catch the error
   * and report it, but don't allow it to be fatal */
  try
  {
    sym = m_keymap->XBMCKeysymForKeycode(key);
  }
  catch (const std::runtime_error &err)
  {
    CLog::Log(LOGERROR, "%s: Failed to process keycode %i: %s",
              __FUNCTION__, key, err.what());
    return;
  }

  uint32_t keyEventType = 0;

  switch (state)
  {
    case WL_KEYBOARD_KEY_STATE_PRESSED:
      keyEventType = XBMC_KEYDOWN;
      break;
    case WL_KEYBOARD_KEY_STATE_RELEASED:
      keyEventType = XBMC_KEYUP;
      break;
    default:
      CLog::Log(LOGERROR, "%s: Unrecognized key state", __FUNCTION__);
      return;
  }
  
  /* Key-repeat is handled on the client side so we need to add a new
   * timeout here to repeat this symbol if it is still being held down
   */
  if (keyEventType == XBMC_KEYDOWN)
  {
    m_repeatCallback =
      m_timeouts.RepeatAfterMs(boost::bind (
                                 &KeyboardProcessor::RepeatCallback,
                                 this,
                                 key,
                                 sym),
                               1000,
                               250);
    m_repeatSym = sym;
  }
  else if (keyEventType == XBMC_KEYUP &&
           sym == m_repeatSym)
    m_repeatCallback.reset();
  
  SendKeyToXBMC(key, sym, keyEventType);
}

/* We MUST update the keymap mask whenever we receive a new modifier
 * event */
void
xbmc::KeyboardProcessor::Modifier(uint32_t serial,
                                  uint32_t depressed,
                                  uint32_t latched,
                                  uint32_t locked,
                                  uint32_t group)
{
  m_keymap->UpdateMask(depressed, latched, locked, group);
}
