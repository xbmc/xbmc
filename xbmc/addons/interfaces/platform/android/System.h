/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

namespace ADDON
{

struct Interface_Android
{
  static void Register();
  static void* Get(const std::string &name, const std::string &version);

  static void* get_jni_env();
  static int get_sdk_version();
  static const char *get_class_name();
};

} //namespace ADDON
