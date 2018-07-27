/*
 *  Copyright (C) 2014-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

typedef void* HANDLE; // forward declaration, to avoid inclusion of whole Windows.h

class CWin32InterfaceForCLog
{
public:
  CWin32InterfaceForCLog();
  ~CWin32InterfaceForCLog();
  bool OpenLogFile(const std::string& logFilename, const std::string& backupOldLogToFilename);
  void CloseLogFile(void);
  bool WriteStringToLog(const std::string& logString);
  void PrintDebugString(const std::string& debugString);
  static void GetCurrentLocalTime(int& hour, int& minute, int& second, double& millisecond);
private:
  HANDLE m_hFile;
};
