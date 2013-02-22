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

CAndroidIntents::CAndroidIntents()
{
}

void CAndroidIntents::ReceiveIntent(JNIEnv *env, const jobject &intent)
{
  std::string action = GetIntentAction(intent);
  CLog::Log(LOGDEBUG,"CAndroidIntents::ReceiveIntent: %s", action.c_str());
}

std::string CAndroidIntents::GetIntentAction(const jobject &intent)
{
  std::string action;
  JNIEnv *env;

 if (!intent)
    return "";

  CXBMCApp::AttachCurrentThread(&env, NULL);
  jclass cIntent = env->GetObjectClass(intent);

  //action = intent.getAction()
  jmethodID mgetAction = env->GetMethodID(cIntent, "getAction", "()Ljava/lang/String;");
  env->DeleteLocalRef(cIntent);
  jstring sAction = (jstring)env->CallObjectMethod(intent, mgetAction);
  const char *nativeString = env->GetStringUTFChars(sAction, 0);
  action = std::string(nativeString);

  env->ReleaseStringUTFChars(sAction, nativeString);
  CXBMCApp::DetachCurrentThread();
  return action;
}
