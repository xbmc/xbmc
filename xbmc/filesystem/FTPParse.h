/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <ctime>
#include <stdint.h>
#include <string>

class CFTPParse
{
public:
  CFTPParse();
  int FTPParse(std::string str);
  std::string getName();
  int getFlagtrycwd();
  int getFlagtryretr();
  uint64_t getSize();
  time_t getTime();
private:
  std::string m_name;            // not necessarily 0-terminated
  int m_flagtrycwd;         // 0 if cwd is definitely pointless, 1 otherwise
  int m_flagtryretr;        // 0 if retr is definitely pointless, 1 otherwise
  uint64_t m_size;              // number of octets
  time_t m_time;            // modification time
  void setTime(std::string str); // Method used to set m_time from a string
  int getDayOfWeek(int month, int date, int year); // Method to get day of week
};
