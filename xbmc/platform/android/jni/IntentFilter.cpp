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

#include "IntentFilter.h"
#include "jutils/jutils-details.hpp"

using namespace jni;

CJNIIntentFilter::CJNIIntentFilter() : CJNIBase("android/content/IntentFilter")
{
  m_object = new_object(GetClassName());
}

CJNIIntentFilter::CJNIIntentFilter(const std::string &action) : CJNIBase("android/content/IntentFilter")
{
  m_object = new_object(GetClassName(),
    "<init>", "(Ljava/lang/String;)V",
    jcast<jhstring>(action));
}

void CJNIIntentFilter::addAction(std::string action)
{
  call_method<void>(m_object,
    "addAction", "(Ljava/lang/String;)V",
    jcast<jhstring>(action));
}

void CJNIIntentFilter::addDataScheme(std::string scheme)
{
  call_method<void>(m_object,
    "addDataScheme", "(Ljava/lang/String;)V",
    jcast<jhstring>(scheme));
}
