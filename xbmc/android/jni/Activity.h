#pragma once
/*
 *      Copyright (C) 2014 Team XBMC
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

#include "JNIBase.h"
#include "Context.h"

struct ANativeActivity;

class CJNIActivity : public CJNIContext
{
public:
  CJNIActivity(const ANativeActivity *nativeActivity);
  ~CJNIActivity();

  static bool moveTaskToBack(bool nonRoot);

private:
  CJNIActivity();
};

///////////////////////////////////

class CJNIApplicationMainActivity : public CJNIActivity
{
public:
  CJNIApplicationMainActivity(const ANativeActivity *nativeActivity);
  ~CJNIApplicationMainActivity();

  static CJNIApplicationMainActivity* GetAppInstance() { return m_appInstance; }

  static void _onNewIntent(JNIEnv *env, jobject context, jobject intent);
  static void _onVolumeChanged(JNIEnv *env, jobject context, jint volume);

private:
  static CJNIApplicationMainActivity *m_appInstance;

protected:
  virtual void onNewIntent(CJNIIntent intent)=0;
  virtual void onVolumeChanged(int volume)=0;
};

