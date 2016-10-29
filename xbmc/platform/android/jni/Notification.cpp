#include "Notification.h"
#include "Activity.h"
#include "ClassLoader.h"

#include "jutils/jutils-details.hpp"

#include <string>


using namespace jni;

std::string  CJNINotification::CATEGORY_ALARM;
std::string  CJNINotification::CATEGORY_CALL;
std::string  CJNINotification::CATEGORY_EMAIL;
std::string  CJNINotification::CATEGORY_ERROR;
std::string  CJNINotification::CATEGORY_EVENT;
std::string  CJNINotification::CATEGORY_MESSAGE;
std::string  CJNINotification::CATEGORY_PROGRESS;
std::string  CJNINotification::CATEGORY_PROMO;
std::string  CJNINotification::CATEGORY_RECOMMENDATION;
std::string  CJNINotification::CATEGORY_SERVICE;
std::string  CJNINotification::CATEGORY_SOCIAL;
std::string  CJNINotification::CATEGORY_STATUS;
std::string  CJNINotification::CATEGORY_SYSTEM;
std::string  CJNINotification::CATEGORY_TRANSPORT;
int          CJNINotification::COLOR_DEFAULT;
int          CJNINotification::DEFAULT_ALL;
int          CJNINotification::DEFAULT_LIGHTS;
int          CJNINotification::DEFAULT_SOUND;
int          CJNINotification::DEFAULT_VIBRATE;
std::string  CJNINotification::EXTRA_BACKGROUND_IMAGE_URI;
std::string  CJNINotification::EXTRA_BIG_TEXT;
std::string  CJNINotification::EXTRA_COMPACT_ACTIONS;
std::string  CJNINotification::EXTRA_INFO_TEXT;
std::string  CJNINotification::EXTRA_LARGE_ICON;
std::string  CJNINotification::EXTRA_LARGE_ICON_BIG;
std::string  CJNINotification::EXTRA_MEDIA_SESSION;
std::string  CJNINotification::EXTRA_PEOPLE;
std::string  CJNINotification::EXTRA_PICTURE;
std::string  CJNINotification::EXTRA_PROGRESS;
std::string  CJNINotification::EXTRA_PROGRESS_INDETERMINATE;
std::string  CJNINotification::EXTRA_PROGRESS_MAX;
std::string  CJNINotification::EXTRA_SHOW_CHRONOMETER;
std::string  CJNINotification::EXTRA_SHOW_WHEN;
std::string  CJNINotification::EXTRA_SMALL_ICON;
std::string  CJNINotification::EXTRA_SUB_TEXT;
std::string  CJNINotification::EXTRA_SUMMARY_TEXT;
std::string  CJNINotification::EXTRA_TEMPLATE;
std::string  CJNINotification::EXTRA_TEXT;
std::string  CJNINotification::EXTRA_TEXT_LINES;
std::string  CJNINotification::EXTRA_TITLE;
std::string  CJNINotification::EXTRA_TITLE_BIG;
int          CJNINotification::FLAG_AUTO_CANCEL;
int          CJNINotification::FLAG_FOREGROUND_SERVICE;
int          CJNINotification::FLAG_GROUP_SUMMARY;
int          CJNINotification::FLAG_HIGH_PRIORITY;
int          CJNINotification::FLAG_INSISTENT;
int          CJNINotification::FLAG_LOCAL_ONLY;
int          CJNINotification::FLAG_NO_CLEAR;
int          CJNINotification::FLAG_ONGOING_EVENT;
int          CJNINotification::FLAG_ONLY_ALERT_ONCE;
int          CJNINotification::FLAG_SHOW_LIGHTS;
std::string  CJNINotification::INTENT_CATEGORY_NOTIFICATION_PREFERENCES;
int          CJNINotification::PRIORITY_DEFAULT;
int          CJNINotification::PRIORITY_HIGH;
int          CJNINotification::PRIORITY_LOW;
int          CJNINotification::PRIORITY_MAX;
int          CJNINotification::PRIORITY_MIN;
int          CJNINotification::STREAM_DEFAULT;
int          CJNINotification::VISIBILITY_PRIVATE;
int          CJNINotification::VISIBILITY_PUBLIC;
int          CJNINotification::VISIBILITY_SECRET;

const char *CJNINotification::m_classname = "android/app/Notification";

void CJNINotification::PopulateStaticFields()
{
  jhclass clazz = find_class(m_classname);
  if(GetSDKVersion() >= 21)
  {
    CATEGORY_ALARM		= jcast<std::string>(get_static_field<jhstring>(clazz, "CATEGORY_ALARM"));
    CATEGORY_CALL		= jcast<std::string>(get_static_field<jhstring>(clazz, "CATEGORY_CALL"));
    CATEGORY_EMAIL		= jcast<std::string>(get_static_field<jhstring>(clazz, "CATEGORY_EMAIL"));
    CATEGORY_ERROR		= jcast<std::string>(get_static_field<jhstring>(clazz, "CATEGORY_ERROR"));
    CATEGORY_EVENT		= jcast<std::string>(get_static_field<jhstring>(clazz, "CATEGORY_EVENT"));
    CATEGORY_MESSAGE		= jcast<std::string>(get_static_field<jhstring>(clazz, "CATEGORY_MESSAGE"));
    CATEGORY_PROGRESS		= jcast<std::string>(get_static_field<jhstring>(clazz, "CATEGORY_PROGRESS"));
    CATEGORY_PROMO		= jcast<std::string>(get_static_field<jhstring>(clazz, "CATEGORY_PROMO"));
    CATEGORY_RECOMMENDATION	= jcast<std::string>(get_static_field<jhstring>(clazz, "CATEGORY_RECOMMENDATION"));
    CATEGORY_SERVICE		= jcast<std::string>(get_static_field<jhstring>(clazz, "CATEGORY_SERVICE"));
    CATEGORY_SOCIAL		= jcast<std::string>(get_static_field<jhstring>(clazz, "CATEGORY_SOCIAL"));
    CATEGORY_STATUS		= jcast<std::string>(get_static_field<jhstring>(clazz, "CATEGORY_STATUS"));
    CATEGORY_SYSTEM		= jcast<std::string>(get_static_field<jhstring>(clazz, "CATEGORY_SYSTEM"));
    CATEGORY_TRANSPORT		= jcast<std::string>(get_static_field<jhstring>(clazz, "CATEGORY_TRANSPORT"));
    COLOR_DEFAULT		= (get_static_field<int>(clazz, "COLOR_DEFAULT"));
    EXTRA_BACKGROUND_IMAGE_URI	= jcast<std::string>(get_static_field<jhstring>(clazz, "EXTRA_BACKGROUND_IMAGE_URI"));
    EXTRA_BIG_TEXT		= jcast<std::string>(get_static_field<jhstring>(clazz, "EXTRA_BIG_TEXT"));
    EXTRA_COMPACT_ACTIONS		= jcast<std::string>(get_static_field<jhstring>(clazz, "EXTRA_COMPACT_ACTIONS"));
    EXTRA_MEDIA_SESSION		= jcast<std::string>(get_static_field<jhstring>(clazz, "EXTRA_MEDIA_SESSION"));
    EXTRA_TEMPLATE		= jcast<std::string>(get_static_field<jhstring>(clazz, "EXTRA_TEMPLATE"));
    INTENT_CATEGORY_NOTIFICATION_PREFERENCES	= jcast<std::string>(get_static_field<jhstring>(clazz, "INTENT_CATEGORY_NOTIFICATION_PREFERENCES"));
    VISIBILITY_PRIVATE		= (get_static_field<int>(clazz, "VISIBILITY_PRIVATE"));
    VISIBILITY_PUBLIC		= (get_static_field<int>(clazz, "VISIBILITY_PUBLIC"));
    VISIBILITY_SECRET		= (get_static_field<int>(clazz, "VISIBILITY_SECRET"));
  }
  if(GetSDKVersion() >= 20)
  {
    FLAG_GROUP_SUMMARY		= (get_static_field<int>(clazz, "FLAG_GROUP_SUMMARY"));
    FLAG_LOCAL_ONLY		= (get_static_field<int>(clazz, "FLAG_LOCAL_ONLY"));
  }
  if(GetSDKVersion() >= 19)
  {
    EXTRA_INFO_TEXT		= jcast<std::string>(get_static_field<jhstring>(clazz, "EXTRA_INFO_TEXT"));
    EXTRA_LARGE_ICON		= jcast<std::string>(get_static_field<jhstring>(clazz, "EXTRA_LARGE_ICON"));
    EXTRA_LARGE_ICON_BIG		= jcast<std::string>(get_static_field<jhstring>(clazz, "EXTRA_LARGE_ICON_BIG"));
    EXTRA_PEOPLE			= jcast<std::string>(get_static_field<jhstring>(clazz, "EXTRA_PEOPLE"));
    EXTRA_PICTURE			= jcast<std::string>(get_static_field<jhstring>(clazz, "EXTRA_PICTURE"));
    EXTRA_PROGRESS		= jcast<std::string>(get_static_field<jhstring>(clazz, "EXTRA_PROGRESS"));
    EXTRA_PROGRESS_INDETERMINATE	= jcast<std::string>(get_static_field<jhstring>(clazz, "EXTRA_PROGRESS_INDETERMINATE"));
    EXTRA_PROGRESS_MAX		= jcast<std::string>(get_static_field<jhstring>(clazz, "EXTRA_PROGRESS_MAX"));
    EXTRA_SHOW_CHRONOMETER	= jcast<std::string>(get_static_field<jhstring>(clazz, "EXTRA_SHOW_CHRONOMETER"));
    EXTRA_SHOW_WHEN		= jcast<std::string>(get_static_field<jhstring>(clazz, "EXTRA_SHOW_WHEN"));
    EXTRA_SMALL_ICON		= jcast<std::string>(get_static_field<jhstring>(clazz, "EXTRA_SMALL_ICON"));
    EXTRA_SUB_TEXT		= jcast<std::string>(get_static_field<jhstring>(clazz, "EXTRA_SUB_TEXT"));
    EXTRA_SUMMARY_TEXT		= jcast<std::string>(get_static_field<jhstring>(clazz, "EXTRA_SUMMARY_TEXT"));
    EXTRA_TEXT			= jcast<std::string>(get_static_field<jhstring>(clazz, "EXTRA_TEXT"));
    EXTRA_TEXT_LINES		= jcast<std::string>(get_static_field<jhstring>(clazz, "EXTRA_TEXT_LINES"));
    EXTRA_TITLE			= jcast<std::string>(get_static_field<jhstring>(clazz, "EXTRA_TITLE"));
    EXTRA_TITLE_BIG		= jcast<std::string>(get_static_field<jhstring>(clazz, "EXTRA_TITLE_BIG"));
  }
  if(GetSDKVersion() >= 16)
  {
    PRIORITY_DEFAULT		= (get_static_field<int>(clazz, "PRIORITY_DEFAULT"));
    PRIORITY_HIGH			= (get_static_field<int>(clazz, "PRIORITY_HIGH"));
    PRIORITY_LOW			= (get_static_field<int>(clazz, "PRIORITY_LOW"));
    PRIORITY_MAX			= (get_static_field<int>(clazz, "PRIORITY_MAX"));
    PRIORITY_MIN			= (get_static_field<int>(clazz, "PRIORITY_MIN"));
  }
  DEFAULT_ALL			= (get_static_field<int>(clazz, "DEFAULT_ALL"));
  DEFAULT_LIGHTS		= (get_static_field<int>(clazz, "DEFAULT_LIGHTS"));
  DEFAULT_SOUND			= (get_static_field<int>(clazz, "DEFAULT_SOUND"));
  DEFAULT_VIBRATE		= (get_static_field<int>(clazz, "DEFAULT_VIBRATE"));
  FLAG_AUTO_CANCEL		= (get_static_field<int>(clazz, "FLAG_AUTO_CANCEL"));
  FLAG_FOREGROUND_SERVICE	= (get_static_field<int>(clazz, "FLAG_FOREGROUND_SERVICE"));
  FLAG_HIGH_PRIORITY		= (get_static_field<int>(clazz, "FLAG_HIGH_PRIORITY"));
  FLAG_INSISTENT		= (get_static_field<int>(clazz, "FLAG_INSISTENT"));
  FLAG_NO_CLEAR			= (get_static_field<int>(clazz, "FLAG_NO_CLEAR"));
  FLAG_ONGOING_EVENT		= (get_static_field<int>(clazz, "FLAG_ONGOING_EVENT"));
  FLAG_ONLY_ALERT_ONCE		= (get_static_field<int>(clazz, "FLAG_ONLY_ALERT_ONCE"));
  FLAG_SHOW_LIGHTS		= (get_static_field<int>(clazz, "FLAG_SHOW_LIGHTS"));
  STREAM_DEFAULT		= (get_static_field<int>(clazz, "STREAM_DEFAULT"));
}

CJNINotification::CJNINotification()
: CJNIBase(CJNINotification::m_classname)
{
  m_object = new_object(CJNIContext::getClassLoader().loadClass(GetDotClassName()));
  m_object.setGlobal();
}
