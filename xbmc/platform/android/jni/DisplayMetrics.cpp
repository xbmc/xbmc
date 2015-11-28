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

#include "DisplayMetrics.h"
#include "jutils/jutils-details.hpp"

using namespace jni;
const char *CJNIDisplayMetrics::m_classname = "platform/android/util/DisplayMetrics";

int 	CJNIDisplayMetrics::DENSITY_DEFAULT(-1);
int 	CJNIDisplayMetrics::DENSITY_HIGH(-1);
int 	CJNIDisplayMetrics::DENSITY_LOW(-1);
int 	CJNIDisplayMetrics::DENSITY_MEDIUM(-1);
int 	CJNIDisplayMetrics::DENSITY_TV(-1);
int 	CJNIDisplayMetrics::DENSITY_XHIGH(-1);
int 	CJNIDisplayMetrics::DENSITY_XXHIGH(-1);
int 	CJNIDisplayMetrics::DENSITY_XXXHIGH(-1);

void CJNIDisplayMetrics::PopulateStaticFields()
{
  jhclass clazz = find_class(m_classname);

  DENSITY_DEFAULT = get_static_field<jint>(clazz, "DENSITY_DEFAULT");
  DENSITY_HIGH = get_static_field<jint>(clazz, "DENSITY_HIGH");
  DENSITY_LOW = get_static_field<jint>(clazz, "DENSITY_LOW");
  DENSITY_MEDIUM = get_static_field<jint>(clazz, "DENSITY_MEDIUM");
  DENSITY_TV = get_static_field<jint>(clazz, "DENSITY_TV");
  DENSITY_XHIGH = get_static_field<jint>(clazz, "DENSITY_XHIGH");
  if(CJNIBase::GetSDKVersion() >= 16)
    DENSITY_XXHIGH = get_static_field<jint>(clazz, "DENSITY_XXHIGH");
  if(CJNIBase::GetSDKVersion() >= 18)
    DENSITY_XXXHIGH = get_static_field<jint>(clazz, "DENSITY_XXXHIGH");
}
