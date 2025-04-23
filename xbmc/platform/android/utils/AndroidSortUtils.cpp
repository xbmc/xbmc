/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AndroidSortUtils.h"
#include "ServiceBroker.h"
#include "utils/StringUtils.h"
#include "utils/CharsetConverter.h"
#include "utils/log.h"
#include "platform/android/activity/JNIMainActivity.h"
#include "CompileInfo.h"

std::map<std::string, std::set<std::string>> CAndroidSortUtils::m_articleCache;
jobject CAndroidSortUtils::m_collator = nullptr;
jmethodID CAndroidSortUtils::m_compareMethodID = nullptr;
std::string CAndroidSortUtils::m_currentLocale = "en_US";

void CAndroidSortUtils::Initialize(JNIEnv* env, jobject thiz)
{
  // Get the current locale from Android
  jclass localeClass = env->FindClass("java/util/Locale");
  jmethodID getDefaultMethod = env->GetStaticMethodID(localeClass, "getDefault", "()Ljava/util/Locale;");
  jobject localeObject = env->CallStaticObjectMethod(localeClass, getDefaultMethod);
  
  // Get locale toString
  jmethodID toStringMethod = env->GetMethodID(localeClass, "toString", "()Ljava/lang/String;");
  jstring localeString = static_cast<jstring>(env->CallObjectMethod(localeObject, toStringMethod));
  const char* localeChars = env->GetStringUTFChars(localeString, nullptr);
  m_currentLocale = localeChars;
  env->ReleaseStringUTFChars(localeString, localeChars);

  CLog::Log(LOGINFO, "CAndroidSortUtils::Initialize: Using locale: %s", m_currentLocale.c_str());
  
  // Get Collator instance for current locale
  jclass collatorClass = env->FindClass("java/text/Collator");
  jmethodID getInstance = env->GetStaticMethodID(collatorClass, "getInstance", "(Ljava/util/Locale;)Ljava/text/Collator;");
  jobject localCollator = env->CallStaticObjectMethod(collatorClass, getInstance, localeObject);
  
  // Make a global reference to keep it available
  m_collator = env->NewGlobalRef(localCollator);
  
  // Store the compare method
  m_compareMethodID = env->GetMethodID(collatorClass, "compare", "(Ljava/lang/String;Ljava/lang/String;)I");
  
  env->DeleteLocalRef(localCollator);
  env->DeleteLocalRef(localeObject);
  env->DeleteLocalRef(localeClass);
  env->DeleteLocalRef(collatorClass);
}

int CAndroidSortUtils::CompareWithCollator(const std::string& left, const std::string& right, const std::string& locale)
{
  JNIEnv* env = xbmc_jnienv();
  if (!env || !m_collator || !m_compareMethodID)
  {
    CLog::Log(LOGWARNING, "CAndroidSortUtils::CompareWithCollator: JNI environment not ready, falling back to standard comparison");
    return StringUtils::CompareNoCase(left, right);
  }
  
  jstring jLeft = env->NewStringUTF(left.c_str());
  jstring jRight = env->NewStringUTF(right.c_str());
  
  int result = env->CallIntMethod(m_collator, m_compareMethodID, jLeft, jRight);
  
  env->DeleteLocalRef(jLeft);
  env->DeleteLocalRef(jRight);
  
  return result;
}

std::set<std::string> CAndroidSortUtils::GetArticlesForLocale(const std::string& locale)
{
  // Check cache first
  auto it = m_articleCache.find(locale);
  if (it != m_articleCache.end())
    return it->second;
  
  std::set<std::string> articles;
  
  // This would normally be populated from JNI call to Android to get locale-specific articles
  // For now, populate with common articles in multiple languages
  if (locale.find("en") != std::string::npos)
  {
    // English articles
    articles.insert("the ");
    articles.insert("a ");
    articles.insert("an ");
  }
  else if (locale.find("es") != std::string::npos)
  {
    // Spanish articles
    articles.insert("el ");
    articles.insert("la ");
    articles.insert("los ");
    articles.insert("las ");
    articles.insert("un ");
    articles.insert("una ");
    articles.insert("unos ");
    articles.insert("unas ");
  }
  else if (locale.find("fr") != std::string::npos)
  {
    // French articles
    articles.insert("le ");
    articles.insert("la ");
    articles.insert("les ");
    articles.insert("l'");
    articles.insert("un ");
    articles.insert("une ");
    articles.insert("des ");
  }
  else if (locale.find("de") != std::string::npos)
  {
    // German articles
    articles.insert("der ");
    articles.insert("die ");
    articles.insert("das ");
    articles.insert("ein ");
    articles.insert("eine ");
  }
  else if (locale.find("zh") != std::string::npos)
  {
    // Chinese doesn't use articles like western languages
    // but we might handle measure words or special prefixes in the future
  }
  
  // Store in cache
  m_articleCache[locale] = articles;
  return articles;
}

std::string CAndroidSortUtils::RemoveArticles(const std::string& text)
{
  std::string locale = GetCurrentLocale();
  std::set<std::string> articles = GetArticlesForLocale(locale);
  
  for (const auto& article : articles)
  {
    if (text.size() > article.size() && 
        StringUtils::StartsWithNoCase(text, article))
      return text.substr(article.size());
  }
  
  return text;
}

std::string CAndroidSortUtils::GetCurrentLocale()
{
  return m_currentLocale;
}

bool CAndroidSortUtils::AndroidSorterAscending(const SortItem &left, const SortItem &right)
{
  bool result;
  std::wstring labelLeft, labelRight;
  if (preliminarySort(left, right, true, result, labelLeft, labelRight))
    return result;

  // Convert wide strings to UTF-8
  std::string utf8Left, utf8Right;
  g_charsetConverter.wToUTF8(labelLeft, utf8Left);
  g_charsetConverter.wToUTF8(labelRight, utf8Right);
  
  // Use Android Collator for comparison
  return CompareWithCollator(utf8Left, utf8Right, GetCurrentLocale()) < 0;
}

bool CAndroidSortUtils::AndroidSorterDescending(const SortItem &left, const SortItem &right)
{
  bool result;
  std::wstring labelLeft, labelRight;
  if (preliminarySort(left, right, true, result, labelLeft, labelRight))
    return result;

  // Convert wide strings to UTF-8
  std::string utf8Left, utf8Right;
  g_charsetConverter.wToUTF8(labelLeft, utf8Left);
  g_charsetConverter.wToUTF8(labelRight, utf8Right);
  
  // Use Android Collator for comparison (reversed)
  return CompareWithCollator(utf8Left, utf8Right, GetCurrentLocale()) > 0;
}