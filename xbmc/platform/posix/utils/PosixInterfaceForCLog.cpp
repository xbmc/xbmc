/*
 *  Copyright (C) 2014-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PosixInterfaceForCLog.h"
#include <stdio.h>
#include <time.h>
#include <sys/time.h>

#if defined(TARGET_DARWIN)
#include "platform/darwin/DarwinUtils.h"
#elif defined(TARGET_ANDROID)
#include "platform/android/activity/XBMCApp.h"
#endif // TARGET_ANDROID

struct FILEWRAP : public FILE
{};


CPosixInterfaceForCLog::CPosixInterfaceForCLog() :
  m_file(NULL)
{ }

CPosixInterfaceForCLog::~CPosixInterfaceForCLog()
{
  if (m_file)
    fclose(m_file);
  m_file = NULL;
}

bool CPosixInterfaceForCLog::OpenLogFile(const std::string &logFilename, const std::string &backupOldLogToFilename)
{
  if (m_file)
    return false; // file was already opened

  (void)remove(backupOldLogToFilename.c_str()); // if it's failed, try to continue
  (void)rename(logFilename.c_str(), backupOldLogToFilename.c_str()); // if it's failed, try to continue

  m_file = (FILEWRAP*)fopen(logFilename.c_str(), "wb");
  if (!m_file)
    return false; // error, can't open log file

  static const unsigned char BOM[3] = { 0xEF, 0xBB, 0xBF };
  (void)fwrite(BOM, sizeof(BOM), 1, m_file); // write BOM, ignore possible errors

  return true;
}

void CPosixInterfaceForCLog::CloseLogFile()
{
  if (m_file)
  {
    fclose(m_file);
    m_file = NULL;
  }
}

bool CPosixInterfaceForCLog::WriteStringToLog(const std::string &logString)
{
  if (!m_file)
    return false;

  const bool ret = (fwrite(logString.data(), logString.size(), 1, m_file) == 1) &&
                   (fwrite("\n", 1, 1, m_file) == 1);
  (void)fflush(m_file);

  return ret;
}

void CPosixInterfaceForCLog::PrintDebugString(const std::string &debugString)
{
#ifdef _DEBUG
#if defined(TARGET_DARWIN)
  CDarwinUtils::PrintDebugString(debugString);
#elif defined(TARGET_ANDROID)
  //print to adb
  CXBMCApp::android_printf("Debug Print: %s", debugString.c_str());
#endif // TARGET_ANDROID
#endif // _DEBUG
}

void CPosixInterfaceForCLog::GetCurrentLocalTime(int &hour, int &minute, int &second, double &milliseconds)
{
  struct tm localTime;
  struct timeval tv;

  if (gettimeofday(&tv, nullptr) != -1 && localtime_r(&tv.tv_sec, &localTime) != NULL)
  {
    hour   = localTime.tm_hour;
    minute = localTime.tm_min;
    second = localTime.tm_sec;
    milliseconds = static_cast<double>(tv.tv_usec) / 1000;
  }
  else
  {
    hour = minute = second = 0;
    milliseconds = 0.0;
  }
}
