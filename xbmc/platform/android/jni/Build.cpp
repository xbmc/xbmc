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

#include "Build.h"
#include "jutils/jutils-details.hpp"

using namespace jni;
const char *CJNIBuild::m_classname = "android/os/Build";

std::string CJNIBuild::UNKNOWN;
std::string CJNIBuild::DISPLAY;
std::string CJNIBuild::PRODUCT;
std::string CJNIBuild::DEVICE;
std::string CJNIBuild::BOARD;
std::string CJNIBuild::CPU_ABI;
std::string CJNIBuild::CPU_ABI2;
std::string CJNIBuild::MANUFACTURER;
std::string CJNIBuild::BRAND;
std::string CJNIBuild::MODEL;
std::string CJNIBuild::BOOTLOADER;
std::string CJNIBuild::RADIO;
std::string CJNIBuild::HARDWARE;
std::string CJNIBuild::SERIAL;
std::string CJNIBuild::TAGS;
std::string CJNIBuild::FINGERPRINT;
int64_t CJNIBuild::TIME;
std::string CJNIBuild::USER;
std::string CJNIBuild::HOST;
int CJNIBuild::SDK_INT;

void CJNIBuild::PopulateStaticFields()
{
  UNKNOWN = jcast<std::string>(get_static_field<jhstring>(m_classname,"UNKNOWN"));
  DISPLAY = jcast<std::string>(get_static_field<jhstring>(m_classname,"DISPLAY"));
  PRODUCT = jcast<std::string>(get_static_field<jhstring>(m_classname,"PRODUCT"));
  DEVICE = jcast<std::string>(get_static_field<jhstring>(m_classname,"DEVICE"));
  BOARD = jcast<std::string>(get_static_field<jhstring>(m_classname,"BOARD"));
  CPU_ABI = jcast<std::string>(get_static_field<jhstring>(m_classname,"CPU_ABI"));
  CPU_ABI2 = jcast<std::string>(get_static_field<jhstring>(m_classname,"CPU_ABI2"));
  MANUFACTURER = jcast<std::string>(get_static_field<jhstring>(m_classname,"MANUFACTURER"));
  BRAND = jcast<std::string>(get_static_field<jhstring>(m_classname,"BRAND"));
  MODEL = jcast<std::string>(get_static_field<jhstring>(m_classname,"MODEL"));
  BOOTLOADER = jcast<std::string>(get_static_field<jhstring>(m_classname,"BOOTLOADER"));
  RADIO = jcast<std::string>(get_static_field<jhstring>(m_classname,"RADIO"));
  HARDWARE = jcast<std::string>(get_static_field<jhstring>(m_classname,"HARDWARE"));
  SERIAL = jcast<std::string>(get_static_field<jhstring>(m_classname,"SERIAL"));
  TAGS = jcast<std::string>(get_static_field<jhstring>(m_classname,"TAGS"));
  FINGERPRINT = jcast<std::string>(get_static_field<jhstring>(m_classname,"FINGERPRINT"));
  TIME = get_static_field<jlong>(m_classname,"TIME");
  USER = jcast<std::string>(get_static_field<jhstring>(m_classname,"USER"));
  HOST = jcast<std::string>(get_static_field<jhstring>(m_classname,"HOST"));
  SDK_INT = get_static_field<jint>((std::string(m_classname)+"$VERSION").c_str(),"SDK_INT");
}

std::string CJNIBuild::getRadioVersion()
{
  return jcast<std::string>(call_static_method<jhstring>(m_classname,
    "getRadioVersion", "()Ljava/lang/String;"));
}
