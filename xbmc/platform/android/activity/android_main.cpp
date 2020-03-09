/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "EventLoop.h"
#include "XBMCApp.h"

#include "platform/android/activity/JNIMainActivity.h"
#include "platform/android/activity/JNIXBMCAudioManagerOnAudioFocusChangeListener.h"
#include "platform/android/activity/JNIXBMCBroadcastReceiver.h"
#include "platform/android/activity/JNIXBMCConnectivityManagerNetworkCallback.h"
#include "platform/android/activity/JNIXBMCDisplayManagerDisplayListener.h"
#include "platform/android/activity/JNIXBMCFile.h"
#include "platform/android/activity/JNIXBMCJsonHandler.h"
#include "platform/android/activity/JNIXBMCMainView.h"
#include "platform/android/activity/JNIXBMCMediaSession.h"
#include "platform/android/activity/JNIXBMCNsdManagerDiscoveryListener.h"
#include "platform/android/activity/JNIXBMCNsdManagerRegistrationListener.h"
#include "platform/android/activity/JNIXBMCNsdManagerResolveListener.h"
#include "platform/android/activity/JNIXBMCSpeechRecognitionListener.h"
#include "platform/android/activity/JNIXBMCSurfaceTextureOnFrameAvailableListener.h"
#include "platform/android/activity/JNIXBMCTextureCache.h"
#include "platform/android/activity/JNIXBMCURIUtils.h"
#include "platform/android/activity/JNIXBMCVideoView.h"

#include <stdint.h>
#include <stdio.h>
#include <thread>

#include <unistd.h>

using namespace jni;

namespace
{

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

  CJNIMainActivity::RegisterNatives(env);
  CJNIXBMCAudioManagerOnAudioFocusChangeListener::RegisterNatives(env);
  CJNIXBMCBroadcastReceiver::RegisterNatives(env);
  CJNIXBMCConnectivityManagerNetworkCallback::RegisterNatives(env);
  CJNIXBMCDisplayManagerDisplayListener::RegisterNatives(env);
  CJNIXBMCFile::RegisterNatives(env);
  CJNIXBMCJsonHandler::RegisterNatives(env);
  CJNIXBMCMainView::RegisterNatives(env);
  CJNIXBMCMediaSession::RegisterNatives(env);
  CJNIXBMCNsdManagerDiscoveryListener::RegisterNatives(env);
  CJNIXBMCNsdManagerRegistrationListener::RegisterNatives(env);
  CJNIXBMCNsdManagerResolveListener::RegisterNatives(env);
  CJNIXBMCSpeechRecognitionListener::RegisterNatives(env);
  CJNIXBMCSurfaceTextureOnFrameAvailableListener::RegisterNatives(env);
  CJNIXBMCTextureCache::RegisterNatives(env);
  CJNIXBMCURIUtils::RegisterNatives(env);
  CJNIXBMCVideoView::RegisterNatives(env);

  return version;
}
