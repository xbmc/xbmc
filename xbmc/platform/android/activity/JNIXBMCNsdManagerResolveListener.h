#pragma once
/*
 *      Copyright (C) 2016 Christian Browet
 *      http://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include <androidjni/JNIBase.h>

#include <androidjni/NsdManager.h>

namespace jni
{

class CJNIXBMCNsdManagerResolveListener : public CJNINsdManagerResolveListener, public CJNIInterfaceImplem<CJNIXBMCNsdManagerResolveListener>
{
public:
public:
  CJNIXBMCNsdManagerResolveListener();  
  CJNIXBMCNsdManagerResolveListener(const CJNIXBMCNsdManagerResolveListener& other); 
  explicit CJNIXBMCNsdManagerResolveListener(const jni::jhobject &object) : CJNIBase(object) {}
  virtual ~CJNIXBMCNsdManagerResolveListener();

  static void RegisterNatives(JNIEnv* env);
  
  // CJNINsdManagerResolveListener interface
  void onResolveFailed(const CJNINsdServiceInfo& serviceInfo, int errorCode) = 0;
  void onServiceResolved(const CJNINsdServiceInfo& serviceInfo) = 0;

protected:
  static void _onResolveFailed(JNIEnv* env, jobject thiz, jobject serviceInfo, jint errorCode);
  static void _onServiceResolved(JNIEnv* env, jobject thiz, jobject serviceInfo);
  
};

}

