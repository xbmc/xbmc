/*
 *  Copyright (C) 2016 Christian Browet
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <androidjni/JNIBase.h>

#include <androidjni/AudioManager.h>

class CJNIXBMCAudioManagerOnAudioFocusChangeListener : public CJNIAudioManagerAudioFocusChangeListener, public CJNIInterfaceImplem<CJNIXBMCAudioManagerOnAudioFocusChangeListener>
{
public:
  CJNIXBMCAudioManagerOnAudioFocusChangeListener();
  CJNIXBMCAudioManagerOnAudioFocusChangeListener(const CJNIXBMCAudioManagerOnAudioFocusChangeListener& other);
  explicit CJNIXBMCAudioManagerOnAudioFocusChangeListener(const jni::jhobject &object) : CJNIBase(object) {}
  virtual ~CJNIXBMCAudioManagerOnAudioFocusChangeListener();

  static void RegisterNatives(JNIEnv* env);

  void onAudioFocusChange(int focusChange) override;

protected:
  static void _onAudioFocusChange(JNIEnv* env, jobject thiz, jint focusChange);
};
