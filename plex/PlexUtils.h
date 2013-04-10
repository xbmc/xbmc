#pragma once

#include <string>
#include <StdString.h>
#include "FileItem.h"

namespace PlexUtils
{
  std::string GetHostName();
  bool IsValidIP(const std::string& address);
  int FileAge(const CStdString &strFileName);
  bool IsPlexMediaServer(const CStdString& strFile);
  bool IsPlexWebKit(const CStdString& strFile);
  bool IsStack(const CStdString& strFile);
  std::string AppendPathToURL(const std::string& baseURL, const std::string& relativePath);
  int64_t Size(const CStdString& strFileName);
  std::string CacheImageUrl(const std::string& url);
  std::string CacheImageUrlAsync(const std::string &url);

  std::string GetMachinePlatform();
  std::string GetMachinePlatformVersion();
  bool IsLocalNetworkIP(const CStdString &host);

  std::string GetStreamCodecName(CFileItemPtr item);
  std::string GetStreamChannelName(CFileItemPtr item);

  CFileItemPtr GetSelectedStreamOfType(CFileItemPtr mediaPart, int streamType);
  void SetSelectedStream(CFileItemPtr mediaPart, int streamType, int id);
}

#ifdef _WIN32

bool Cocoa_IsHostLocal(const std::string& host);

#include <sys/timeb.h>
#ifndef gettimeofday
static inline int _private_gettimeofday( struct timeval *tv, void *tz )
{
  struct timeb t;
  ftime( &t );
  tv->tv_sec = t.time;
  tv->tv_usec = t.millitm * 1000;
  return 0;
}
#define gettimeofday(TV, TZ) _private_gettimeofday((TV), (TZ))
#endif

#ifndef usleep
typedef unsigned int useconds_t;
int usleep(useconds_t useconds);
#endif

#endif
