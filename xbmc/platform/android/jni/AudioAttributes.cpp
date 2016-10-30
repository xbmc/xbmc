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

#include "AudioAttributes.h"
#include "ClassLoader.h"

#include "JNIBase.h"
#include "jutils/jutils-details.hpp"

using namespace jni;

int CJNIAudioAttributes::CONTENT_TYPE_MOVIE       = -1;
int CJNIAudioAttributes::CONTENT_TYPE_MUSIC       = -1;
int CJNIAudioAttributes::FLAG_HW_AV_SYNC          = -1;
int CJNIAudioAttributes::USAGE_MEDIA              = -1;

const char *CJNIAudioAttributes::m_classname = "android/media/AudioAttributes";
const char *CJNIAudioAttributesBuilder::m_classname = "android/media/AudioAttributes$Builder";


void CJNIAudioAttributes::GetStaticValue(jhclass& c, int& field, char* value)
{
  jfieldID id = get_static_field_id<jclass>(c, value, "I");
  if (id != NULL)
    field = get_static_field<int>(c, value);
  else
    xbmc_jnienv()->ExceptionClear();
}


void CJNIAudioAttributes::PopulateStaticFields()
{
  int sdk = CJNIBase::GetSDKVersion();
  if (sdk >= 21)
  {
    jhclass c = find_class(m_classname);
    if (sdk >= 21)
    {
      GetStaticValue(c, CJNIAudioAttributes::CONTENT_TYPE_MOVIE, "CONTENT_TYPE_MOVIE");
      GetStaticValue(c, CJNIAudioAttributes::CONTENT_TYPE_MUSIC, "CONTENT_TYPE_MUSIC");
      GetStaticValue(c, CJNIAudioAttributes::FLAG_HW_AV_SYNC, "FLAG_HW_AV_SYNC");
      GetStaticValue(c, CJNIAudioAttributes::USAGE_MEDIA, "USAGE_MEDIA");

    }

  }
}


CJNIAudioAttributesBuilder::CJNIAudioAttributesBuilder()
  : CJNIBase(CJNIAudioAttributesBuilder::m_classname)
{
  m_object = new_object(GetClassName());
  m_object.setGlobal();
}

CJNIAudioAttributes CJNIAudioAttributesBuilder::build()
{
  return call_method<jhobject>(m_object,
   "build", "()Landroid/media/AudioAttributes;");
}

CJNIAudioAttributesBuilder CJNIAudioAttributesBuilder::setContentType(int contentType)
{
  return call_method<jhobject>(m_object,
   "setContentType", "(I)Landroid/media/AudioAttributes$Builder;", contentType);
}

CJNIAudioAttributesBuilder CJNIAudioAttributesBuilder::setFlags(int flags)
{
  return call_method<jhobject>(m_object,
   "setFlags", "(I)Landroid/media/AudioAttributes$Builder;", flags);
}

CJNIAudioAttributesBuilder CJNIAudioAttributesBuilder::setLegacyStreamType(int streamType)
{
  return call_method<jhobject>(m_object,
   "setLegacyStreamType", "(I)Landroid/media/AudioAttributes$Builder;", streamType);
}

CJNIAudioAttributesBuilder CJNIAudioAttributesBuilder::setUsage(int usage)
{
  return call_method<jhobject>(m_object,
   "setUsage", "(I)Landroid/media/AudioAttributes$Builder;", usage);
}
