/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

#include <androidjni/JNIBase.h>

namespace jni
{

/*!
 * \brief Callback interface implemented by the native consumer of
 *        CJNIXBMCNativeKeyboard to receive soft keyboard input events.
 */
class IKeyboardInputHandler
{
public:
  virtual ~IKeyboardInputHandler() = default;

  virtual void OnTextChanged(const std::string& text) = 0;
  virtual void OnInputFinished(const std::string& text) = 0;
  virtual void OnInputCanceled() = 0;
};

class CJNIXBMCNativeKeyboard : public CJNIBase, public CJNIInterfaceImplem<CJNIXBMCNativeKeyboard>
{
public:
  explicit CJNIXBMCNativeKeyboard(IKeyboardInputHandler* handler);
  ~CJNIXBMCNativeKeyboard() override;

  static void RegisterNatives(JNIEnv* env);

  void show(const std::string& heading, const std::string& initialText, bool hiddenInput);
  void setText(const std::string& text);
  void hide();

protected:
  static void _onTextChanged(JNIEnv* env, jobject thiz, jstring text);
  static void _onInputFinished(JNIEnv* env, jobject thiz, jstring text);
  static void _onInputCanceled(JNIEnv* env, jobject thiz);

private:
  IKeyboardInputHandler* m_handler{nullptr};
};

} // namespace jni
