/*
 *      Copyright (C) 2013 Team XBMC
 *      http://www.xbmc.org
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
#include <string>
#include <jni.h>
#include "Intents.h"

extern "C"{

  static void jni_ReceiveIntent(JNIEnv *env, jobject thiz, jobject intent)
  {
     CAndroidIntents::getInstance().ReceiveIntent(env, intent);
  }

  static JNINativeMethod jniMethods[] =
  {
    {"ReceiveIntent", "(Landroid/content/Intent;)V", (void*)jni_ReceiveIntent}
  };

  // This is a special function called when libxbmc is loaded. It sets up our
  // internal functions so that we can use them in native code much more simply.
  // It loads a array of methods, params, and function-pointers.
  jint JNI_OnLoad(JavaVM* vm, void* reserved)
  {
    JNIEnv* env;
    if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
        return -1;
    }
    jclass jniClass = env->FindClass("org/xbmc/xbmc/XBMCBroadcastReceiver");
    env->RegisterNatives(jniClass, jniMethods, sizeof(jniMethods) / sizeof(jniMethods[0]));

    return JNI_VERSION_1_6;
  }
}
