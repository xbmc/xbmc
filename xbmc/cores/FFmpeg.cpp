/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "cores/FFmpeg.h"

#include "ServiceBroker.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "threads/CriticalSection.h"
#include "threads/Thread.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <map>

static thread_local CFFmpegLog* CFFmpegLogTls;

void CFFmpegLog::SetLogLevel(int level)
{
  CFFmpegLog::ClearLogLevel();
  CFFmpegLog *log = new CFFmpegLog();
  log->level = level;
  CFFmpegLogTls = log;
}

int CFFmpegLog::GetLogLevel()
{
  CFFmpegLog* log = CFFmpegLogTls;
  if (!log)
    return -1;
  return log->level;
}

void CFFmpegLog::ClearLogLevel()
{
  CFFmpegLog* log = CFFmpegLogTls;
  CFFmpegLogTls = nullptr;
  if (log)
    delete log;
}

static CCriticalSection m_logSection;
std::map<const CThread*, std::string> g_logbuffer;

void ff_flush_avutil_log_buffers(void)
{
  CSingleLock lock(m_logSection);
  /* Loop through the logbuffer list and remove any blank buffers
     If the thread using the buffer is still active, it will just
     add a new buffer next time it writes to the log */
  std::map<const CThread*, std::string>::iterator it;
  for (it = g_logbuffer.begin(); it != g_logbuffer.end(); )
    if ((*it).second.empty())
      g_logbuffer.erase(it++);
    else
      ++it;
}

void ff_avutil_log(void* ptr, int level, const char* format, va_list va)
{
  CSingleLock lock(m_logSection);
  const CThread* threadId = CThread::GetCurrentThread();
  std::string &buffer = g_logbuffer[threadId];

  AVClass* avc= ptr ? *(AVClass**)ptr : NULL;

  int maxLevel = AV_LOG_WARNING;
  if (CFFmpegLog::GetLogLevel() > 0)
    maxLevel = AV_LOG_INFO;

  if (level > maxLevel &&
     !CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->CanLogComponent(LOGFFMPEG))
    return;
  else if (CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_logLevel <= LOG_LEVEL_NORMAL)
    return;

  int type;
  switch (level)
  {
    case AV_LOG_INFO:
      type = LOGINFO;
      break;

    case AV_LOG_ERROR:
      type = LOGERROR;
      break;

    case AV_LOG_DEBUG:
    default:
      type = LOGDEBUG;
      break;
  }

  std::string message = StringUtils::FormatV(format, va);
  std::string prefix = StringUtils::Format("ffmpeg[%pX]: ", static_cast<const void*>(threadId));
  if (avc)
  {
    if (avc->item_name)
      prefix += std::string("[") + avc->item_name(ptr) + "] ";
    else if (avc->class_name)
      prefix += std::string("[") + avc->class_name + "] ";
  }

  buffer += message;
  int pos, start = 0;
  while ((pos = buffer.find_first_of('\n', start)) >= 0)
  {
    if (pos > start)
      CLog::Log(type, "%s%s", prefix.c_str(), buffer.substr(start, pos - start).c_str());
    start = pos+1;
  }
  buffer.erase(0, start);
}

