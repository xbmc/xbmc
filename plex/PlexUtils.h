#pragma once

#include <string>
#include <StdString.h>

namespace PlexUtils
{
std::string GetHostName();
bool IsValidIP(const std::string& address);
int FileAge(const CStdString &strFileName);
bool IsPlexMediaServer(const CStdString& strFile);
bool IsPlexWebKit(const CStdString& strFile);
bool IsStack(const CStdString& strFile);
std::string AppendPathToURL(const std::string& baseURL, const std::string& relativePath);
__int64_t Size(const CStdString& strFileName);
}

#ifdef _WIN32

bool Cocoa_IsHostLocal(const std::string& host);

struct timezone
{
  int  tz_minuteswest; /* minutes W of Greenwich */
  int  tz_dsttime;     /* type of dst correction */
};
 
int gettimeofday(struct timeval *tv, struct timezone *tz);

#ifndef usleep
typedef unsigned int useconds_t;
int usleep(useconds_t useconds);
#endif

#endif
