/*
 *  Copyright (C) 2012-2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <androidjni/JNIBase.h>
#include <androidjni/Network.h>

namespace jni
{

class CJNIXBMCConnectivityManagerNetworkCallback
  : public CJNIBase,
    public CJNIInterfaceImplem<CJNIXBMCConnectivityManagerNetworkCallback>
{
public:
  CJNIXBMCConnectivityManagerNetworkCallback();
  ~CJNIXBMCConnectivityManagerNetworkCallback() override;

  static void RegisterNatives(JNIEnv* env);

  virtual void onAvailable(const CJNINetwork network) = 0;
  virtual void onLost(const CJNINetwork network) = 0;

protected:
  static void _onAvailable(JNIEnv* env, jobject thiz, jobject network);
  static void _onLost(JNIEnv* env, jobject thiz, jobject network);
};

} // namespace jni
