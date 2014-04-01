#pragma once

#include <string>
#include <StdString.h>
#include "FileItem.h"
#include "utils/XBMCTinyXML.h"

#if defined(HAVE_EXECINFO_H)
#include <execinfo.h>
#endif

namespace PlexUtils
{
  std::string GetHostName();
  bool IsValidIP(const std::string& address);
  int FileAge(const CStdString &strFileName);
  bool IsPlexMediaServer(const CStdString& strFile);
  bool IsPlexWebKit(const CStdString& strFile);
  bool IsStack(const CStdString& strFile);
  std::string AppendPathToURL(const std::string& baseURL, const std::string& relativePath);
  void AppendPathToURL(CURL &baseURL, const std::string& relativePath);
  int64_t Size(const CStdString& strFileName);
  std::string CacheImageUrl(const std::string& url);
  std::string CacheImageUrlAsync(const std::string &url);

  std::string GetMachinePlatform();
  std::string GetMachinePlatformVersion();
  bool IsLocalNetworkIP(const CStdString &host);

  std::string GetStreamCodecName(CFileItemPtr item);
  std::string GetStreamChannelName(CFileItemPtr item);

  bool PlexMediaStreamCompare(CFileItemPtr stream1, CFileItemPtr stream2);
  CFileItemPtr GetSelectedStreamOfType(CFileItemPtr mediaPart, int streamType);
  CFileItemPtr GetItemSelectedStreamOfType(const CFileItem& fileItem, int streamType);
  void SetSelectedStream(CFileItemPtr item, CFileItemPtr stream);
  CFileItemPtr GetStreamByID(CFileItemPtr item, int streamType, int plexStreamID);

  bool CurrentSkinHasPreplay();
  bool CurrentSkinHasFilters();
  
  std::string GetPlexCrashPath();
  CStdString GetPrettyStreamNameFromStreamItem(CFileItemPtr stream);
  CStdString GetPrettyStreamName(const CFileItem& fileItem, bool audio);

  CStdString GetSHA1SumFromURL(const CURL &url);

  CStdString GetXMLString(const CXBMCTinyXML &document);

  bool MakeWakeupPipe(SOCKET *pipe);

  void LogStackTrace(char *FuncName);

  ePlexMediaType GetMediaTypeFromItem(CFileItemPtr item);
  ePlexMediaType GetMediaTypeFromItem(const CFileItem& item);
  std::string GetMediaTypeString(ePlexMediaType type);
  ePlexMediaType GetMediaTypeFromString(const std::string &typeString);
  ePlexMediaState GetMediaStateFromString(const std::string &statestring);
  std::string GetMediaStateString(ePlexMediaState state);

  unsigned long GetFastHash(std::string Data);
}

#if defined(HAVE_EXECINFO_H)
#define LOG_STACKTRACE  PlexUtils::LogStackTrace((char*)__PRETTY_FUNCTION__);
#endif

#ifdef _WIN32

bool Cocoa_IsHostLocal(const std::string& host);

#include <sys/timeb.h>
#ifndef gettimeofday
static inline int _plex_private_gettimeofday( struct timeval *tv, void *tz )
{
  struct timeb t;
  ftime( &t );
  tv->tv_sec = t.time;
  tv->tv_usec = t.millitm * 1000;
  return 0;
}
#define gettimeofday(TV, TZ) _plex_private_gettimeofday((TV), (TZ))
#endif

#ifndef usleep
typedef unsigned int useconds_t;
int usleep(useconds_t useconds);
#endif

#endif
