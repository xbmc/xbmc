#pragma once
/*
 *      Copyright (C) 2010-present Team Kodi
 *      http://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include <string>
#include <ctime>
#include <stdint.h>

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
