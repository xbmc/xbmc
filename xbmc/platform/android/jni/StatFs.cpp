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

#include "StatFs.h"
#include "jutils/jutils-details.hpp"

using namespace jni;

CJNIStatFs::CJNIStatFs(const std::string &path) : CJNIBase("android/os/StatFs")
{
  m_object = new_object(GetClassName(), "<init>",
    "(Ljava/lang/String;)V",
    jcast<jhstring>(path));
  m_object.setGlobal();
}

void CJNIStatFs::restat(const std::string &path)
{
  call_method<void>(m_object,
    "restat", "(Ljava/lang/String;)V",
    jcast<jhstring>(path));
}

int CJNIStatFs::getBlockSize()
{
  return call_method<jint>(m_object,
    "getBlockSize", "()I");
}

int CJNIStatFs::getBlockCount()
{
  return call_method<jint>(m_object,
    "getBlockCount", "()I");
}

int CJNIStatFs::getFreeBlocks()
{
  return call_method<jint>(m_object,
    "getFreeBlocks", "()I");
}

int CJNIStatFs::getAvailableBlocks()
{
  return call_method<jint>(m_object,
    "getAvailableBlocks", "()I");
}
