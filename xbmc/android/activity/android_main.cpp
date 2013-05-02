/*
 *      Copyright (C) 2012-2013 Team XBMC
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

#include <stdlib.h>
#include <stdio.h>
#include <dlfcn.h>

#include <android_native_app_glue.h>
#include <jni.h>
#include "JNIThreading.h"

#include "EventLoop.h"
#include "XBMCApp.h"
#include "JNIManager.h"
#include <android/log.h>

extern void android_main(struct android_app* state)
{
  // make sure that the linker doesn't strip out our glue
  app_dummy();
  int val = xbmc_jni_on_load(state->activity->vm, state->activity->env);
  __android_log_print(ANDROID_LOG_VERBOSE, "XBMC","xbmc_jni_on_load returned %i\n",val);
  CAndroidJNIManager::GetInstance().SetActivityInstance(state->activity->clazz);
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
  xbmc_jni_on_unload();
  exit(0);
}
