/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "CompileInfo.h"
#include "EventLoop.h"
#include "XBMCApp.h"

#include "platform/android/activity/JNIMainActivity.h"
#include "platform/android/activity/JNIXBMCAudioManagerOnAudioFocusChangeListener.h"
#include "platform/android/activity/JNIXBMCDisplayManagerDisplayListener.h"
#include "platform/android/activity/JNIXBMCFile.h"
#include "platform/android/activity/JNIXBMCJsonHandler.h"
#include "platform/android/activity/JNIXBMCMainView.h"
#include "platform/android/activity/JNIXBMCMediaSession.h"
#include "platform/android/activity/JNIXBMCNsdManagerDiscoveryListener.h"
#include "platform/android/activity/JNIXBMCNsdManagerRegistrationListener.h"
#include "platform/android/activity/JNIXBMCNsdManagerResolveListener.h"
#include "platform/android/activity/JNIXBMCSurfaceTextureOnFrameAvailableListener.h"
#include "platform/android/activity/JNIXBMCURIUtils.h"
#include "platform/android/activity/JNIXBMCVideoView.h"

#include <stdint.h>
#include <stdio.h>
#include <thread>

#include <android_native_app_glue.h>
#include <androidjni/SurfaceTexture.h>
#include <unistd.h>

namespace
{
// copied from new android_native_app_glue.c
static void process_input(struct android_app* app, struct android_poll_source* source) {
    AInputEvent* event = NULL;
    int processed = 0;
    while (AInputQueue_getEvent(app->inputQueue, &event) >= 0) {
        if (AInputQueue_preDispatchEvent(app->inputQueue, event)) {
            continue;
        }
        int32_t handled = 0;
        if (app->onInputEvent != NULL) handled = app->onInputEvent(app, event);
        AInputQueue_finishEvent(app->inputQueue, event, handled);
        processed = 1;
    }
    if (processed == 0 && errno != EAGAIN) {
        CXBMCApp::android_printf("process_input: Failure reading next input event: %s", strerror(errno));
    }
}

class LogRedirector
{
  // redirect stdout / stderr to logcat
  // based on https://codelab.wordpress.com/2014/11/03/how-to-use-standard-output-streams-for-logging-in-android-apps/

public:
  LogRedirector()
  {
    // make stdout line-buffered and stderr unbuffered
    setvbuf(stdout, 0, _IOLBF, 0);
    setvbuf(stderr, 0, _IONBF, 0);

    // spawn the logging thread
    std::thread thread(&LogRedirector::Run);
    thread.detach();
  }

private:
  static void Run()
  {
    // create a pipe and redirect stdout and stderr
    int pfds[2];
    pipe(pfds);
    dup2(pfds[1], STDOUT_FILENO);
    dup2(pfds[1], STDERR_FILENO);

    ssize_t rdsz;
    char buf[128];
    while ((rdsz = read(pfds[0], buf, sizeof(buf) - 1)) > 0)
    {
      if (buf[rdsz - 1] == '\n')
        --rdsz;

      buf[rdsz] = 0; // add null-terminator
      __android_log_write(ANDROID_LOG_DEBUG, "Kodi", buf);
    }
  }
};

LogRedirector g_LogRedirector;

} // namespace

extern void android_main(struct android_app* state)
{
  {
    // revector inputPollSource.process so we can shut up
    // its useless verbose logging on new events (see ouya)
    // and fix the error in handling multiple input events.
    // see https://code.google.com/p/android/issues/detail?id=41755
    state->inputPollSource.process = process_input;

    CEventLoop eventLoop(state);
    IInputHandler inputHandler;
    CXBMCApp& theApp = CXBMCApp::Create(state->activity, inputHandler);
    if (theApp.isValid())
    {
      eventLoop.run(theApp, inputHandler);
      theApp.Quit();
    }
    else
      CXBMCApp::android_printf("android_main: setup failed");

    CXBMCApp::android_printf("android_main: Exiting");

    CXBMCApp::Destroy();
  }
  // We need to call exit() so that all loaded libraries are properly unloaded
  // otherwise on the next start of the Activity android will simply re-use
  // those loaded libs in the state they were in when we quit Kodi last time
  // which will lead to crashes because of global/static classes that haven't
  // been properly uninitialized
  exit(0);
}

extern "C" JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *reserved)
{
  jint version = JNI_VERSION_1_6;
  JNIEnv* env;
  if (vm->GetEnv(reinterpret_cast<void**>(&env), version) != JNI_OK)
    return -1;

  std::string pkgRoot = CCompileInfo::GetClass();

  const std::string mainClass = pkgRoot + "/Main";
  const std::string bcReceiver = pkgRoot + "/XBMCBroadcastReceiver";
  const std::string settingsObserver = pkgRoot + "/XBMCSettingsContentObserver";
  const std::string inputDeviceListener = pkgRoot + "/XBMCInputDeviceListener";

  CJNIXBMCAudioManagerOnAudioFocusChangeListener::RegisterNatives(env);
  CJNIXBMCSurfaceTextureOnFrameAvailableListener::RegisterNatives(env);
  CJNIXBMCMainView::RegisterNatives(env);
  CJNIXBMCVideoView::RegisterNatives(env);
  CJNIXBMCDisplayManagerDisplayListener::RegisterNatives(env);

  jni::CJNIXBMCNsdManagerDiscoveryListener::RegisterNatives(env);
  jni::CJNIXBMCNsdManagerRegistrationListener::RegisterNatives(env);
  jni::CJNIXBMCNsdManagerResolveListener::RegisterNatives(env);
  jni::CJNIXBMCMediaSession::RegisterNatives(env);
  jni::CJNIXBMCJsonHandler::RegisterNatives(env);
  jni::CJNIXBMCFile::RegisterNatives(env);
  jni::CJNIXBMCURIUtils::RegisterNatives(env);

  jclass cMain = env->FindClass(mainClass.c_str());
  if(cMain)
  {
    JNINativeMethod methods[] =
    {
      {"_onNewIntent", "(Landroid/content/Intent;)V", (void*)&CJNIMainActivity::_onNewIntent},
      {"_onActivityResult", "(IILandroid/content/Intent;)V", (void*)&CJNIMainActivity::_onActivityResult},
      {"_doFrame", "(J)V", (void*)&CJNIMainActivity::_doFrame},
      {"_callNative", "(JJ)V", (void*)&CJNIMainActivity::_callNative},
      {"_onVisibleBehindCanceled", "()V", (void*)&CJNIMainActivity::_onVisibleBehindCanceled},
    };
    env->RegisterNatives(cMain, methods, sizeof(methods)/sizeof(methods[0]));
  }

  jclass cBroadcastReceiver = env->FindClass(bcReceiver.c_str());
  if(cBroadcastReceiver)
  {
    JNINativeMethod methods[] =
    {
      {"_onReceive", "(Landroid/content/Intent;)V", (void*)&CJNIBroadcastReceiver::_onReceive},
    };
    env->RegisterNatives(cBroadcastReceiver, methods, sizeof(methods)/sizeof(methods[0]));
  }

  jclass cSettingsObserver = env->FindClass(settingsObserver.c_str());
  if(cSettingsObserver)
  {
    JNINativeMethod methods[] =
    {
      {"_onVolumeChanged", "(I)V", (void*)&CJNIMainActivity::_onVolumeChanged},
    };
    env->RegisterNatives(cSettingsObserver, methods, sizeof(methods)/sizeof(methods[0]));
  }

  jclass cInputDeviceListener = env->FindClass(inputDeviceListener.c_str());
  if(cInputDeviceListener)
  {
    JNINativeMethod methods[] =
    {
      { "_onInputDeviceAdded", "(I)V", (void*)&CJNIMainActivity::_onInputDeviceAdded },
      { "_onInputDeviceChanged", "(I)V", (void*)&CJNIMainActivity::_onInputDeviceChanged },
      { "_onInputDeviceRemoved", "(I)V", (void*)&CJNIMainActivity::_onInputDeviceRemoved }
    };
    env->RegisterNatives(cInputDeviceListener, methods, sizeof(methods)/sizeof(methods[0]));
  }

  return version;
}
