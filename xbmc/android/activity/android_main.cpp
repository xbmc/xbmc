/*
 *      Copyright (C) 2012-2013 Team XBMC
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

#include <stdlib.h>
#include <android_native_app_glue.h>
#include "EventLoop.h"
#include "XBMCApp.h"

extern void android_main(struct android_app* state)
{
  {
    // make sure that the linker doesn't strip out our glue
    app_dummy();
    CEventLoop eventLoop(state);
    CXBMCApp xbmcApp(state->activity);
    if (xbmcApp.isValid())
    {
      IInputHandler inputHandler;
      eventLoop.run(xbmcApp, inputHandler);
    }
    else
      CXBMCApp::android_printf("android_main: setup failed");

    CXBMCApp::android_printf("android_main: Exiting");
    // We need to call exit() so that all loaded libraries are properly unloaded
    // otherwise on the next start of the Activity android will simple re-use
    // those loaded libs in the state they were in when we quit XBMC last time
    // which will lead to crashes because of global/static classes that haven't
    // been properly uninitialized
  }
  exit(0);
}

extern "C" JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *reserved)
{
  jint version = JNI_VERSION_1_6;
  JNIEnv* env;
  if (vm->GetEnv(reinterpret_cast<void**>(&env), version) != JNI_OK)
    return -1;

  jclass cMain = env->FindClass("org/xbmc/xbmc/XBMCBroadcastReceiver");
  if(cMain)
  {
    JNINativeMethod mOnReceive =   { "_onReceive",     "(Landroid/content/Intent;)V", (void*)&CJNIBroadcastReceiver::_onReceive};
    env->RegisterNatives(cMain, &mOnReceive, 1);
  }

  jclass cBroadcastReceiver = env->FindClass("org/xbmc/xbmc/Main");
  if(cBroadcastReceiver)
  {
    JNINativeMethod mOnNewIntent = { "_onNewIntent",   "(Landroid/content/Intent;)V", (void*)&CJNIContext::_onNewIntent};
    env->RegisterNatives(cBroadcastReceiver, &mOnNewIntent, 1);
  }
  return version;
}
