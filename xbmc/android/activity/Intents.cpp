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
#include "Intents.h"
#include "utils/log.h"
#include "XBMCApp.h"
#include "JNIThreading.h"
#include <android/native_activity.h>

#include "ApplicationMessenger.h"
#include "AppParamParser.h"
#include "URL.h"
#include "utils/URIUtils.h"

//Intents
#define ACTION_VIEW "android.intent.action.VIEW"

CAndroidIntents::CAndroidIntents()
{
}

void CAndroidIntents::ReceiveIntent(JNIEnv *, const jobject &intent)
{
  JNIEnv* env = xbmc_jnienv();

  std::string action = GetIntentAction(env, intent);
  CLog::Log(LOGDEBUG,"CAndroidIntents::ReceiveIntent: %s", action.c_str());
}

void CAndroidIntents::ReceiveInitialIntent(ANativeActivity *activity)
{
  JNIEnv* env = xbmc_jnienv();

  jobject oActivity = activity->clazz;
  jclass cActivity = env->GetObjectClass(oActivity);

  // oIntent = getIntent();
  jmethodID mgetIntent = env->GetMethodID(cActivity, "getIntent", "()Landroid/content/Intent;");
  jobject oIntent = env->CallObjectMethod(oActivity, mgetIntent);
  
  CStdString action = GetIntentAction(env, oIntent);
  if (action.compare(ACTION_VIEW) == 0)
  {
    CStdString data = GetIntentData(env, oIntent);
    CURL dataUrl(data);
    if (!URIUtils::ProtocolHasEncodedFilename(dataUrl.GetProtocol()))
      CURL::Decode(data);

    CXBMCApp::android_printf("CAndroidIntents::ReceiveInitialIntent: %s - %s", action.c_str(), data.c_str());

    int argc = 2;
    const char** argv = (const char**) malloc(argc*sizeof(char*));
    
    CStdString exe_name("XBMC");
    argv[0] = exe_name.c_str();
    argv[1] = data.c_str();

    CAppParamParser appParamParser;
    appParamParser.Parse((const char **)argv, argc);

    free(argv);
  }

  env->DeleteLocalRef(oIntent);
}

void CAndroidIntents::ReceiveViewIntent(JNIEnv *, const jobject &intent)
{
  JNIEnv* env = xbmc_jnienv();

  CStdString data = GetIntentData(env, intent);
  CURL dataUrl(data);
  if (!URIUtils::ProtocolHasEncodedFilename(dataUrl.GetProtocol()))
    CURL::Decode(data);
  
  CXBMCApp::android_printf("CAndroidIntents::ReceiveViewIntent: %s", data.c_str());
  
  if (data.size() > 0)
      s_messenger.MediaPlay(data);
}

std::string CAndroidIntents::GetIntentAction(JNIEnv *env, const jobject &intent)
{
  std::string action;

 if (!intent)
    return "";

  jclass cIntent = env->GetObjectClass(intent);

  //action = intent.getAction()
  jmethodID mgetAction = env->GetMethodID(cIntent, "getAction", "()Ljava/lang/String;");
  env->DeleteLocalRef(cIntent);
  jstring sAction = (jstring)env->CallObjectMethod(intent, mgetAction);
  if (sAction == NULL)
    return "";
  
  const char *nativeString = env->GetStringUTFChars(sAction, 0);
  action = std::string(nativeString);
  env->ReleaseStringUTFChars(sAction, nativeString);

  return action;
}

std::string CAndroidIntents::GetIntentData(JNIEnv *env, const jobject &intent)
{
  std::string data;

 if (!intent)
    return "";

  jclass cIntent = env->GetObjectClass(intent);

  //data = intent.getDataString()
  jmethodID mgetData = env->GetMethodID(cIntent, "getDataString", "()Ljava/lang/String;");
  env->DeleteLocalRef(cIntent);
  jstring sData = (jstring)env->CallObjectMethod(intent, mgetData);
  if (sData == NULL)
    return "";
  
  const char *nativeString = env->GetStringUTFChars(sData, 0);
  data = std::string(nativeString);
  env->ReleaseStringUTFChars(sData, nativeString);

  return data;
}
