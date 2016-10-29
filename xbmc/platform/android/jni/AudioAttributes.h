#pragma once
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

#include "JNIBase.h"

namespace jni
{

class CJNIAudioAttributes : public CJNIBase
{
public:
  CJNIAudioAttributes(const jni::jhobject &object) : CJNIBase(object) {}

  static void PopulateStaticFields();

  static int CONTENT_TYPE_MOVIE;
  static int CONTENT_TYPE_MUSIC;

  static int FLAG_HW_AV_SYNC;

  static int USAGE_MEDIA;

protected:
  static const char *m_classname;

  static void GetStaticValue(jhclass &c, int &field, char *value);
};

class CJNIAudioAttributesBuilder : public CJNIBase
{
public:
  CJNIAudioAttributesBuilder();
  CJNIAudioAttributesBuilder(const jni::jhobject &object) : CJNIBase(object) {}

  CJNIAudioAttributes build();

  CJNIAudioAttributesBuilder setContentType(int contentType);
  CJNIAudioAttributesBuilder setFlags(int flags);
  CJNIAudioAttributesBuilder setLegacyStreamType(int streamType);
  CJNIAudioAttributesBuilder setUsage(int usage);

protected:
  static const char *m_classname;
};


};

