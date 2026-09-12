/*
 *  Copyright (C) 2026 Team Kodi
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "WasmKeyboard.h"

#include "utils/log.h"

#include <atomic>
#include <cstddef>

#include <emscripten.h>
#include <emscripten/em_asm.h>
#include <emscripten/threading.h>

namespace KODI::WINDOWING::WASM
{
namespace
{
std::atomic<int> g_activeKeyboards{0};
bool g_jsInstalled = false;
constexpr double WAIT_MS = 100.0;

int32_t Load(const int32_t& field)
{
  return __atomic_load_n(&field, __ATOMIC_ACQUIRE);
}
} // namespace

static_assert(offsetof(CWasmKeyboard::Shared, signal) == 0, "signal offset");
static_assert(offsetof(CWasmKeyboard::Shared, state) == 4, "state offset");
static_assert(offsetof(CWasmKeyboard::Shared, textLength) == 8, "textLength offset");
static_assert(offsetof(CWasmKeyboard::Shared, text) == 12, "text offset");

// Main-thread side. Field offsets and state values mirror struct Shared.
void CWasmKeyboard::InstallJs()
{
  if (g_jsInstalled)
    return;
  g_jsInstalled = true;

  MAIN_THREAD_EM_ASM(({
    const SIG = 0, STATE = 1, LEN = 2, TEXT = 12, TEXT_MAX = 4095;
    const SHOWN = 1, CONFIRMED = 2, CANCELLED = 3;
    // Samsung TV IME soft keys.
    const KEY_IME_DONE = 65376, KEY_IME_CANCEL = 65385;
    const kodi = (Module.kodi = Module.kodi || {});
    const kb = (kodi.keyboard = {});
    let root = null;
    let input = null;
    let ptr = 0;

    const publish = (state) => {
      if (!ptr) return;
      const base = ptr >> 2;
      if (state !== undefined) Atomics.store(HEAP32, base + STATE, state);
      Atomics.add(HEAP32, base + SIG, 1);
      Atomics.notify(HEAP32, base + SIG);
    };
    const publishText = () => {
      if (!ptr || !input) return;
      const bytes = new TextEncoder().encode(input.value);
      const n = Math.min(bytes.length, TEXT_MAX);
      HEAPU8.set(bytes.subarray(0, n), ptr + TEXT);
      HEAPU8[ptr + TEXT + n] = 0;
      Atomics.store(HEAP32, (ptr >> 2) + LEN, n);
      publish();
    };
    const finish = (state) => {
      if (!ptr) return;
      if (Atomics.load(HEAP32, (ptr >> 2) + STATE) !== SHOWN) return;
      publishText();
      publish(state);
    };

    kb.show = (sharedPtr, initialPtr, headingPtr, hidden) => {
      kb.hide();
      ptr = sharedPtr;
      root = document.createElement('div');
      root.style.cssText = 'position:fixed;left:50%;top:12%;transform:translateX(-50%);'
        + 'z-index:10;min-width:40vw;max-width:80vw;padding:1.2em 1.5em;border-radius:0.6em;'
        + 'background:rgba(20,20,20,0.92);color:#fff;font:1.6em sans-serif;box-shadow:0 0.5em 2em rgba(0,0,0,0.6)';
      const heading = UTF8ToString(headingPtr);
      if (heading) {
        const label = document.createElement('div');
        label.textContent = heading;
        label.style.cssText = 'margin-bottom:0.6em;opacity:0.85';
        root.appendChild(label);
      }
      input = document.createElement('input');
      input.type = hidden ? 'password' : 'text';
      input.value = UTF8ToString(initialPtr);
      input.autocomplete = 'off';
      input.autocapitalize = 'off';
      input.spellcheck = false;
      input.style.cssText = 'width:100%;box-sizing:border-box;font:inherit;padding:0.4em 0.6em;'
        + 'border:2px solid #17b2e7;border-radius:0.3em;background:#000;color:#fff;outline:none';
      input.addEventListener('input', publishText);
      input.addEventListener('keydown', (e) => {
        if (e.keyCode === 13 || e.keyCode === KEY_IME_DONE) { e.preventDefault(); finish(CONFIRMED); }
        else if (e.keyCode === 27 || e.keyCode === KEY_IME_CANCEL) { e.preventDefault(); finish(CANCELLED); }
        e.stopPropagation();
      });
      input.addEventListener('keyup', (e) => e.stopPropagation());
      input.addEventListener('blur', () => finish(CANCELLED));
      root.appendChild(input);
      document.body.appendChild(root);
      publish(SHOWN);
      input.focus();
      input.setSelectionRange(input.value.length, input.value.length);
    };
    kb.setText = (textPtr, close) => {
      if (!input) return;
      input.value = UTF8ToString(textPtr);
      publishText();
      if (close) finish(CONFIRMED);
    };
    kb.hide = () => {
      if (root) { root.remove(); root = null; input = null; }
      ptr = 0;
      const canvas = document.getElementById('canvas');
      if (canvas) canvas.focus();
    };
  }));
}

CWasmKeyboard::~CWasmKeyboard()
{
  Hide();
}

bool CWasmKeyboard::IsActive()
{
  return g_activeKeyboards.load(std::memory_order_acquire) > 0;
}

bool CWasmKeyboard::ShowAndGetInput(char_callback_t pCallback,
                                    const std::string& initialString,
                                    std::string& typedString,
                                    const std::string& heading,
                                    bool bHiddenInput)
{
  InstallJs();

  m_shared = {};
  __atomic_store_n(&m_shared.state, SHOWN, __ATOMIC_RELEASE);
  m_shown = true;
  g_activeKeyboards.fetch_add(1, std::memory_order_acq_rel);

  MAIN_THREAD_EM_ASM(({ Module.kodi.keyboard.show($0, $1, $2, $3); }), &m_shared,
                     initialString.c_str(), heading.c_str(), bHiddenInput ? 1 : 0);

  std::string lastReported = initialString;
  int32_t state = SHOWN;
  while (true)
  {
    const auto seen = static_cast<uint32_t>(Load(m_shared.signal));
    state = Load(m_shared.state);
    const int32_t length = Load(m_shared.textLength);
    const std::string text(m_shared.text, static_cast<size_t>(length));
    if (text != lastReported)
    {
      lastReported = text;
      if (pCallback)
        pCallback(this, text);
    }
    if (state != SHOWN)
      break;
    emscripten_futex_wait(&m_shared.signal, seen, WAIT_MS);
  }

  Hide();
  if (state != CONFIRMED)
    return false;

  typedString = lastReported;
  return true;
}

void CWasmKeyboard::Cancel()
{
  if (Load(m_shared.state) != SHOWN)
    return;
  __atomic_store_n(&m_shared.state, static_cast<int32_t>(CANCELLED), __ATOMIC_RELEASE);
  __atomic_add_fetch(&m_shared.signal, 1, __ATOMIC_ACQ_REL);
  emscripten_futex_wake(&m_shared.signal, 1);
}

bool CWasmKeyboard::SetTextToKeyboard(const std::string& text, bool closeKeyboard)
{
  if (!m_shown)
    return false;
  MAIN_THREAD_EM_ASM(({ Module.kodi.keyboard.setText($0, $1); }), text.c_str(),
                     closeKeyboard ? 1 : 0);
  return true;
}

void CWasmKeyboard::Hide()
{
  if (!m_shown)
    return;
  m_shown = false;
  MAIN_THREAD_EM_ASM(({ Module.kodi.keyboard.hide(); }));
  g_activeKeyboards.fetch_sub(1, std::memory_order_acq_rel);
}
} // namespace KODI::WINDOWING::WASM
