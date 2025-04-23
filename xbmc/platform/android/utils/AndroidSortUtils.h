/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/SortUtils.h"

#include <jni.h>
#include <map>
#include <set>
#include <string>

class CAndroidSortUtils
{
public:
  /**
   * @brief Initialize the Android sorting utilities
   * @param env JNI environment
   * @param thiz JNI object reference
   */
  static void Initialize(JNIEnv* env, jobject thiz);

  /**
   * @brief Compare two strings using the Android Collator for proper international sorting
   * @param left First string to compare
   * @param right Second string to compare
   * @param locale Locale identifier (e.g., "en_US", "zh_CN", etc.)
   * @return Negative if left < right, 0 if equal, positive if left > right
   */
  static int CompareWithCollator(const std::string& left, const std::string& right, const std::string& locale);
  
  /**
   * @brief Get sorting articles for a specific locale
   * @param locale Locale identifier
   * @return Set of articles to be ignored in sorting
   */
  static std::set<std::string> GetArticlesForLocale(const std::string& locale);
  
  /**
   * @brief Remove articles from text based on current Android locale
   * @param text Text to process
   * @return Text with sorting articles removed
   */
  static std::string RemoveArticles(const std::string& text);
  
  /**
   * @brief Get current Android system locale
   * @return Current locale string
   */
  static std::string GetCurrentLocale();

  /**
   * @brief Enhanced sorter for Android that uses proper collation
   * @param left First item to compare
   * @param right Second item to compare
   * @return true if left should be sorted before right
   */
  static bool AndroidSorterAscending(const SortItem &left, const SortItem &right);
  
  /**
   * @brief Enhanced descending sorter for Android that uses proper collation
   * @param left First item to compare
   * @param right Second item to compare
   * @return true if left should be sorted before right
   */
  static bool AndroidSorterDescending(const SortItem &left, const SortItem &right);

private:
  // Locale-specific article cache
  static std::map<std::string, std::set<std::string>> m_articleCache;
  static jobject m_collator;
  static jmethodID m_compareMethodID;
  static std::string m_currentLocale;
};