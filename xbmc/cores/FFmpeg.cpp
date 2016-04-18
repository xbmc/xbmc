/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "cores/FFmpeg.h"
#include "utils/log.h"
#include "threads/CriticalSection.h"
#include "utils/StringUtils.h"
#include "threads/Thread.h"
#include "settings/AdvancedSettings.h"
#include "URL.h"
#include <map>

static XbmcThreads::ThreadLocal<CFFmpegLog> CFFmpegLogTls;

void CFFmpegLog::SetLogLevel(int level)
{
  CFFmpegLog::ClearLogLevel();
  CFFmpegLog *log = new CFFmpegLog();
  log->level = level;
  CFFmpegLogTls.set(log);
}

int CFFmpegLog::GetLogLevel()
{
  CFFmpegLog* log = CFFmpegLogTls.get();
  if (!log)
    return -1;
  return log->level;
}

void CFFmpegLog::ClearLogLevel()
{
  CFFmpegLog* log = CFFmpegLogTls.get();
  CFFmpegLogTls.set(nullptr);
  if (log)
    delete log;
}

/* callback for the ffmpeg lock manager */
int ffmpeg_lockmgr_cb(void **mutex, enum AVLockOp operation)
{
  CCriticalSection **lock = (CCriticalSection **)mutex;

  switch (operation)
  {
    case AV_LOCK_CREATE:
    {
      *lock = NULL;
      *lock = new CCriticalSection();
      if (*lock == NULL)
        return 1;
      break;
    }
    case AV_LOCK_OBTAIN:
      (*lock)->lock();
      break;

    case AV_LOCK_RELEASE:
      (*lock)->unlock();
      break;

    case AV_LOCK_DESTROY:
    {
      delete *lock;
      *lock = NULL;
      break;
    }

    default:
      return 1;
  }
  return 0;
}

static CCriticalSection m_logSection;
std::map<uintptr_t, std::string> g_logbuffer;

void ff_flush_avutil_log_buffers(void)
{
  CSingleLock lock(m_logSection);
  /* Loop through the logbuffer list and remove any blank buffers
     If the thread using the buffer is still active, it will just
     add a new buffer next time it writes to the log */
  std::map<uintptr_t, std::string>::iterator it;
  for (it = g_logbuffer.begin(); it != g_logbuffer.end(); )
    if ((*it).second.empty())
      g_logbuffer.erase(it++);
    else
      ++it;
}

void ff_avutil_log(void* ptr, int level, const char* format, va_list va)
{
  CSingleLock lock(m_logSection);
  uintptr_t threadId = (uintptr_t)CThread::GetCurrentThreadId();
  std::string &buffer = g_logbuffer[threadId];

  AVClass* avc= ptr ? *(AVClass**)ptr : NULL;

  int maxLevel = AV_LOG_WARNING;
  if (CFFmpegLog::GetLogLevel() > 0)
    maxLevel = AV_LOG_INFO;

  if (level > maxLevel &&
     !g_advancedSettings.CanLogComponent(LOGFFMPEG))
    return;
  else if (g_advancedSettings.m_logLevel <= LOG_LEVEL_NORMAL)
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
  std::string prefix = StringUtils::Format("ffmpeg[%lX]: ", threadId);
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
    if(pos > start)
    {
      std::vector<std::string> toLog = StringUtils::Split(buffer.substr(start, pos - start), "from '");
      if (toLog.size() > 1)
      {
       size_t pos = toLog[1].find_first_of('\'');
       std::string url = CURL::GetRedacted(toLog[1].substr(0, pos - 1));
       toLog[0] += url + toLog[1].substr(pos);
      }

      CLog::Log(type, "%s%s", prefix.c_str(), toLog[0].c_str());
    }
    start = pos+1;
  }
  buffer.erase(0, start);
}

