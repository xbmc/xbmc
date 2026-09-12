/*
 *  Copyright (C) 2026 Team Kodi
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "WinEventsWasm.h"

#include "ServiceBroker.h"
#include "WasmKeyboard.h"
#include "application/AppInboundProtocol.h"
#include "guilib/GUIWindowManager.h"
#include "input/keyboard/XBMC_keyboard.h"
#include "utils/CharsetConverter.h"

#include <algorithm>
#include <climits>
#include <cstdint>
#include <cstring>
#include <string>
#include <unordered_set>

#include <emscripten.h>
#include <emscripten/html5.h>
#include <emscripten/key_codes.h>
#include <emscripten/threading.h>

namespace
{
CWinEventsWasm* g_events = nullptr;

constexpr const char* CANVAS_TARGET = "#canvas";

XBMCMod TranslateModifiers(const EmscriptenKeyboardEvent* e)
{
  XBMCMod mod = XBMCKMOD_NONE;

  if (e->shiftKey)
    mod = static_cast<XBMCMod>(mod | XBMCKMOD_LSHIFT);
  if (e->ctrlKey)
    mod = static_cast<XBMCMod>(mod | XBMCKMOD_LCTRL);
  if (e->altKey)
    mod = static_cast<XBMCMod>(mod | XBMCKMOD_LALT);
  if (e->metaKey)
    mod = static_cast<XBMCMod>(mod | XBMCKMOD_LMETA);

  return mod;
}

XBMCKey TranslatePrintableKey(const char c)
{
  if (c >= 'a' && c <= 'z')
    return static_cast<XBMCKey>(XBMCK_a + (c - 'a'));
  if (c >= 'A' && c <= 'Z')
    return static_cast<XBMCKey>(XBMCK_a + (c - 'A'));
  if (c >= '0' && c <= '9')
    return static_cast<XBMCKey>(XBMCK_0 + (c - '0'));

  switch (c)
  {
    case ' ':
      return XBMCK_SPACE;
    case '!':
      return XBMCK_EXCLAIM;
    case '"':
      return XBMCK_QUOTEDBL;
    case '#':
      return XBMCK_HASH;
    case '$':
      return XBMCK_DOLLAR;
    case '%':
      return XBMCK_PERCENT;
    case '&':
      return XBMCK_AMPERSAND;
    case '\'':
      return XBMCK_QUOTE;
    case '(':
      return XBMCK_LEFTPAREN;
    case ')':
      return XBMCK_RIGHTPAREN;
    case '*':
      return XBMCK_ASTERISK;
    case '+':
      return XBMCK_PLUS;
    case ',':
      return XBMCK_COMMA;
    case '-':
      return XBMCK_MINUS;
    case '.':
      return XBMCK_PERIOD;
    case '/':
      return XBMCK_SLASH;
    case ':':
      return XBMCK_COLON;
    case ';':
      return XBMCK_SEMICOLON;
    case '<':
      return XBMCK_LESS;
    case '=':
      return XBMCK_EQUALS;
    case '>':
      return XBMCK_GREATER;
    case '?':
      return XBMCK_QUESTION;
    case '@':
      return XBMCK_AT;
    case '[':
      return XBMCK_LEFTBRACKET;
    case '\\':
      return XBMCK_BACKSLASH;
    case ']':
      return XBMCK_RIGHTBRACKET;
    case '^':
      return XBMCK_CARET;
    case '_':
      return XBMCK_UNDERSCORE;
    case '`':
      return XBMCK_BACKQUOTE;
    case '{':
      return XBMCK_LEFTBRACE;
    case '|':
      return XBMCK_PIPE;
    case '}':
      return XBMCK_RIGHTBRACE;
    case '~':
      return XBMCK_TILDE;
    default:
      return XBMCK_UNKNOWN;
  }
}

uint16_t TranslateUnicode(const EmscriptenKeyboardEvent* e)
{
  // KeyboardEvent.key holds either one printable character or a named key such as
  // "Enter", so a single code point is what distinguishes text from the rest.
  // Anything above the BMP has no room in the uint16_t keysym.
  const std::u32string codePoints = CCharsetConverter::utf8ToUtf32(e->key);
  if (codePoints.size() != 1 || codePoints[0] > UINT16_MAX)
    return 0;

  return static_cast<uint16_t>(codePoints[0]);
}

XBMCKey TranslateDomKey(const EmscriptenKeyboardEvent* e)
{
  if (std::strcmp(e->key, "Back") == 0)
    return XBMCK_ESCAPE;

  if (e->location == DOM_KEY_LOCATION_NUMPAD)
  {
    if (e->keyCode >= DOM_VK_NUMPAD0 && e->keyCode <= DOM_VK_NUMPAD9)
      return static_cast<XBMCKey>(XBMCK_KP0 + (e->keyCode - DOM_VK_NUMPAD0));

    switch (e->keyCode)
    {
      case DOM_VK_ADD:
        return XBMCK_KP_PLUS;
      case DOM_VK_SUBTRACT:
        return XBMCK_KP_MINUS;
      case DOM_VK_MULTIPLY:
        return XBMCK_KP_MULTIPLY;
      case DOM_VK_DIVIDE:
        return XBMCK_KP_DIVIDE;
      case DOM_VK_DECIMAL:
      case DOM_VK_DELETE:
        return XBMCK_KP_PERIOD;
      case DOM_VK_RETURN:
      case DOM_VK_ENTER:
        return XBMCK_KP_ENTER;
      default:
        break;
    }
  }

  if (const XBMCKey printable = TranslatePrintableKey(e->key[0]);
      e->key[0] != '\0' && e->key[1] == '\0' && printable != XBMCK_UNKNOWN)
  {
    return printable;
  }

  if (e->keyCode >= DOM_VK_F1 && e->keyCode <= DOM_VK_F15)
    return static_cast<XBMCKey>(XBMCK_F1 + (e->keyCode - DOM_VK_F1));

  switch (e->keyCode)
  {
    case 10009: // Tizen TV hardware Back key
      return XBMCK_ESCAPE;
    case DOM_VK_BACK_SPACE:
      return XBMCK_BACKSPACE;
    case DOM_VK_TAB:
      return XBMCK_TAB;
    case DOM_VK_RETURN:
    case DOM_VK_ENTER:
      return XBMCK_RETURN;
    case DOM_VK_ESCAPE:
      return XBMCK_ESCAPE;
    case DOM_VK_SPACE:
      return XBMCK_SPACE;
    case DOM_VK_PAGE_UP:
      return XBMCK_PAGEUP;
    case DOM_VK_PAGE_DOWN:
      return XBMCK_PAGEDOWN;
    case DOM_VK_END:
      return XBMCK_END;
    case DOM_VK_HOME:
      return XBMCK_HOME;
    case DOM_VK_LEFT:
      return XBMCK_LEFT;
    case DOM_VK_UP:
      return XBMCK_UP;
    case DOM_VK_RIGHT:
      return XBMCK_RIGHT;
    case DOM_VK_DOWN:
      return XBMCK_DOWN;
    case DOM_VK_INSERT:
      return XBMCK_INSERT;
    case DOM_VK_DELETE:
      return XBMCK_DELETE;
    case DOM_VK_SHIFT:
      return e->location == DOM_KEY_LOCATION_RIGHT ? XBMCK_RSHIFT : XBMCK_LSHIFT;
    case DOM_VK_CONTROL:
      return e->location == DOM_KEY_LOCATION_RIGHT ? XBMCK_RCTRL : XBMCK_LCTRL;
    case DOM_VK_ALT:
      return e->location == DOM_KEY_LOCATION_RIGHT ? XBMCK_RALT : XBMCK_LALT;
    case DOM_VK_WIN:
      return e->location == DOM_KEY_LOCATION_RIGHT ? XBMCK_RSUPER : XBMCK_LSUPER;
    case DOM_VK_CONTEXT_MENU:
      return XBMCK_MENU;
    case DOM_VK_CAPS_LOCK:
      return XBMCK_CAPSLOCK;
    case DOM_VK_NUM_LOCK:
      return XBMCK_NUMLOCK;
    case DOM_VK_SCROLL_LOCK:
      return XBMCK_SCROLLOCK;
    default:
      return XBMCK_UNKNOWN;
  }
}

XBMC_Event TranslateKeyEvent(const EmscriptenKeyboardEvent* e, uint8_t type)
{
  XBMC_Event ev{};
  ev.type = type;
  ev.key.keysym.scancode = e->keyCode;
  ev.key.keysym.sym = TranslateDomKey(e);
  ev.key.keysym.mod = TranslateModifiers(e);
  ev.key.keysym.unicode = TranslateUnicode(e);
  return ev;
}

// Identity mapping: render resolution == canvas CSS size.
void TranslateMousePosition(const EmscriptenMouseEvent* e, uint16_t& x, uint16_t& y)
{
  const int rawX = std::max(0, e->targetX);
  const int rawY = std::max(0, e->targetY);
  x = static_cast<uint16_t>(std::min(rawX, static_cast<int>(UINT16_MAX)));
  y = static_cast<uint16_t>(std::min(rawY, static_cast<int>(UINT16_MAX)));
}

// Keys pressed while the native keyboard owned input. Their release arrives
// after the keyboard has closed and must be dropped too, or Kodi's long-press
// handling acts on a release it never saw the press for.
std::unordered_set<unsigned long> g_swallowedKeys;

EM_BOOL OnKeyDown(int /*eventType*/, const EmscriptenKeyboardEvent* e, void* /*userData*/)
{
  if (!g_events)
    return EM_FALSE;
  if (KODI::WINDOWING::WASM::CWasmKeyboard::IsActive())
  {
    g_swallowedKeys.insert(e->keyCode);
    return EM_FALSE;
  }
  g_events->MessagePush(TranslateKeyEvent(e, XBMC_KEYDOWN));
  // Cancelling the paste shortcut would suppress the DOM "paste" event that
  // supplies the clipboard text (see WasmClipboard.cpp).
  const bool isPaste = (e->ctrlKey || e->metaKey) && e->keyCode == DOM_VK_V;
  return isPaste ? EM_FALSE : EM_TRUE;
}

EM_BOOL OnKeyUp(int /*eventType*/, const EmscriptenKeyboardEvent* e, void* /*userData*/)
{
  if (!g_events)
    return EM_FALSE;
  if (KODI::WINDOWING::WASM::CWasmKeyboard::IsActive() || g_swallowedKeys.erase(e->keyCode) > 0)
    return EM_FALSE;
  g_events->MessagePush(TranslateKeyEvent(e, XBMC_KEYUP));
  return EM_TRUE;
}

EM_BOOL OnMouseMove(int /*eventType*/, const EmscriptenMouseEvent* e, void* /*userData*/)
{
  if (!g_events)
    return EM_FALSE;
  XBMC_Event ev{};
  ev.type = XBMC_MOUSEMOTION;
  TranslateMousePosition(e, ev.motion.x, ev.motion.y);
  g_events->MessagePush(ev);
  return EM_FALSE;
}

EM_BOOL OnMouseButtonDown(int /*eventType*/, const EmscriptenMouseEvent* e, void* /*userData*/)
{
  if (!g_events)
    return EM_FALSE;
  XBMC_Event ev{};
  ev.type = XBMC_MOUSEBUTTONDOWN;
  TranslateMousePosition(e, ev.button.x, ev.button.y);
  ev.button.button = static_cast<unsigned char>(e->button + 1);
  g_events->MessagePush(ev);
  return EM_TRUE;
}

EM_BOOL OnMouseButtonUp(int /*eventType*/, const EmscriptenMouseEvent* e, void* /*userData*/)
{
  if (!g_events)
    return EM_FALSE;
  XBMC_Event ev{};
  ev.type = XBMC_MOUSEBUTTONUP;
  TranslateMousePosition(e, ev.button.x, ev.button.y);
  ev.button.button = static_cast<unsigned char>(e->button + 1);
  g_events->MessagePush(ev);
  return EM_TRUE;
}

EM_BOOL OnResize(int /*eventType*/, const EmscriptenUiEvent* e, void* /*userData*/)
{
  if (!g_events)
    return EM_FALSE;
  XBMC_Event ev{};
  ev.type = XBMC_VIDEORESIZE;
  ev.resize.width = e->windowInnerWidth;
  ev.resize.height = e->windowInnerHeight;
  ev.resize.scale = 1.0;
  g_events->MessagePush(ev);
  return EM_FALSE;
}
} // namespace

CWinEventsWasm::CWinEventsWasm()
{
  g_events = this;
  emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, EM_TRUE, OnKeyDown);
  emscripten_set_keyup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, EM_TRUE, OnKeyUp);
  emscripten_set_mousemove_callback(CANVAS_TARGET, nullptr, EM_TRUE, OnMouseMove);
  emscripten_set_mousedown_callback(CANVAS_TARGET, nullptr, EM_TRUE, OnMouseButtonDown);
  emscripten_set_mouseup_callback(CANVAS_TARGET, nullptr, EM_TRUE, OnMouseButtonUp);
  emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, EM_TRUE, OnResize);
}

CWinEventsWasm::~CWinEventsWasm()
{
  g_events = nullptr;
}

void CWinEventsWasm::MessagePush(const XBMC_Event& newEvent)
{
  std::unique_lock lock(m_mutex);
  m_events.push_back(newEvent);
}

void CWinEventsWasm::ProcessProxyCallbacks()
{
  // Modal dialogs spin a nested Kodi render loop on the worker thread.
  // Process proxied browser callbacks here so keyboard/mouse events can still
  // reach this thread while that nested loop is active.
  emscripten_current_thread_process_queued_calls();
}

bool CWinEventsWasm::DispatchQueuedEvents()
{
  bool handledEvent = false;
  for (;;)
  {
    XBMC_Event pumpEvent{};
    {
      std::unique_lock lock(m_mutex);
      if (m_events.empty())
        break;
      pumpEvent = m_events.front();
      m_events.pop_front();
    }
    std::shared_ptr<CAppInboundProtocol> appPort = CServiceBroker::GetAppPort();
    if (appPort)
      handledEvent |= appPort->OnEvent(pumpEvent);
  }
  return handledEvent;
}

bool CWinEventsWasm::MessagePump()
{
  ProcessProxyCallbacks();
  return DispatchQueuedEvents();
}
