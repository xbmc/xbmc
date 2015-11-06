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

class CJNIKeyEventAudioFocusChangeListener : public CJNIBase
{
public:
  CJNIKeyEventAudioFocusChangeListener(const jni::jhobject &object) : CJNIBase(object) {};
  virtual ~CJNIKeyEventAudioFocusChangeListener() {};

  static void _onAudioFocusChange(JNIEnv *env, jobject context, jint focusChange);

protected:
  CJNIKeyEventAudioFocusChangeListener();

  virtual void onAudioFocusChange(int focusChange)=0;

private:
  static CJNIKeyEventAudioFocusChangeListener *m_listenerInstance;
};

class CJNIKeyEvent : public CJNIBase
{
public:
  CJNIKeyEvent(const jni::jhobject &object) : CJNIBase(object) {};
  ~CJNIKeyEvent() {};

  int getKeyCode();
  int getAction();

  static void PopulateStaticFields();
  static int ACTION_DOWN;
  static int ACTION_UP;

  static int KEYCODE_MEDIA_RECORD;
  static int KEYCODE_MEDIA_EJECT;
  static int KEYCODE_MEDIA_FAST_FORWARD;
  static int KEYCODE_MEDIA_NEXT ;
  static int KEYCODE_MEDIA_PAUSE;
  static int KEYCODE_MEDIA_PLAY;
  static int KEYCODE_MEDIA_PLAY_PAUSE;
  static int KEYCODE_MEDIA_PREVIOUS;
  static int KEYCODE_MEDIA_REWIND;
  static int KEYCODE_MEDIA_STOP;

private:
  CJNIKeyEvent();
};

