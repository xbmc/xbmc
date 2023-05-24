/*
 *  Copyright (C) 2012-2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <androidjni/JNIBase.h>

namespace jni
{

class CJNIXBMCBroadcastReceiver
{
public:
  static void RegisterNatives(JNIEnv* env);
};

} // namespace jni
