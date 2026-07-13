/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WebOSKeyboard.h"

#include "ServiceBroker.h"
#include "WinSystemWaylandWebOS.h"
#include "guilib/GUIFont.h"
#include "guilib/GUIFontManager.h"
#include "guilib/GUITextLayout.h"
#include "guilib/GUITexture.h"
#include "utils/CharsetConverter.h"
#include "utils/log.h"
#include "windowing/GraphicContext.h"
#include "windowing/WinSystem.h"

#include <algorithm>
#include <cstdint>

#include <xkbcommon/xkbcommon-keysyms.h>

namespace KODI::WINDOWING::WAYLAND
{

CWebOSKeyboard::CWebOSKeyboard() = default;
CWebOSKeyboard::~CWebOSKeyboard() = default;

bool CWebOSKeyboard::ShowAndGetInput(const char_callback_t callback,
                                     const std::string& initialString,
                                     std::string& typedString,
                                     const std::string& heading,
                                     const bool hiddenInput)
{
  m_winSystem = dynamic_cast<CWinSystemWaylandWebOS*>(CServiceBroker::GetWinSystem());
  if (!m_winSystem || !m_winSystem->HasTextInput())
  {
    CLog::LogF(LOGWARNING, "webOS text input protocol is unavailable");
    return false;
  }

  m_textModel = m_winSystem->CreateTextModel();
  if (!m_textModel)
    return false;

  {
    std::unique_lock lock(m_mutex);
    m_text = initialString;
    m_preedit.clear();
    m_preeditCommit.clear();
    m_cursor = m_text.size();
    m_callback = callback;
    m_confirmed = false;
    m_active = true;
    m_panelShown = false;
    m_hiddenInput = hiddenInput;
    m_inputPanelRect = {};
    m_renderDirty = true;
    m_finished.Reset();
  }

  m_textModel.on_commit_string() = [this](const std::uint32_t, const std::string& text)
  {
    {
      std::unique_lock lock(m_mutex);
      m_preedit.clear();
      m_preeditCommit.clear();
      m_text.insert(m_cursor, text);
      m_cursor += text.size();
      UpdateSurroundingText();
    }
    TextChanged();
  };
  m_textModel.on_preedit_string() =
      [this](const std::uint32_t, const std::string& text, const std::string& commit)
  {
    {
      std::unique_lock lock(m_mutex);
      m_preedit = text;
      m_preeditCommit = commit;
    }
    TextChanged();
  };
  m_textModel.on_delete_surrounding_text() =
      [this](const std::uint32_t, const std::int32_t index, const std::uint32_t length)
  {
    {
      std::unique_lock lock(m_mutex);
      const auto cursor = static_cast<std::int64_t>(m_cursor);
      const auto start = std::clamp<std::int64_t>(cursor + index, 0, m_text.size());
      const auto byteStart = static_cast<std::size_t>(start);
      const auto count = std::min<std::size_t>(length, m_text.size() - byteStart);
      m_text.erase(byteStart, count);
      m_cursor = byteStart;
      UpdateSurroundingText();
    }
    TextChanged();
  };
  m_textModel.on_keysym() = [this](const std::uint32_t, const std::uint32_t,
                                   const std::uint32_t sym, const std::uint32_t state,
                                   const std::uint32_t)
  {
    if (state == 0)
      return;
    if (sym == XKB_KEY_BackSpace)
      Backspace();
    else if (sym == XKB_KEY_Return || sym == XKB_KEY_KP_Enter)
      Finish(true);
    else if (sym == XKB_KEY_Escape)
      Finish(false);
  };
  m_textModel.on_input_panel_state() = [this](const std::uint32_t state)
  {
    std::unique_lock lock(m_mutex);
    if (state != 0)
      m_panelShown = true;
    else if (m_panelShown)
    {
      lock.unlock();
      Finish(false);
    }
  };
  m_textModel.on_input_panel_rect() = [this](const std::int32_t x, const std::int32_t y,
                                             const std::uint32_t width, const std::uint32_t height)
  {
    const CRectInt panelRect{x, y, x + static_cast<int>(width), y + static_cast<int>(height)};
    {
      std::unique_lock lock(m_mutex);
      m_inputPanelRect = panelRect;
      m_renderDirty = true;
    }
  };
  m_textModel.on_leave() = [this] { Finish(false); };

  const auto hint = hiddenInput ? wayland::text_model_content_hint::password
                                : wayland::text_model_content_hint::_default;
  const auto purpose = hiddenInput ? wayland::text_model_content_purpose::password
                                   : wayland::text_model_content_purpose::normal;
  m_textModel.set_content_type(static_cast<std::uint32_t>(hint),
                               static_cast<std::uint32_t>(purpose));
  m_textModel.set_enter_key_type(
      static_cast<std::uint32_t>(wayland::text_model_enter_key_type::done));
  UpdateSurroundingText();

  if (!m_winSystem->ActivateTextModel(m_textModel))
  {
    Finish(false);
    {
      std::unique_lock lock(m_mutex);
      m_active = false;
      m_callback = nullptr;
    }
    m_textModel = {};
    return false;
  }
  m_textModel.show_input_panel();
  m_finished.Wait();

  m_textModel.hide_input_panel();
  m_winSystem->DeactivateTextModel(m_textModel);
  {
    std::unique_lock lock(m_mutex);
    if (m_confirmed)
      typedString = m_text;
    m_active = false;
    m_callback = nullptr;
  }
  m_textModel = {};
  return m_confirmed;
}

bool CWebOSKeyboard::SetTextToKeyboard(const std::string& text, const bool closeKeyboard)
{
  {
    std::unique_lock lock(m_mutex);
    if (!m_active)
      return false;
    m_text = text;
    m_preedit.clear();
    m_preeditCommit.clear();
    m_cursor = text.size();
    m_textModel.reset(0);
    UpdateSurroundingText();
  }
  TextChanged();
  if (closeKeyboard)
    Finish(true);
  return true;
}

void CWebOSKeyboard::Cancel()
{
  Finish(false);
}

void CWebOSKeyboard::Render()
{
  std::string text;
  CRectInt panel;
  bool hiddenInput;
  {
    std::unique_lock lock(m_mutex);
    if (!m_active)
      return;
    text = m_text;
    text.insert(m_cursor, m_preedit);
    panel = m_inputPanelRect;
    hiddenInput = m_hiddenInput;
  }

  if (panel.IsEmpty())
    return;

  auto& context = CServiceBroker::GetWinSystem()->GetGfxContext();
  context.SetRenderingResolution(context.GetResInfo(), false);

  const float screenWidth = context.GetWidth();
  const float screenHeight = context.GetHeight();
  const float padding = screenWidth * 0.15f;
  const float height = std::max(64.0f, screenHeight * 0.08f);
  const float x = panel.x1;
  const float y = panel.y1 - height;
  const float width = screenWidth - x;

  if (hiddenInput)
    text.assign(CCharsetConverter::utf8ToUtf32(text, false).size(), '*');
  text += '|';

  CGUITexture::DrawQuad(CRect{x, y, x + width, y + height}, 0xff1c1d1e);

  if (!m_previewLayout)
  {
    CGUIFont* font = g_fontManager.GetDefaultFont();
    if (!font)
      return;
    m_previewLayout = std::make_unique<CGUITextLayout>(font, false, height - 2.0f * padding);
  }

  m_previewLayout->Update(text, width - 2.0f * padding);
  const float textY = y + (height - m_previewLayout->GetTextHeight()) * 0.5f;
  m_previewLayout->Render(x + padding, textY, 0.0f, 0xffffffff, 0xff000000, XBFONT_TRUNCATED_LEFT,
                          width - 2.0f * padding);
}

bool CWebOSKeyboard::ConsumeRenderDirty()
{
  return m_renderDirty.exchange(false);
}

void CWebOSKeyboard::Backspace()
{
  {
    std::unique_lock lock(m_mutex);
    if (m_cursor == 0 || m_cursor > m_text.size())
      return;

    std::size_t start = m_cursor - 1;
    while (start > 0 && (static_cast<unsigned char>(m_text[start]) & 0xc0) == 0x80)
      --start;

    m_text.erase(start, m_cursor - start);
    m_cursor = start;
    UpdateSurroundingText();
  }
  TextChanged();
}

void CWebOSKeyboard::CommitPreedit()
{
  if (!m_preedit.empty() || !m_preeditCommit.empty())
  {
    const std::string& text = m_preeditCommit.empty() ? m_preedit : m_preeditCommit;
    m_text.insert(m_cursor, text);
    m_cursor += text.size();
    m_preedit.clear();
    m_preeditCommit.clear();
  }
}

void CWebOSKeyboard::UpdateSurroundingText()
{
  m_textModel.set_surrounding_text(m_text, m_cursor, m_cursor);
  m_textModel.commit();
}

void CWebOSKeyboard::TextChanged()
{
  char_callback_t callback;
  std::string text;
  {
    std::unique_lock lock(m_mutex);
    callback = m_callback;
    text = m_text;
    text.insert(m_cursor, m_preedit);
    m_renderDirty = true;
  }
  if (callback)
    callback(this, text);
}

void CWebOSKeyboard::Finish(const bool confirmed)
{
  std::unique_lock lock(m_mutex);
  if (!m_active || m_finished.Signaled())
    return;
  if (confirmed)
    CommitPreedit();
  m_confirmed = confirmed;
  m_finished.Set();
}

} // namespace KODI::WINDOWING::WAYLAND
