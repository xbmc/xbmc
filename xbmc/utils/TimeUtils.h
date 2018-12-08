/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <stdint.h>
#include <time.h>
#include <string>

class CDateTime;

int64_t CurrentHostCounter(void);
int64_t CurrentHostFrequency(void);

class CTimeUtils
{
public:
  
  /*!
   * @brief Update the time frame
   * @note Not threadsafe
   */
  static void UpdateFrameTime(bool flip);
  
  /*!
   * @brief Returns the frame time in MS
   * @note Not threadsafe
   */
  static unsigned int GetFrameTime();
  static CDateTime GetLocalTime(time_t time);
  
  /*!
   * @brief Returns a time string without seconds, i.e: HH:MM
   * @param hhmmss Time string in the format HH:MM:SS
  */
  static std::string WithoutSeconds(const std::string hhmmss);
private:
  static unsigned int frameTime;
};

