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

#include "Locale.h"
#include "jutils/jutils-details.hpp"

using namespace jni;

#define CLASSNAME_LOCALE  "java/util/Locale"

CJNILocale::CJNILocale() : CJNIBase(CLASSNAME_LOCALE)
{ }

CJNILocale CJNILocale::getDefault()
{
  return call_static_method<jhobject>(CLASSNAME_LOCALE,
    "getDefault", "()L" CLASSNAME_LOCALE ";");
}

std::string CJNILocale::toString() const
{
  return jcast<std::string>(call_method<jhstring>(m_object,
    "toString", "()Ljava/lang/String;"));
}
