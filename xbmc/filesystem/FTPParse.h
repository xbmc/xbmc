#pragma once
/*
 *      Copyright (C) 2010 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include <string>
#include <ctime>

using namespace std;

class CFTPParse
{
public:
  CFTPParse();
  int FTPParse(string str);
  string getName();
  int getFlagtrycwd();
  int getFlagtryretr();
  long getSize();
  time_t getTime();
private:
  string m_name;            // not necessarily 0-terminated
  int m_flagtrycwd;         // 0 if cwd is definitely pointless, 1 otherwise
  int m_flagtryretr;        // 0 if retr is definitely pointless, 1 otherwise
  long m_size;              // number of octets
  time_t m_time;            // modification time
  void setTime(string str); // Method used to set m_time from a string
  int getDayOfWeek(int month, int date, int year); // Method to get day of week
};
