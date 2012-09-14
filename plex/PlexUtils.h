#pragma once

#include <string>

std::string GetHostName();
bool IsValidIP(const std::string& address);

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
