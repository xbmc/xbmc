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

#include "KeyEvent.h"

#include "jutils/jutils-details.hpp"

using namespace jni;

int CJNIKeyEvent::ACTION_DOWN;
int CJNIKeyEvent::ACTION_UP;

int CJNIKeyEvent::KEYCODE_MEDIA_RECORD;
int CJNIKeyEvent::KEYCODE_MEDIA_EJECT;
int CJNIKeyEvent::KEYCODE_MEDIA_FAST_FORWARD;
int CJNIKeyEvent::KEYCODE_MEDIA_NEXT ;
int CJNIKeyEvent::KEYCODE_MEDIA_PAUSE;
int CJNIKeyEvent::KEYCODE_MEDIA_PLAY;
int CJNIKeyEvent::KEYCODE_MEDIA_PLAY_PAUSE;
int CJNIKeyEvent::KEYCODE_MEDIA_PREVIOUS;
int CJNIKeyEvent::KEYCODE_MEDIA_REWIND;
int CJNIKeyEvent::KEYCODE_MEDIA_STOP;

void CJNIKeyEvent::PopulateStaticFields()
{
  jhclass clazz = find_class("android/view/KeyEvent");
  ACTION_DOWN  = (get_static_field<int>(clazz, "ACTION_DOWN"));
  ACTION_UP  = (get_static_field<int>(clazz, "ACTION_UP"));
  KEYCODE_MEDIA_RECORD  = (get_static_field<int>(clazz, "KEYCODE_MEDIA_RECORD"));
  KEYCODE_MEDIA_EJECT  = (get_static_field<int>(clazz, "KEYCODE_MEDIA_EJECT"));
  KEYCODE_MEDIA_FAST_FORWARD  = (get_static_field<int>(clazz, "KEYCODE_MEDIA_FAST_FORWARD"));
  KEYCODE_MEDIA_NEXT  = (get_static_field<int>(clazz, "KEYCODE_MEDIA_NEXT"));
  KEYCODE_MEDIA_PAUSE  = (get_static_field<int>(clazz, "KEYCODE_MEDIA_PAUSE"));
  KEYCODE_MEDIA_PLAY  = (get_static_field<int>(clazz, "KEYCODE_MEDIA_PLAY"));
  KEYCODE_MEDIA_PLAY_PAUSE  = (get_static_field<int>(clazz, "KEYCODE_MEDIA_PLAY_PAUSE"));
  KEYCODE_MEDIA_PREVIOUS  = (get_static_field<int>(clazz, "KEYCODE_MEDIA_PREVIOUS"));
  KEYCODE_MEDIA_REWIND  = (get_static_field<int>(clazz, "KEYCODE_MEDIA_REWIND"));
  KEYCODE_MEDIA_STOP  = (get_static_field<int>(clazz, "KEYCODE_MEDIA_STOP"));
}

int CJNIKeyEvent::getKeyCode()
{
  return call_method<jint>(m_object,
                           "getKeyCode", "()I");
}

int CJNIKeyEvent::getAction()
{
  return call_method<jint>(m_object,
                           "getAction", "()I");
}
