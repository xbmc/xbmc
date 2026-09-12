/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/GUIKeyboard.h"
#include "threads/CriticalSection.h"
#include "threads/Event.h"
#include "utils/Geometry.h"

#include <atomic>
#include <cstdint>
#include <memory>

#include <wayland-webos-protocols.hpp>

class CGUITextLayout;

namespace KODI::WINDOWING::WAYLAND
{

class CWinSystemWaylandWebOS;

/**
 * \brief Native on-screen keyboard using the webOS Wayland text model protocol.
 *
 * The keyboard delegates text entry and rendering to the webOS compositor. Protocol events are
 * translated into Kodi keyboard callbacks while \ref ShowAndGetInput waits for the user to either
 * confirm or cancel the input.
 */
class CWebOSKeyboard : public CGUIKeyboard
{
public:
  CWebOSKeyboard();
  ~CWebOSKeyboard() override;

  /**
   * \brief Show the native webOS keyboard and wait for it to close.
   * \param callback Callback invoked whenever the complete input text changes.
   * \param initialString Text with which to initialize the keyboard.
   * \param typedString Receives the entered text when the input is confirmed.
   * \param heading Dialog heading supplied by the caller. The webOS text model does not display it.
   * \param hiddenInput Whether to request password-style input from the compositor.
   * \return True when the user confirmed the input, false when it was canceled or could not be
   * opened.
   */
  bool ShowAndGetInput(char_callback_t callback,
                       const std::string& initialString,
                       std::string& typedString,
                       const std::string& heading,
                       bool hiddenInput) override;

  /**
   * \brief Replace the current keyboard text.
   * \param text New complete input text.
   * \param closeKeyboard Whether to confirm and close the keyboard after updating the text.
   * \return True when a native keyboard is active, otherwise false.
   */
  bool SetTextToKeyboard(const std::string& text, bool closeKeyboard = false) override;

  /**
   * \brief Cancel the active keyboard and unblock \ref ShowAndGetInput.
   */
  void Cancel() override;

  /** \brief Render the current input above the compositor-owned keyboard. */
  void Render() override;

  /** \brief Return whether the keyboard preview needs to be redrawn. */
  bool ConsumeRenderDirty() override;

private:
  void Backspace();
  void CommitPreedit();
  void UpdateSurroundingText();
  void TextChanged();
  void Finish(bool confirmed);

  CWinSystemWaylandWebOS* m_winSystem{nullptr};
  wayland::text_model_t m_textModel;
  CCriticalSection m_mutex;
  CEvent m_finished{true};
  std::string m_text;
  std::string m_preedit;
  std::string m_preeditCommit;
  std::size_t m_cursor{0};
  char_callback_t m_callback{nullptr};
  bool m_confirmed{false};
  bool m_active{false};
  bool m_panelShown{false};
  bool m_hiddenInput{false};
  CRectInt m_inputPanelRect;
  std::unique_ptr<CGUITextLayout> m_previewLayout;
  std::atomic_bool m_renderDirty{false};
};

} // namespace KODI::WINDOWING::WAYLAND
