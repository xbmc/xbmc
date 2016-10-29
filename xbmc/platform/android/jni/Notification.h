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

class CJNINotification : public CJNIBase
{
public:
  CJNINotification(const jni::jhobject &object) : CJNIBase(object) {}
  CJNINotification();

  static void  PopulateStaticFields();

  static std::string  CATEGORY_ALARM;
  static std::string  CATEGORY_CALL;
  static std::string  CATEGORY_EMAIL;
  static std::string  CATEGORY_ERROR;
  static std::string  CATEGORY_EVENT;
  static std::string  CATEGORY_MESSAGE;
  static std::string  CATEGORY_PROGRESS;
  static std::string  CATEGORY_PROMO;
  static std::string  CATEGORY_RECOMMENDATION;
  static std::string  CATEGORY_SERVICE;
  static std::string  CATEGORY_SOCIAL;
  static std::string  CATEGORY_STATUS;
  static std::string  CATEGORY_SYSTEM;
  static std::string  CATEGORY_TRANSPORT;
  static int          COLOR_DEFAULT;
  static int          DEFAULT_ALL;
  static int          DEFAULT_LIGHTS;
  static int          DEFAULT_SOUND;
  static int          DEFAULT_VIBRATE;
  static std::string  EXTRA_BACKGROUND_IMAGE_URI;
  static std::string  EXTRA_BIG_TEXT;
  static std::string  EXTRA_COMPACT_ACTIONS;
  static std::string  EXTRA_INFO_TEXT;
  static std::string  EXTRA_LARGE_ICON;
  static std::string  EXTRA_LARGE_ICON_BIG;
  static std::string  EXTRA_MEDIA_SESSION;
  static std::string  EXTRA_PEOPLE;
  static std::string  EXTRA_PICTURE;
  static std::string  EXTRA_PROGRESS;
  static std::string  EXTRA_PROGRESS_INDETERMINATE;
  static std::string  EXTRA_PROGRESS_MAX;
  static std::string  EXTRA_SHOW_CHRONOMETER;
  static std::string  EXTRA_SHOW_WHEN;
  static std::string  EXTRA_SMALL_ICON;
  static std::string  EXTRA_SUB_TEXT;
  static std::string  EXTRA_SUMMARY_TEXT;
  static std::string  EXTRA_TEMPLATE;
  static std::string  EXTRA_TEXT;
  static std::string  EXTRA_TEXT_LINES;
  static std::string  EXTRA_TITLE;
  static std::string  EXTRA_TITLE_BIG;
  static int          FLAG_AUTO_CANCEL;
  static int          FLAG_FOREGROUND_SERVICE;
  static int          FLAG_GROUP_SUMMARY;
  static int          FLAG_HIGH_PRIORITY;
  static int          FLAG_INSISTENT;
  static int          FLAG_LOCAL_ONLY;
  static int          FLAG_NO_CLEAR;
  static int          FLAG_ONGOING_EVENT;
  static int          FLAG_ONLY_ALERT_ONCE;
  static int          FLAG_SHOW_LIGHTS;
  static std::string  INTENT_CATEGORY_NOTIFICATION_PREFERENCES;
  static int          PRIORITY_DEFAULT;
  static int          PRIORITY_HIGH;
  static int          PRIORITY_LOW;
  static int          PRIORITY_MAX;
  static int          PRIORITY_MIN;
  static int          STREAM_DEFAULT;
  static int          VISIBILITY_PRIVATE;
  static int          VISIBILITY_PUBLIC;
  static int          VISIBILITY_SECRET;

  static const char *m_classname;
};

