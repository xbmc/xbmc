#ifndef __PLEX_LOG_H__
#define __PLEX_LOG_H__

#include "utils/log.h"

#ifdef _WIN32
#define dprintf(format,  ...) \
  CLog::Log(LOGINFO, format, __VA_ARGS__)

#define eprintf(format, ...) \
  CLog::Log(LOGERROR, format, __VA_ARGS__)

#undef wprintf
#define wprintf(format, ...) \
  CLog::Log(LOGWARNING, format, __VA_ARGS__)

#define iprintf(format, ...) \
  CLog::Log(LOGINFO, format, __VA_ARGS__)

#else
#define dprintf(format, args...) \
  CLog::Log(LOGINFO, format, ## args)

#define eprintf(format, args...) \
  CLog::Log(LOGERROR, format, ## args)

#undef wprintf
#define wprintf(format, args...) \
  CLog::Log(LOGWARNING, format, ## args)

#define iprintf(format, args...) \
  CLog::Log(LOGINFO, format, ## args)
#endif

#endif
