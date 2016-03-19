#pragma once
/*
 *      Copyright (C) 2013 Team XBMC
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

#include "JNIBase.h"

class CJNIAudioManagerAudioFocusChangeListener : public CJNIBase
{
public:
  CJNIAudioManagerAudioFocusChangeListener(const jni::jhobject &object) : CJNIBase(object) {};
  virtual ~CJNIAudioManagerAudioFocusChangeListener() {};

  static void _onAudioFocusChange(JNIEnv *env, jobject context, jint focusChange);

protected:
  CJNIAudioManagerAudioFocusChangeListener();

  virtual void onAudioFocusChange(int focusChange)=0;

private:
  static CJNIAudioManagerAudioFocusChangeListener *m_listenerInstance;
};

class CJNIAudioManager : public CJNIBase
{
public:
  CJNIAudioManager(const jni::jhobject &object) : CJNIBase(object) {};
  ~CJNIAudioManager() {};

  // Note removal of streamType param.
  int  getStreamMaxVolume();
  int  getStreamVolume();
  void setStreamVolume(int index = 0, int flags = 0);

  int requestAudioFocus(const CJNIAudioManagerAudioFocusChangeListener &listener, int streamType, int durationHint);
  int abandonAudioFocus (const CJNIAudioManagerAudioFocusChangeListener &listener);
  bool isBluetoothA2dpOn();
  bool isWiredHeadsetOn();

  static void PopulateStaticFields();
  static int STREAM_MUSIC;

  static int AUDIOFOCUS_GAIN;
  static int AUDIOFOCUS_LOSS;
  static int AUDIOFOCUS_REQUEST_GRANTED;
  static int AUDIOFOCUS_REQUEST_FAILED;

private:
  CJNIAudioManager();
};

