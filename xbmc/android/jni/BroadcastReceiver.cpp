/*
 *      Copyright (C) 2013 Team XBMC
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

#include "BroadcastReceiver.h"
#include "Intent.h"
#include "Context.h"
#include "Activity.h"
#include "ClassLoader.h"
#include "jutils/jutils-details.hpp"

using namespace jni;

CJNIBroadcastReceiver *CJNIBroadcastReceiver::m_receiverInstance(NULL);
CJNIBroadcastReceiver::CJNIBroadcastReceiver(const std::string &className) : CJNIBase(className)
{
  CJNIApplicationMainActivity *appInstance = CJNIApplicationMainActivity::GetAppInstance();
  if (!appInstance || className.empty())
    return;

  // Convert "the/class/name" to "the.class.name" as loadClass() expects it.
  std::string dotClassName = GetClassName();
  for (std::string::iterator it = dotClassName.begin(); it != dotClassName.end(); ++it)
  {
    if (*it == '/')
      *it = '.';
  }
  m_object = new_object(appInstance->getClassLoader().loadClass(dotClassName));
  m_receiverInstance = this;
  m_object.setGlobal();
}

void CJNIBroadcastReceiver::_onReceive(JNIEnv *env, jobject context, jobject intent)
{
  (void)env;
  (void)context;
  if(m_receiverInstance)
    m_receiverInstance->onReceive(CJNIIntent(jhobject(intent)));
}
