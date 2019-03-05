/*
 *  Copyright (C) 2017 Christian Browet
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <androidjni/JNIBase.h>

#include <androidjni/Intent.h>
#include <androidjni/MediaMetadata.h>
#include <androidjni/PlaybackState.h>

namespace jni
{

class CJNIXBMCMediaSession : public CJNIBase, public CJNIInterfaceImplem<CJNIXBMCMediaSession>
{
public:
  CJNIXBMCMediaSession();
  CJNIXBMCMediaSession(const CJNIXBMCMediaSession& other);
  CJNIXBMCMediaSession(const jni::jhobject &object) : CJNIBase(object) {}
  virtual ~CJNIXBMCMediaSession();

  static void RegisterNatives(JNIEnv* env);

  void activate(bool state);
  void updatePlaybackState(const CJNIPlaybackState& state);
  void updateMetadata(const CJNIMediaMetadata& myData);
  void updateIntent(const CJNIIntent& intent);

  void OnPlayRequested();
  void OnPauseRequested();
  void OnNextRequested();
  void OnPreviousRequested();
  void OnForwardRequested();
  void OnRewindRequested();
  void OnStopRequested();
  void OnSeekRequested(int64_t pos);
  bool OnMediaButtonEvent(CJNIIntent intent);
  bool isActive() const;

protected:
  static void _onPlayRequested(JNIEnv* env, jobject thiz);
  static void _onPauseRequested(JNIEnv* env, jobject thiz);
  static void _onNextRequested(JNIEnv* env, jobject thiz);
  static void _onPreviousRequested(JNIEnv* env, jobject thiz);
  static void _onForwardRequested(JNIEnv* env, jobject thiz);
  static void _onRewindRequested(JNIEnv* env, jobject thiz);
  static void _onStopRequested(JNIEnv* env, jobject thiz);
  static void _onSeekRequested(JNIEnv* env, jobject thiz, jlong pos);
  static bool _onMediaButtonEvent(JNIEnv* env, jobject thiz, jobject intent);

  bool m_isActive;
};

}
