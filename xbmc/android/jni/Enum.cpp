/*
 *      Copyright (C) 2005-2014 Team Kodi
 *      http://kodi.tv
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

#include "Enum.h"
#include "jutils/jutils-details.hpp"

using namespace jni;

std::string CJNIEnum::name()
{
  return jcast<std::string>(call_method<jhstring>(m_object,
    "name", "()Ljava/lang/String;"));
}

std::string CJNIEnum::toString()
{
  return jcast<std::string>(call_method<jhstring>(m_object,
    "toString", "()Ljava/lang/String;"));
}
bool CJNIEnum::equals(const CJNIEnum &object)
{
  return call_method<jboolean>(m_object,
    "equals", "(Ljava/lang/Object;)Z", object.get_raw());
}
