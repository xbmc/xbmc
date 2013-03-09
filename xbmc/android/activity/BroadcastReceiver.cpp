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
#include <jni.h>
#include "Intents.h"
#include "BroadcastReceiver.h"

CBroadcastReceiver::CBroadcastReceiver() : CAndroidJNIBase("org/xbmc/xbmc/XBMCBroadcastReceiver")
{
  AddMethod(jniNativeMethod("ReceiveIntent", "(Landroid/content/Intent;)V", (void*)jni_ReceiveIntent));
}

void CBroadcastReceiver::ReceiveIntent(JNIEnv *env, jobject thiz, jobject intent)
{
  CAndroidIntents::getInstance().ReceiveIntent(env, intent);
}

void jni_ReceiveIntent(JNIEnv *env, jobject thiz, jobject intent)
{
  CAndroidJNIManager::GetInstance().GetBroadcastReceiver()->ReceiveIntent(env, thiz, intent);
}
