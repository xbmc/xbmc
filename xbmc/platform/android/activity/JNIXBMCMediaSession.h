#pragma once
/*
 *      Copyright (C) 2017 Christian Browet
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

  bool m_isActive;
};

}
