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

class CJNIBuild
{
public:
  static std::string UNKNOWN;
  static std::string ID;
  static std::string DISPLAY;
  static std::string PRODUCT;
  static std::string DEVICE;
  static std::string BOARD;
  static std::string CPU_ABI;
  static std::string CPU_ABI2;
  static std::string MANUFACTURER;
  static std::string BRAND;
  static std::string MODEL;
  static std::string BOOTLOADER;
  static std::string RADIO;
  static std::string HARDWARE;
  static std::string SERIAL;
  static std::string TYPE;
  static std::string TAGS;
  static std::string FINGERPRINT;
  static int64_t TIME;
  static std::string USER;
  static std::string HOST;
  static int SDK_INT;
  static std::string getRadioVersion();

  static void PopulateStaticFields();
private:
  CJNIBuild();
  ~CJNIBuild() {};
  static const char *m_classname;
};
