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
#include "input/mouse/MouseStat.h"
#include "input/touch/generic/GenericTouchActionHandler.h"
#include "input/touch/generic/GenericTouchInputHandler.h"
#include "utils/CharsetConverter.h"

#include <algorithm>
#include <array>
#include <climits>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>
#include <unordered_set>

#include <emscripten.h>
#include <emscripten/html5.h>
#include <emscripten/key_codes.h>
#include <emscripten/threading.h>

namespace
{
constexpr const char* CANVAS_TARGET = "#canvas";

// CSS pixels are defined at 96 per inch.
constexpr double CSS_DPI = 96.0;

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
  // A TV remote's Back gets the remote rules keymaps give browser_back, not Escape's.
  if (std::strcmp(e->key, "Back") == 0)
    return XBMCK_BROWSER_BACK;

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
      return XBMCK_BROWSER_BACK;
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

// Positions arrive in CSS pixels, the GUI renders in device pixels.
void TranslateMousePosition(const EmscriptenMouseEvent* e, double ratio, uint16_t& x, uint16_t& y)
{
  const int rawX = std::max(0, static_cast<int>(std::lround(e->targetX * ratio)));
  const int rawY = std::max(0, static_cast<int>(std::lround(e->targetY * ratio)));
  x = static_cast<uint16_t>(std::min(rawX, static_cast<int>(UINT16_MAX)));
  y = static_cast<uint16_t>(std::min(rawY, static_cast<int>(UINT16_MAX)));
}

// The DOM numbers the back and forward buttons 3 and 4, Kodi's wheel IDs.
unsigned char TranslateMouseButton(unsigned short button)
{
  if (button < 3)
    return static_cast<unsigned char>(XBMC_BUTTON_LEFT + button);
  return static_cast<unsigned char>(XBMC_BUTTON_X1 + button - 3);
}

// Keys typed into another field of the hosting page belong to that field.
bool IsTextFieldFocused()
{
  // clang-format off
  return EM_ASM_INT({
    const t = document.activeElement;
    return t && (t.tagName === 'INPUT' || t.tagName === 'TEXTAREA' || t.isContentEditable) ? 1 : 0;
  }) != 0;
  // clang-format on
}

// Reload, fullscreen, developer tools and zoom stay with the browser.
bool IsBrowserShortcut(const EmscriptenKeyboardEvent* e)
{
  if (e->keyCode == DOM_VK_F5 || e->keyCode == DOM_VK_F11 || e->keyCode == DOM_VK_F12)
    return true;
  if (!e->ctrlKey && !e->metaKey)
    return false;
  constexpr std::array<std::string_view, 6> keys{"r", "R", "+", "=", "-", "0"};
  return std::ranges::find(keys, std::string_view(e->key)) != keys.end();
}

EM_BOOL OnKeyDown(int /*eventType*/, const EmscriptenKeyboardEvent* e, void* userData)
{
  auto& events = *static_cast<CWinEventsWasm*>(userData);
  if (KODI::WINDOWING::WASM::CWasmKeyboard::IsActive())
  {
    events.AddSwallowedKey(e->keyCode);
    return EM_FALSE;
  }
  // A new press means the release of a swallowed one never reached the page.
  events.RemoveSwallowedKey(e->keyCode);
  if (IsTextFieldFocused())
    return EM_FALSE;
  // The browser's "paste" event supplies the clipboard text (see
  // WasmClipboard.cpp); passed on, the shortcut would also type a "v".
  const bool isPaste = (e->ctrlKey || e->metaKey) && e->keyCode == DOM_VK_V;
  if (isPaste || IsBrowserShortcut(e))
  {
    events.AddSwallowedKey(e->keyCode);
    return EM_FALSE;
  }
  events.MessagePush(TranslateKeyEvent(e, XBMC_KEYDOWN));
  return EM_TRUE;
}

EM_BOOL OnKeyUp(int /*eventType*/, const EmscriptenKeyboardEvent* e, void* userData)
{
  auto& events = *static_cast<CWinEventsWasm*>(userData);
  if (KODI::WINDOWING::WASM::CWasmKeyboard::IsActive() || events.RemoveSwallowedKey(e->keyCode))
    return EM_FALSE;
  if (IsTextFieldFocused())
    return EM_FALSE;
  events.MessagePush(TranslateKeyEvent(e, XBMC_KEYUP));
  return EM_TRUE;
}

EM_BOOL OnMouseMove(int /*eventType*/, const EmscriptenMouseEvent* e, void* userData)
{
  auto& events = *static_cast<CWinEventsWasm*>(userData);
  XBMC_Event ev{};
  ev.type = XBMC_MOUSEMOTION;
  TranslateMousePosition(e, events.GetPointerScale(), ev.motion.x, ev.motion.y);
  events.MessagePush(ev);
  return EM_FALSE;
}

EM_BOOL OnMouseButtonDown(int /*eventType*/, const EmscriptenMouseEvent* e, void* userData)
{
  auto& events = *static_cast<CWinEventsWasm*>(userData);
  XBMC_Event ev{};
  ev.type = XBMC_MOUSEBUTTONDOWN;
  TranslateMousePosition(e, events.GetPointerScale(), ev.button.x, ev.button.y);
  ev.button.button = TranslateMouseButton(e->button);
  events.MessagePush(ev);
  return EM_TRUE;
}

EM_BOOL OnMouseButtonUp(int /*eventType*/, const EmscriptenMouseEvent* e, void* userData)
{
  auto& events = *static_cast<CWinEventsWasm*>(userData);
  XBMC_Event ev{};
  ev.type = XBMC_MOUSEBUTTONUP;
  TranslateMousePosition(e, events.GetPointerScale(), ev.button.x, ev.button.y);
  ev.button.button = TranslateMouseButton(e->button);
  events.MessagePush(ev);
  return EM_TRUE;
}

// Like the other backends, each wheel event is one step: DOM deltas vary too
// much across browsers and devices to scale them.
EM_BOOL OnWheel(int /*eventType*/, const EmscriptenWheelEvent* e, void* userData)
{
  // Ctrl+wheel zooms the page.
  if (e->deltaY == 0.0 || e->mouse.ctrlKey)
    return EM_FALSE;
  auto& events = *static_cast<CWinEventsWasm*>(userData);
  XBMC_Event ev{};
  ev.type = XBMC_MOUSEBUTTONDOWN;
  TranslateMousePosition(&e->mouse, events.GetPointerScale(), ev.button.x, ev.button.y);
  ev.button.button = e->deltaY < 0.0 ? XBMC_BUTTON_WHEELUP : XBMC_BUTTON_WHEELDOWN;
  events.MessagePush(ev);
  ev.type = XBMC_MOUSEBUTTONUP;
  events.MessagePush(ev);
  return EM_TRUE;
}

void FocusCanvas()
{
  // clang-format off
  EM_ASM({
    const canvas = document.querySelector(UTF8ToString($0));
    if (canvas && document.activeElement !== canvas)
      canvas.focus();
  }, CANVAS_TARGET);
  // clang-format on
}

EM_BOOL OnTouch(int eventType, const EmscriptenTouchEvent* e, void* userData)
{
  auto& events = *static_cast<CWinEventsWasm*>(userData);
  TouchInput event;
  switch (eventType)
  {
    case EMSCRIPTEN_EVENT_TOUCHSTART:
      event = TouchInputDown;
      break;
    case EMSCRIPTEN_EVENT_TOUCHMOVE:
      event = TouchInputMove;
      break;
    case EMSCRIPTEN_EVENT_TOUCHEND:
      event = TouchInputUp;
      break;
    case EMSCRIPTEN_EVENT_TOUCHCANCEL:
      event = TouchInputAbort;
      break;
    default:
      return EM_FALSE;
  }

  // Returning true cancels the browser's own handling of the tap, which would
  // have moved the focus to the canvas and so closed a native keyboard.
  if (eventType == EMSCRIPTEN_EVENT_TOUCHSTART)
    FocusCanvas();

  const double scale = events.GetPointerScale();
  const auto time = static_cast<int64_t>(e->timestamp * 1e6);
  for (int i = 0; i < e->numTouches; ++i)
  {
    const EmscriptenTouchPoint& point = e->touches[i];
    if (!point.isChanged)
      continue;
    // A touch dragged off the canvas keeps reporting, past its edges too.
    const auto x = static_cast<float>(std::max(0.0, point.targetX * scale));
    const auto y = static_cast<float>(std::max(0.0, point.targetY * scale));
    events.TouchPush({event, point.identifier, x, y, time});
  }
  return EM_TRUE;
}

EM_BOOL OnResize(int /*eventType*/, const EmscriptenUiEvent* e, void* userData)
{
  auto& events = *static_cast<CWinEventsWasm*>(userData);
  XBMC_Event ev{};
  ev.type = XBMC_VIDEORESIZE;
  ev.resize.width = e->windowInnerWidth;
  ev.resize.height = e->windowInnerHeight;
  // zooming the page changes the device pixel ratio along with the size
  ev.resize.scale = CWinEventsWasm::DevicePixelRatio();
  events.MessagePush(ev);
  return EM_FALSE;
}

// Sets the callbacks with events as their userData, or removes them for nullptr.
void SetCallbacks(CWinEventsWasm* events)
{
  // Key callbacks run on the browser thread: only there does their return value
  // cancel the browser's default action, and the native keyboard's state must be
  // read when the key is pressed, not when the Kodi thread gets to the event.
  // Setting and removing them is synchronous with the browser thread, so none is
  // still running once they are removed.
  emscripten_set_keydown_callback_on_thread(EMSCRIPTEN_EVENT_TARGET_WINDOW, events, EM_TRUE,
                                            events ? OnKeyDown : nullptr,
                                            EM_CALLBACK_THREAD_CONTEXT_MAIN_RUNTIME_THREAD);
  emscripten_set_keyup_callback_on_thread(EMSCRIPTEN_EVENT_TARGET_WINDOW, events, EM_TRUE,
                                          events ? OnKeyUp : nullptr,
                                          EM_CALLBACK_THREAD_CONTEXT_MAIN_RUNTIME_THREAD);
  // Touch callbacks run on the browser thread too: returning true there is what
  // stops the browser from also turning the touch into mouse clicks.
  emscripten_set_touchstart_callback_on_thread(CANVAS_TARGET, events, EM_TRUE,
                                               events ? OnTouch : nullptr,
                                               EM_CALLBACK_THREAD_CONTEXT_MAIN_RUNTIME_THREAD);
  emscripten_set_touchmove_callback_on_thread(CANVAS_TARGET, events, EM_TRUE,
                                              events ? OnTouch : nullptr,
                                              EM_CALLBACK_THREAD_CONTEXT_MAIN_RUNTIME_THREAD);
  emscripten_set_touchend_callback_on_thread(CANVAS_TARGET, events, EM_TRUE,
                                             events ? OnTouch : nullptr,
                                             EM_CALLBACK_THREAD_CONTEXT_MAIN_RUNTIME_THREAD);
  emscripten_set_touchcancel_callback_on_thread(CANVAS_TARGET, events, EM_TRUE,
                                                events ? OnTouch : nullptr,
                                                EM_CALLBACK_THREAD_CONTEXT_MAIN_RUNTIME_THREAD);
  emscripten_set_mousemove_callback(CANVAS_TARGET, events, EM_TRUE, events ? OnMouseMove : nullptr);
  emscripten_set_mousedown_callback(CANVAS_TARGET, events, EM_TRUE,
                                    events ? OnMouseButtonDown : nullptr);
  emscripten_set_mouseup_callback(CANVAS_TARGET, events, EM_TRUE,
                                  events ? OnMouseButtonUp : nullptr);
  emscripten_set_wheel_callback(CANVAS_TARGET, events, EM_TRUE, events ? OnWheel : nullptr);
  emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, events, EM_TRUE,
                                 events ? OnResize : nullptr);
}
} // namespace

CWinEventsWasm::CWinEventsWasm()
{
  CGenericTouchInputHandler::GetInstance().RegisterHandler(
      &CGenericTouchActionHandler::GetInstance());
  SetCallbacks(this);
}

CWinEventsWasm::~CWinEventsWasm()
{
  SetCallbacks(nullptr);
  // Events the browser already queued for this thread still point at this
  // object, so run them before it goes away.
  ProcessProxyCallbacks();
  CGenericTouchInputHandler::GetInstance().UnregisterHandler();
}

void CWinEventsWasm::MessagePush(const XBMC_Event& newEvent)
{
  std::unique_lock lock(m_mutex);
  m_events.push_back(newEvent);
}

void CWinEventsWasm::TouchPush(const Touch& touch)
{
  std::unique_lock lock(m_mutex);
  m_touches.push_back(touch);
}

double CWinEventsWasm::DevicePixelRatio()
{
  const double ratio = emscripten_get_device_pixel_ratio();
  return ratio > 0 ? ratio : 1.0;
}

void CWinEventsWasm::SetPointerScale(double scale)
{
  m_pointerScale = scale;
  CGenericTouchInputHandler::GetInstance().SetScreenDPI(static_cast<float>(CSS_DPI * scale));
}

void CWinEventsWasm::AddSwallowedKey(unsigned long keyCode)
{
  m_swallowedKeys.insert(keyCode);
}

bool CWinEventsWasm::RemoveSwallowedKey(unsigned long keyCode)
{
  return m_swallowedKeys.erase(keyCode) > 0;
}

void CWinEventsWasm::ProcessProxyCallbacks()
{
  // Modal dialogs spin a nested Kodi render loop on the worker thread.
  // Process proxied browser callbacks here so mouse and resize events can still
  // reach this thread while that nested loop is active.
  emscripten_current_thread_process_queued_calls();
}

void CWinEventsWasm::DispatchQueuedTouches()
{
  std::deque<Touch> touches;
  {
    std::unique_lock lock(m_mutex);
    touches.swap(m_touches);
  }

  CGenericTouchInputHandler& handler = CGenericTouchInputHandler::GetInstance();
  for (const Touch& touch : touches)
  {
    // Abort ends every touch, whichever pointer it is sent for.
    if (touch.event == TouchInputAbort)
    {
      if (!m_touchPointers.empty())
        handler.HandleTouchInput(TouchInputAbort, touch.x, touch.y, touch.time);
      m_touchPointers.clear();
      continue;
    }

    int32_t pointer = -1;
    if (touch.event == TouchInputDown)
    {
      if (m_touchPointers.size() >= CGenericTouchInputHandler::MAX_POINTERS)
        continue;
      pointer = static_cast<int32_t>(m_touchPointers.size());
      m_touchPointers[touch.id] = pointer;
    }
    else
    {
      const auto it = m_touchPointers.find(touch.id);
      if (it == m_touchPointers.end())
        continue;
      pointer = it->second;
      if (touch.event == TouchInputUp)
      {
        m_touchPointers.erase(it);
        // Keep the numbers packed, see m_touchPointers.
        for (auto& entry : m_touchPointers)
        {
          if (entry.second > pointer)
            --entry.second;
        }
      }
    }

    // The handler only takes a move's position from here.
    if (touch.event == TouchInputMove)
      handler.UpdateTouchPointer(pointer, touch.x, touch.y, touch.time);
    handler.HandleTouchInput(touch.event, touch.x, touch.y, touch.time, pointer);
  }
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
  DispatchQueuedTouches();
  return DispatchQueuedEvents();
}
