#pragma once

/*
*      Copyright (C) 2014 Team XBMC
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
#include <string>
#include <assert.h>

class IPlatformInterfaceForCLog
{
public:
  IPlatformInterfaceForCLog() 
  {}
  ~IPlatformInterfaceForCLog()
  {}
  bool OpenLogFile(const std::string& logFilename, const std::string& backupOldLogToFilename)
  { assert(0); return false; } // must be implemented in derived class
  void CloseLogFile(void)
  { assert(0); } // must be implemented in derived class
  bool WriteStringToLog(const std::string& logString)
  { assert(0); return false; } // must be implemented in derived class
  void PrintDebugString(const std::string& debugString)
  {} // can be unimplemented
  static void GetCurrentLocalTime(int& hour, int& minute, int& second)
  { assert(0); } // must be implemented in derived class
private:
  IPlatformInterfaceForCLog(const IPlatformInterfaceForCLog&); // disallow copy constructor
  IPlatformInterfaceForCLog& operator=(const IPlatformInterfaceForCLog&); // disallow assignment
};
