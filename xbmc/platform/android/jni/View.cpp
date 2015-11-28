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

#include "Window.h"
#include "View.h"
#include "Display.h"

#include "jutils/jutils-details.hpp"

using namespace jni;

/************************************************************************/
/************************************************************************/
int CJNIViewInputDeviceMotionRange::getAxis() const
{
  return call_method<int>(m_object,
    "getAxis", "()I");
}

float CJNIViewInputDeviceMotionRange::getFlat() const
{
  return call_method<jfloat>(m_object,
    "getFlat", "()F");
}

float CJNIViewInputDeviceMotionRange::getFuzz() const
{
  return call_method<jfloat>(m_object,
    "getFuzz", "()F");
}

float	CJNIViewInputDeviceMotionRange::getMax() const
{
  return call_method<jfloat>(m_object,
    "getMax", "()F");
}

float	CJNIViewInputDeviceMotionRange::getMin() const
{
  return call_method<jfloat>(m_object,
    "getMin", "()F");
}

float	CJNIViewInputDeviceMotionRange::getRange() const
{
  return call_method<jfloat>(m_object,
    "getRange", "()F");
}

int CJNIViewInputDeviceMotionRange::getSource() const
{
  return call_method<int>(m_object,
    "getSource", "()I");
}

/************************************************************************/
/************************************************************************/
const char *CJNIViewInputDevice::m_classname = "platform/android/view/InputDevice";

const CJNIViewInputDevice CJNIViewInputDevice::getDevice(int id)
{
  return call_static_method<jhobject>(m_classname,
    "getDevice", "(I)Landroid/view/InputDevice;",
    id);
}

std::string CJNIViewInputDevice::getName() const
{
  return jcast<std::string>(call_method<jhstring>(m_object,
    "getName", "()Ljava/lang/String;"));
}

int CJNIViewInputDevice::getSources() const
{
  return call_method<int>(m_object,
    "getSources", "()I");
}

const CJNIList<CJNIViewInputDeviceMotionRange> CJNIViewInputDevice::getMotionRanges() const
{
  return call_method<jhobject>(m_object,
    "getMotionRanges", "()Ljava/util/List;");
}

const CJNIViewInputDeviceMotionRange CJNIViewInputDevice::getMotionRange(int axis) const
{
  return call_method<jhobject>(m_object,
    "getMotionRange", "(I)Landroid/view/InputDevice$MotionRange;",
    axis);
}

const CJNIViewInputDeviceMotionRange CJNIViewInputDevice::getMotionRange(int axis, int source) const
{
  return call_method<jhobject>(m_object,
    "getMotionRange", "(II)Landroid/view/InputDevice$MotionRange;",
    axis, source);
}

/************************************************************************/
/************************************************************************/
int CJNIView::SYSTEM_UI_FLAG_FULLSCREEN(0);
int CJNIView::SYSTEM_UI_FLAG_HIDE_NAVIGATION(0);
int CJNIView::SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN(0);
int CJNIView::SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION(0);
int CJNIView::SYSTEM_UI_FLAG_LAYOUT_STABLE(0);
int CJNIView::SYSTEM_UI_FLAG_LOW_PROFILE(0);
int CJNIView::SYSTEM_UI_FLAG_VISIBLE(0);

void CJNIView::PopulateStaticFields()
{
  jhclass clazz = find_class("platform/android/view/View");
  if(GetSDKVersion() >= 16)
  {
    SYSTEM_UI_FLAG_FULLSCREEN              = (get_static_field<int>(clazz, "SYSTEM_UI_FLAG_FULLSCREEN"));
    SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN       = (get_static_field<int>(clazz, "SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN"));
    SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION  = (get_static_field<int>(clazz, "SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION"));
    SYSTEM_UI_FLAG_LAYOUT_STABLE           = (get_static_field<int>(clazz, "SYSTEM_UI_FLAG_LAYOUT_STABLE"));
  }
  SYSTEM_UI_FLAG_HIDE_NAVIGATION           = (get_static_field<int>(clazz, "SYSTEM_UI_FLAG_HIDE_NAVIGATION"));
  SYSTEM_UI_FLAG_LOW_PROFILE               = (get_static_field<int>(clazz, "SYSTEM_UI_FLAG_LOW_PROFILE"));
  SYSTEM_UI_FLAG_VISIBLE                   = (get_static_field<int>(clazz, "SYSTEM_UI_FLAG_VISIBLE"));
}

void CJNIView::setSystemUiVisibility(int visibility)
{
  call_method<void>(m_object,
    "setSystemUiVisibility", "(I)V", visibility);
}

int CJNIView::getSystemUiVisibility()
{
  return call_method<int>(m_object,
    "getSystemUiVisibility", "()I");
}

CJNIDisplay CJNIView::getDisplay()
{
  if (GetSDKVersion() >= 17)
    return call_method<jhobject>(m_object,
      "getDisplay", "()Landroid/view/Display;");
  else
    return jhobject();
}
