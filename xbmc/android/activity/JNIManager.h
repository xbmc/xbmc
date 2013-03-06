#pragma once
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
#include <vector>
#include <jni.h>

class CBroadcastReceiver;

struct jniNativeMethod
{
  std::string name;
  std::string signature;
  void *function;
  jniNativeMethod(std::string a, std::string b, void *c) : name(a), signature(b), function(c){};
};

class CAndroidJNIBase
{
friend class CAndroidJNIManager;
public:
  jclass GetClass() const {return m_class;};
  std::string GetClassName() const {return m_className;};

protected:
  CAndroidJNIBase(std::string classname) : m_className(classname), m_class(NULL) {};
  void AddMethod(jniNativeMethod method){ m_jniMethods.push_back(method); };

private:
  CAndroidJNIBase();
  CAndroidJNIBase(CAndroidJNIBase const&);
  void operator=(CAndroidJNIBase const&);

  std::string m_className;
  std::vector<jniNativeMethod> m_jniMethods;
  jclass m_class;
};

class CAndroidJNIManager
{
public:
  static CAndroidJNIManager &GetInstance() {static CAndroidJNIManager temp; return temp;};
  bool Load(JavaVM* vm, int jniVersion);
  jobject GetActivityInstance() const {return m_oActivity;};

  CBroadcastReceiver* GetBroadcastReceiver() const {return m_broadcastReceiver;};
  void SetActivityInstance(jobject oActivity) {m_oActivity = oActivity;};
private:
  CAndroidJNIManager();
  ~CAndroidJNIManager();
  CAndroidJNIManager(CAndroidJNIManager const&);
  void operator=(CAndroidJNIManager const&);
  bool RegisterClass(JNIEnv* env, CAndroidJNIBase *jniClass);

  CBroadcastReceiver *m_broadcastReceiver;

  jobject m_oActivity;
};
